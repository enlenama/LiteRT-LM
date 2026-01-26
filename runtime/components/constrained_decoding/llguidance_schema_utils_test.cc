// Copyright 2025 The ODML Authors.
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

#include "runtime/components/constrained_decoding/llguidance_schema_utils.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/status/statusor.h"  // from @com_google_absl
#include "absl/strings/escaping.h"  // from @com_google_absl
#include "absl/strings/string_view.h"  // from @com_google_absl
#include "absl/types/source_location.h"  // from @com_google_absl
#include "nlohmann/json.hpp"  // from @nlohmann_json
#include "runtime/components/constrained_decoding/bitmap.h"
#include "runtime/components/constrained_decoding/constraint.h"
#include "runtime/components/constrained_decoding/llg_constraint_config.h"
#include "runtime/components/constrained_decoding/llg_constraint_provider.h"
#include "runtime/components/tokenizer.h"

namespace litert::lm {
namespace {

using ::testing::status::StatusIs;

// A simple mock tokenizer for testing.
// Special tokens:
// <pad>: 0
// <eos>: 1
class SimpleTokenizer : public Tokenizer {
 public:
  TokenizerType GetTokenizerType() const override {
    return TokenizerType::kSentencePiece;  // Dummy
  }

  absl::StatusOr<TokenIds> TextToTokenIds(absl::string_view text) override {
    TokenIds ids;
    absl::string_view remaining_text = text;

    while (!remaining_text.empty()) {
      int shortest_match_len = remaining_text.size() + 1;
      int token_id = -1;

      // Check special tokens
      for (const auto& pair : single_tokens_) {
        const std::string& token_str = pair.first;
        if (remaining_text.size() >= token_str.length() &&
            remaining_text.substr(0, token_str.length()) == token_str) {
          if (token_str.length() < shortest_match_len) {
            shortest_match_len = token_str.length();
            token_id = pair.second;
          }
        }
      }

      // Check single char (ASCII/Bytes), excluding '<'
      if (remaining_text[0] != '<') {
        if (1 < shortest_match_len) {
          shortest_match_len = 1;
          token_id = static_cast<unsigned char>(remaining_text[0]);
        }
      }

      if (token_id != -1) {
        ids.push_back(token_id);
        remaining_text.remove_prefix(shortest_match_len);
      } else {
        return absl::InternalError(
            absl::StrCat("Failed to tokenize at: ", remaining_text));
      }
    }
    return ids;
  }

  absl::StatusOr<int> TokenToId(absl::string_view token) override {
    if (token == "<pad>") return 0;
    if (token == "<eos>") return 1;

    for (const auto& pair : single_tokens_) {
      if (token == pair.first) {
        return pair.second;
      }
    }

    if (token.length() == 1 && token[0] != '<') {
      return static_cast<unsigned char>(token[0]);
    }
    return -1;
  }

  absl::StatusOr<std::string> TokenIdsToText(const TokenIds& ids) override {
    std::string text;
    for (int id : ids) {
      bool found = false;
      for (const auto& pair : single_tokens_) {
        if (id == pair.second) {
          text += pair.first;
          found = true;
          break;
        }
      }
      if (found) continue;

      if (id >= 0 && id < 256) {
        text += static_cast<char>(id);
      }
    }
    return text;
  }

  std::vector<std::string> GetTokens() const override {
    std::vector<std::string> tokens;
    tokens.resize(600);  // vocab size
    // Fill unused slots with unique garbage first to avoid duplicated tokens.
    for (int i = 0; i < tokens.size(); ++i) {
      tokens[i] = "[UNUSED_" + std::to_string(i) + "]";
    }

    for (const auto& pair : single_tokens_) {
      if (pair.second >= 0 && pair.second < tokens.size()) {
        bool is_special =
            special_tokens_.find(pair.first) != special_tokens_.end();
        if (is_special) {
          tokens[pair.second] = "\xff" + pair.first;
        } else {
          tokens[pair.second] = pair.first;
        }
      }
    }

    // Fill ASCII/Bytes
    for (int i = 0; i < 128; ++i) {
      if (i == '<') continue;  // "<" is always part of a special token.
      bool is_special = false;
      for (const auto& pair : single_tokens_) {
        if (pair.second == i) {
          is_special = true;
          break;
        }
      }
      if (!is_special) {
        tokens[i] = std::string(1, static_cast<char>(i));
      }
    }
    tokens[0] = "<pad>";
    tokens[1] = "<eos>";
    return tokens;
  }

