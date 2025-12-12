// Copyright 2025 The Google AI Edge Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

use antlr4rust::common_token_stream::CommonTokenStream;
use antlr4rust::error_strategy::BailErrorStrategy;
use antlr4rust::tree::{ParseTree, ParseTreeListener};
use antlr4rust::InputStream;
use antlr_python_tool_call_parser::{
    antlrpythonlexer, antlrpythonparser, antlrpythonparserlistener,
};
use antlrpythonlexer::AntlrPythonLexer;
use antlrpythonparser::{
    AntlrPythonParser, AntlrPythonParserContextType, AntlrPythonParserTreeWalker,
    ArgValContextAttrs, ArgValExprContextAttrs, DictContext, DictContextAttrs,
    EmptyFunctionCallContextAttrs, FullFunctionCallContextAttrs, FunctionCallContext,
    FunctionCallContextAttrs, ListContext, ListContextAttrs, ObjectContext, ObjectContextAttrs,
    ValueContext, ValueContextAttrs,
};
use antlrpythonparserlistener::AntlrPythonParserListener;
use protobuf::prelude::*;
use std::collections::HashSet;
use tool_call_rust_proto::{Field, ListValue, NullValue, Struct, ToolCall, ToolCalls, Value};

#[cxx::bridge(namespace = "litert::lm")]
pub mod ffi {
    struct ToolCallResult {
        serialized_tool_calls: Vec<u8>,
        is_ok: bool,
        error: String,
    }

    extern "Rust" {
        fn parse_python_expression(text: &str) -> ToolCallResult;
    }
}

impl ffi::ToolCallResult {
    pub fn with_tool_calls(tool_calls: Vec<u8>) -> Self {
        Self { serialized_tool_calls: tool_calls, is_ok: true, error: String::new() }
    }

    pub fn with_error(error: String) -> Self {
        Self { serialized_tool_calls: Vec::new(), is_ok: false, error: error }
    }
}

impl Default for ffi::ToolCallResult {
    fn default() -> Self {
        Self { serialized_tool_calls: Vec::new(), is_ok: true, error: String::new() }
    }
}

fn strip_quotes(s: &str) -> &str {
    if s.len() >= 2 {
        let first = s.chars().next().unwrap();
        let last = s.chars().last().unwrap();
        if (first == '"' && last == '"') || (first == '\'' && last == '\'') {
            return &s[1..s.len() - 1];
        }
    }
    s
}

fn parse_value(value_ctx: &ValueContext) -> Result<Value, String> {
    if let Some(int_token) = value_ctx.INT() {
        let text = int_token.get_text();
        if let Ok(val) = text.parse::<i64>() {
            Ok(proto!(Value { number_value: val as f64 }))
        } else {
            Err(format!("Invalid int: {}", text))
        }
    } else if let Some(float_token) = value_ctx.FLOAT() {
        let text = float_token.get_text();
        if let Ok(val) = text.parse::<f64>() {
            Ok(proto!(Value { number_value: val }))
        } else {
            Err(format!("Invalid float: {}", text))
        }
    } else if let Some(str_token) = value_ctx.STRING() {
        let text = str_token.get_text();
        Ok(proto!(Value { string_value: strip_quotes(&text).to_string() }))
    } else if let Some(bool_token) = value_ctx.BOOL() {
        Ok(proto!(Value { bool_value: bool_token.get_text() == "True" }))
    } else if let Some(_) = value_ctx.NONE() {
        Ok(proto!(Value { null_value: NullValue::default() }))
    } else if let Some(list_ctx) = value_ctx.list() {
        let l = parse_list(&list_ctx)?;
        Ok(proto!(Value { list_value: l }))
    } else if let Some(dict_ctx) = value_ctx.dict() {
        let s = parse_dict(&dict_ctx)?;
        Ok(proto!(Value { struct_value: s }))
    } else if let Some(obj_ctx) = value_ctx.object() {
        let s = parse_object(&obj_ctx)?;
        Ok(proto!(Value { struct_value: s }))
    } else {
        Err(format!("Unhandled value type: {}", value_ctx.get_text()))
    }
}

fn parse_list(list_ctx: &ListContext) -> Result<ListValue, String> {
    let mut list_value = ListValue::new();
    for value in list_ctx.value_all() {
        let parsed_value = parse_value(&value)?;
        list_value.values_mut().push(parsed_value);
    }
    Ok(list_value)
}

fn parse_dict(dict_ctx: &DictContext) -> Result<Struct, String> {
    let mut dict = Struct::new();
    let keys = dict_ctx.key_all();
    let values = dict_ctx.value_all();

    if keys.len() != values.len() {
        return Err("Dictionary keys and values count mismatch".to_string());
    }

    let mut seen_keys = HashSet::new();

    for (i, key_token) in keys.iter().enumerate() {
        let key_raw = key_token.get_text();
        let key = strip_quotes(&key_raw).to_string();

        if seen_keys.contains(&key) {
            // Log duplicate key but don't treat it as an error.
            eprintln!("Ignoring duplicate key: {}", key);
            continue;
        }
        seen_keys.insert(key.clone());

        let value_ctx = &values[i];
        let parsed_value = parse_value(value_ctx)?;

        let mut field = Field::new();
        field.set_name(key);
        field.set_value(parsed_value);
        dict.fields_mut().push(field);
    }
    Ok(dict)
}