 private:
  // Tokens that are treated as 1 single token instead of char sequence.
  const std::vector<std::pair<std::string, int>> single_tokens_ = {
      {"<start_function_call>", 501},
      {"<end_function_call>", 502},
      {"<escape>", 503},
      {"<start_function_response>", 504},
      {"<ctrl99>", 509},
      {"<ctrl100>", 510},
      // Some common html tags.
      {"<div>", 505},
      {"</div>", 506},
      {"<span>", 507},
      {"</span>", 508},
  };

  // Mark these with 0xFF prefix so that they are interpreted as control
  // tokens in regexes:
  // https://github.com/guidance-ai/llguidance/blob/main/docs/special_tokens.md
  const std::set<std::string> special_tokens_ = {
      "<start_function_call>",
      "<end_function_call>",
      "<start_function_response>",
      "<escape>",
      "<ctrl99>",
      "<ctrl100>",
  };
};

class LlguidanceSchemaUtilsTest : public testing::Test {
 protected:
  SimpleTokenizer tokenizer_;
  LlGuidanceConfig config_{.eos_id = 1};

  LlgConstraintsOptions GetDefaultOptions(LlgConstraintMode mode) {
    LlgConstraintsOptions options;
    options.constraint_mode = mode;
    options.fc2_code_fence_start = "<start_function_call>";
    options.fc2_code_fence_end = "<end_function_call>";
    options.fc2_function_response_start = "<start_function_response>";
    options.fc2_open_quote = "<escape>";
    options.fc2_close_quote = "<escape>";
    return options;
  }

  // Validates if the constraint accepts the text sequence.
  absl::StatusOr<bool> AcceptsInternal(Constraint& constraint,
                                       absl::string_view text) {
    ASSIGN_OR_RETURN(TokenIds ids, tokenizer_.TextToTokenIds(text));
    auto state = constraint.Start();
    for (int i = 0; i < ids.size(); ++i) {
      int id = ids[i];
      ASSIGN_OR_RETURN(auto bitmap, constraint.ComputeBitmap(*state));

      if (!bitmap->Get(id)) {
        // Rejected
        return false;
      }
      ASSIGN_OR_RETURN(state, constraint.ComputeNext(*state, id));
    }
    ASSIGN_OR_RETURN(auto final_bitmap, constraint.ComputeBitmap(*state));
    return final_bitmap->Get(*config_.eos_id);
    // return true;
  }

  void AssertAccepts(
      Constraint& constraint, absl::string_view text,
      absl::SourceLocation location = absl::SourceLocation::current()) {
    auto accepts_or = AcceptsInternal(constraint, text);
    if (!accepts_or.ok()) {
      ADD_FAILURE_AT(location.file_name(), location.line())
          << "AcceptsInternal failed for text: \"" << text
          << "\"\nStatus: " << accepts_or.status();
      return;
    }
    if (!*accepts_or) {
      ADD_FAILURE_AT(location.file_name(), location.line())
          << "Constraint failed to ACCEPT text: \""
          << absl::Utf8SafeCEscape(text) << "\"";
    }
  }

  void AssertRejects(
      Constraint& constraint, absl::string_view text,
      absl::SourceLocation location = absl::SourceLocation::current()) {
    auto accepts_or = AcceptsInternal(constraint, text);
    // Failure to process is considered a rejection.
    if (!accepts_or.ok() || !*accepts_or) return;
    if (*accepts_or) {
      ADD_FAILURE_AT(location.file_name(), location.line())
          << "Constraint failed to REJECT text: \""
          << absl::Utf8SafeCEscape(text) << "\"";
    }
  }

  // Helper to create constraint from tools
  std::unique_ptr<Constraint> CreateConstraint(
      const nlohmann::ordered_json& tools,
      const LlgConstraintsOptions& options) {
    auto provider_or = LlgConstraintProvider::Create(tokenizer_, config_);
    EXPECT_OK(provider_or);
    auto provider = std::move(*provider_or);

    auto res = FormatToolsAsLarkGrammar(tools, options);
    EXPECT_OK(res);
    auto constraint_or = provider->CreateConstraint(
        LlGuidanceConstraintArg{.constraint_type = LlgConstraintType::kLark,
                                .constraint_string = *res});
    EXPECT_OK(constraint_or);
    return std::move(*constraint_or);
  }
};

TEST_F(LlguidanceSchemaUtilsTest, Lark_TextOnly) {
  nlohmann::ordered_json tool = nlohmann::ordered_json::parse(R"json({
    "name": "get_weather"
  })json");
  nlohmann::ordered_json tools = nlohmann::ordered_json::array({tool});

  auto constraint =
      CreateConstraint(tools, GetDefaultOptions(LlgConstraintMode::kTextOnly));

  AssertAccepts(*constraint, "This is just plain text.");
  AssertAccepts(*constraint, "Some html tags <div>some text</div>");
  AssertRejects(
      *constraint,
      "Something <start_function_call>call:get_weather{}<end_function_call>");
}

TEST_F(LlguidanceSchemaUtilsTest, Lark_TextAndOr) {
  nlohmann::ordered_json tool1 = nlohmann::ordered_json::parse(R"json({
    "name": "get_weather",
    "parameters": {
      "type": "object",
      "properties": {
        "location": {
          "type": "string"
        },
        "unit": {
          "type": "string",
          "enum": ["celsius", "fahrenheit"]
        }
      },
      "required": ["location"]
    }
  })json");
  nlohmann::ordered_json tool2 = nlohmann::ordered_json::parse(R"json({
    "name": "find_movies",
    "parameters": {
      "type": "object",
      "properties": {
        "genres": {
          "type": "array",
          "items": {
            "type": "string"
          }
        }
      }
    }
  })json");
  nlohmann::ordered_json tools = nlohmann::ordered_json::array({tool1, tool2});

  auto constraint =
      CreateConstraint(tools, GetDefaultOptions(LlgConstraintMode::kTextAndOr));

  // Text only
  AssertAccepts(*constraint, "A normal text");

  // Single function call.
  AssertAccepts(
      *constraint,
      R"(<start_function_call>call:get_weather{location:<escape>Mountain View<escape>,unit:<escape>celsius<escape>}<end_function_call><start_function_response>)");

  // Single function call with text before
  AssertAccepts(
      *constraint,
      R"(Some normal text<start_function_call>call:find_movies{genres:[<escape>Action<escape>]}<end_function_call><start_function_response>)");

  // Multiple function calls.
  AssertAccepts(
      *constraint,
      R"(<start_function_call>call:get_weather{location:<escape>Mountain View<escape>}<end_function_call><start_function_call>call:find_movies{genres:[<escape>Action<escape>]}<end_function_call><start_function_response>)");

  // Multiple function calls with text before.
  AssertAccepts(
      *constraint,
      R"(Some normal text ... <start_function_call>call:get_weather{location:<escape>Mountain View<escape>,unit:<escape>celsius<escape>}<end_function_call><start_function_call>call:find_movies{genres:[<escape>Action<escape>,<escape>Comedy<escape>]}<end_function_call><start_function_response>)");

  // Rejects function call without <start_function_response> suffix.
  AssertRejects(
      *constraint,
      R"(<start_function_call>call:get_weather{location:<escape>Mountain View<escape>,unit:<escape>celsius<escape>}<end_function_call>)");
  // Rejects function call with wrong function name.
  AssertRejects(
      *constraint,
      R"(<start_function_call>call:get_weath{}<end_function_call><start_function_response>)");
  // Rejects function call with extra text after it.
  AssertRejects(
      *constraint,
      R"(<start_function_call>call:get_weather{}<end_function_call><start_function_response>extra text)");
}