fn parse_object(object_ctx: &ObjectContext) -> Result<Struct, String> {
    let mut object = Struct::new();

    let name_token = object_ctx.NAME().ok_or("Missing object name")?;
    let type_name = name_token.get_text();

    let mut type_field = Field::new();
    type_field.set_name("__type__".to_string());
    type_field.set_value(proto!(Value { string_value: type_name }));
    object.fields_mut().push(type_field);

    if let Some(arg_val_expr) = object_ctx.argValExpr() {
        let mut seen_keys = HashSet::new();
        for arg_val in arg_val_expr.argVal_all() {
            let arg_name_token = arg_val.NAME().ok_or("Missing argument name")?;
            let arg_name = arg_name_token.get_text();

            if seen_keys.contains(&arg_name) {
                return Err(format!("Duplicate key: {}", arg_name));
            }
            seen_keys.insert(arg_name.clone());

            let val_ctx = arg_val.value().ok_or("Missing argument value")?;
            let parsed_val = parse_value(&val_ctx)?;

            let mut field = Field::new();
            field.set_name(arg_name);
            field.set_value(parsed_val);
            object.fields_mut().push(field);
        }
    }

    Ok(object)
}

struct PythonListener {
    tool_calls: Result<ToolCalls, String>,
}

impl PythonListener {
    fn new() -> Self {
        PythonListener { tool_calls: Ok(ToolCalls::default()) }
    }

    fn tool_calls(self) -> Result<ToolCalls, String> {
        self.tool_calls
    }
}

impl<'input> ParseTreeListener<'input, AntlrPythonParserContextType> for PythonListener {}

impl<'input> AntlrPythonParserListener<'input> for PythonListener {
    #[allow(non_snake_case)]
    fn enter_functionCall(&mut self, ctx: &FunctionCallContext<'input>) {
        if self.tool_calls.is_err() {
            return;
        }

        if let Some(full_ctx) = ctx.fullFunctionCall() {
            let name = match full_ctx.NAME() {
                Some(n) => n.get_text(),
                None => {
                    self.tool_calls = Err("Missing function name".to_string());
                    return;
                }
            };

            let mut tool_call = ToolCall::new();
            tool_call.set_name(name);

            let mut args = Struct::new();
            if let Some(arg_val_expr) = full_ctx.argValExpr() {
                let mut seen_keys = HashSet::new();
                for arg_val in arg_val_expr.argVal_all() {
                    let arg_name_token = match arg_val.NAME() {
                        Some(n) => n,
                        None => {
                            self.tool_calls = Err("Missing argument name".to_string());
                            return;
                        }
                    };
                    let arg_name = arg_name_token.get_text();

                    if seen_keys.contains(&arg_name) {
                        self.tool_calls = Err(format!("Duplicate key: {}", arg_name));
                        return;
                    }
                    seen_keys.insert(arg_name.clone());

                    let val_ctx = match arg_val.value() {
                        Some(v) => v,
                        None => {
                            self.tool_calls = Err("Missing argument value".to_string());
                            return;
                        }
                    };

                    match parse_value(&val_ctx) {
                        Ok(v) => {
                            let mut field = Field::new();
                            field.set_name(arg_name);
                            field.set_value(v);
                            args.fields_mut().push(field);
                        }
                        Err(e) => {
                            self.tool_calls = Err(e);
                            return;
                        }
                    }
                }
            }
            tool_call.set_arguments(args);
            self.tool_calls.as_mut().unwrap().tool_calls_mut().push(tool_call);
        } else if let Some(empty_ctx) = ctx.emptyFunctionCall() {
            let name = match empty_ctx.NAME() {
                Some(n) => n.get_text(),
                None => {
                    self.tool_calls = Err("Missing function name".to_string());
                    return;
                }
            };
            let mut tool_call = ToolCall::new();
            tool_call.set_name(name);
            tool_call.set_arguments(Struct::new());
            self.tool_calls.as_mut().unwrap().tool_calls_mut().push(tool_call);
        }
    }
}

pub fn parse_python_expression(text: &str) -> ffi::ToolCallResult {
    if text.len() == 0 {
        return ffi::ToolCallResult::default();
    }
    let lexer = AntlrPythonLexer::new(InputStream::new(text));
    let mut parser = AntlrPythonParser::with_strategy(
        CommonTokenStream::new(lexer),
        Box::new(BailErrorStrategy::new()),
    );
    let start = match parser.main() {
        Ok(start) => start,
        Err(e) => return ffi::ToolCallResult::with_error(e.to_string()),
    };
    match AntlrPythonParserTreeWalker::walk(Box::new(PythonListener::new()), start.as_ref()) {
        Ok(listener) => match listener.tool_calls() {
            Ok(tool_calls) => match tool_calls.serialize() {
                Ok(serialized_tool_calls) => {
                    ffi::ToolCallResult::with_tool_calls(serialized_tool_calls)
                }
                Err(e) => ffi::ToolCallResult::with_error(e.to_string()),
            },
            Err(e) => ffi::ToolCallResult::with_error(e.to_string()),
        },
        Err(e) => ffi::ToolCallResult::with_error(e.to_string()),
    }
}