TEST_F(LlguidanceSchemaUtilsTest, Lark_FunctionCallOnly) {
  nlohmann::ordered_json tool1 = nlohmann::ordered_json::parse(R"json({
    "name": "get_weather",
    "parameters": {
      "type": "object",
      "properties": {
        "location": {
          "type": "string"
        },
        "unit": {
          "type": "string",
          "enum": ["celsius", "fahrenheit"]
        }
      },
      "required": ["location"]
    }
  })json");
  nlohmann::ordered_json tool2 = nlohmann::ordered_json::parse(R"json({
    "name": "find_movies",
    "parameters": {
      "type": "object",
      "properties": {
        "genres": {
          "type": "array",
          "items": {
            "type": "string"
          }
        }
      }
    }
  })json");
  nlohmann::ordered_json tool3 = nlohmann::ordered_json::parse(R"json({
    "name": "get_time"
  })json");
  nlohmann::ordered_json tools =
      nlohmann::ordered_json::array({tool1, tool2, tool3});

  auto constraint = CreateConstraint(
      tools, GetDefaultOptions(LlgConstraintMode::kFunctionCallOnly));

  // Single function call.
  AssertAccepts(
      *constraint,
      R"(<start_function_call>call:get_weather{location:<escape>Mountain View<escape>,unit:<escape>celsius<escape>}<end_function_call><start_function_response>)");

  // Single function call without params.
  AssertAccepts(
      *constraint,
      R"(<start_function_call>call:get_time{}<end_function_call><start_function_response>)");

  // Multiple function calls.
  AssertAccepts(
      *constraint,
      R"(<start_function_call>call:get_weather{location:<escape>Mountain View<escape>}<end_function_call><start_function_call>call:find_movies{genres:[<escape>Action<escape>]}<end_function_call><start_function_response>)");

  // Rejects Text only
  AssertRejects(*constraint, "A normal text");

  // Rejects single function call with text before
  AssertRejects(
      *constraint,
      R"(Some normal text<start_function_call>call:find_movies{genres:[<escape>Action<escape>,<escape>Comedy<escape>]}<end_function_call><start_function_response>)");

  // Rejects multiple function calls with text before.
  AssertRejects(
      *constraint,
      R"(Some normal text ... <start_function_call>call:get_weather{location:<escape>Mountain View<escape>,unit:<escape>celsius<escape>}<end_function_call><start_function_call>call:find_movies{genres:[<escape>Action<escape>,<escape>Comedy<escape>]}<end_function_call><start_function_response>)");

  // Rejects function call without <start_function_response> suffix.
  AssertRejects(
      *constraint,
      R"(<start_function_call>call:get_weather{location:<escape>Mountain View<escape>,unit:<escape>celsius<escape>}<end_function_call>)");
  // Rejects function call with wrong function name.
  AssertRejects(
      *constraint,
      R"(<start_function_call>call:get_weath{}<end_function_call><start_function_response>)");
  // Rejects function call with extra text after it.
  AssertRejects(
      *constraint,
      R"(<start_function_call>call:get_weather{}<end_function_call><start_function_response>extra text)");
}

TEST_F(LlguidanceSchemaUtilsTest, EmptyTools_TextOnly_Lark) {
  nlohmann::ordered_json tools = nlohmann::ordered_json::array();
  auto constraint =
      CreateConstraint(tools, GetDefaultOptions(LlgConstraintMode::kTextOnly));
  AssertAccepts(*constraint, "Any text is fine.");
  AssertRejects(
      *constraint,
      "Text with <start_function_call>call:some_tool{}<end_function_call>");
}

TEST_F(LlguidanceSchemaUtilsTest, EmptyTools_TextAndOr_Lark) {
  nlohmann::ordered_json tools = nlohmann::ordered_json::array();
  auto constraint =
      CreateConstraint(tools, GetDefaultOptions(LlgConstraintMode::kTextAndOr));
  AssertAccepts(*constraint, "Any text is fine.");
  AssertRejects(
      *constraint,
      "Text with <start_function_call>call:some_tool{}<end_function_call>");
}

TEST_F(LlguidanceSchemaUtilsTest, EmptyTools_FunctionCallOnly_Lark) {
  nlohmann::ordered_json tools = nlohmann::ordered_json::array();
  auto res = FormatToolsAsLarkGrammar(
      tools, GetDefaultOptions(LlgConstraintMode::kFunctionCallOnly));
  EXPECT_THAT(res, StatusIs(absl::StatusCode::kInvalidArgument));
}

}  // namespace
}  // namespace litert::lm
