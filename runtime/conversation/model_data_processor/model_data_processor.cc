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

#include "runtime/conversation/model_data_processor/model_data_processor.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>

#include "absl/log/absl_log.h"  // from @com_google_absl
#include "absl/status/status.h"  // from @com_google_absl
#include "absl/status/statusor.h"  // from @com_google_absl
#include "absl/strings/match.h"  // from @com_google_absl
#include "absl/strings/str_cat.h"  // from @com_google_absl
#include "absl/strings/string_view.h"  // from @com_google_absl
#include "absl/synchronization/mutex.h"  // from @com_google_absl
#include "nlohmann/json_fwd.hpp"  // from @nlohmann_json
#include "nlohmann/json.hpp"  // from @nlohmann_json
#include "runtime/components/prompt_template.h"
#include "runtime/conversation/io_types.h"
#include "runtime/conversation/model_data_processor/config_registry.h"
#include "runtime/engine/io_types.h"
#include "runtime/util/status_macros.h"

namespace litert::lm {
namespace {

// Returns the number of overlapping characters between the suffix of string `a`
// and the prefix of string `b`.
size_t SuffixPrefixOverlap(absl::string_view a, absl::string_view b) {
  if (a.empty() || b.empty()) {
    return false;
  }

  size_t max_overlap = std::min(a.length(), b.length());

  for (size_t len = max_overlap; len > 0; --len) {
    if (a.substr(a.length() - len) == b.substr(0, len)) {
      return len;
    }
  }

  return 0;
}

}  // namespace

absl::StatusOr<std::string> ModelDataProcessor::GetSingleTurnText(
    const Message& message) const {
  PromptTemplateInput old_tmpl_input;
  if (std::holds_alternative<JsonPreface>(preface_)) {
    auto json_preface = std::get<JsonPreface>(preface_);

    for (auto& message : json_preface.messages) {
      ASSIGN_OR_RETURN(nlohmann::ordered_json message_tmpl_input,
                       MessageToTemplateInput(message));
      old_tmpl_input.messages.push_back(message_tmpl_input);
    }

    if (json_preface.tools.is_null()) {
      old_tmpl_input.tools = nullptr;
    } else {
      ASSIGN_OR_RETURN(old_tmpl_input.tools, FormatTools(json_preface.tools));
    }
    old_tmpl_input.extra_context = json_preface.extra_context;
  } else {
    return absl::UnimplementedError("Preface type is not supported yet");
  }
  absl::MutexLock lock(&history_mutex_);  // NOLINT
  for (const auto& history_msg : history_) {
    if (std::holds_alternative<nlohmann::ordered_json>(history_msg)) {
      ASSIGN_OR_RETURN(nlohmann::ordered_json message_tmpl_input,
                       MessageToTemplateInput(
                           std::get<nlohmann::ordered_json>(history_msg)));
      old_tmpl_input.messages.push_back(message_tmpl_input);
    } else {
      return absl::UnimplementedError("Message type is not supported yet");
    }
  }

  if (history_.empty()) {
    PromptTemplateInput new_tmpl_input = std::move(old_tmpl_input);
    if (std::holds_alternative<nlohmann::ordered_json>(message)) {
      ASSIGN_OR_RETURN(
          nlohmann::ordered_json message_tmpl_input,
          MessageToTemplateInput(std::get<nlohmann::ordered_json>(message)));
      new_tmpl_input.messages.push_back(message_tmpl_input);
    } else {
      return absl::UnimplementedError("Message type is not supported yet");
    }
    new_tmpl_input.add_generation_prompt = true;
    return prompt_template_.Apply(new_tmpl_input);
  }

  old_tmpl_input.add_generation_prompt = false;
  ASSIGN_OR_RETURN(const std::string old_string,
                   prompt_template_.Apply(old_tmpl_input));

  if (std::holds_alternative<nlohmann::ordered_json>(message)) {
    PromptTemplateInput new_tmpl_input = std::move(old_tmpl_input);
    ASSIGN_OR_RETURN(
        nlohmann::ordered_json message_tmpl_input,
        MessageToTemplateInput(std::get<nlohmann::ordered_json>(message)));
    new_tmpl_input.messages.push_back(message_tmpl_input);
    new_tmpl_input.add_generation_prompt = true;
    ASSIGN_OR_RETURN(const std::string& new_string,
                     prompt_template_.Apply(new_tmpl_input));
    if (new_string.substr(0, old_string.size()) != old_string) {
      return absl::InternalError(absl::StrCat(
          "The new rendered template string does not start with the previous "
          "rendered template string. \nold_string: ",
          old_string, "\nnew_string: ", new_string));
    }
    return {new_string.substr(old_string.size(),
                              new_string.size() - old_string.size())};
  } else {
    return absl::InvalidArgumentError("Json message is required for now.");
  }
}

absl::StatusOr<Message> ModelDataProcessor::SendMessage(
    const Message& message, std::optional<DataProcessorArguments> args) {
  if (!std::holds_alternative<nlohmann::ordered_json>(message)) {
    return absl::InvalidArgumentError("Json message is required for now.");
  }

  auto json_message = std::get<nlohmann::ordered_json>(message);
  ASSIGN_OR_RETURN(const std::string& single_turn_text,
                   GetSingleTurnText(message));
  absl::MutexLock lock(&history_mutex_);  // NOLINT
  history_.push_back(json_message);
  ASSIGN_OR_RETURN(
      const auto session_inputs,
      ToInputDataVector(single_turn_text,
                        nlohmann::ordered_json::array({json_message}),
                        args.value_or(std::monostate())));
  ASSIGN_OR_RETURN(const Responses& responses,
                   session_->GenerateContent(session_inputs));
  ASSIGN_OR_RETURN(const Message assistant_message,
                   ToMessage(responses, args.value_or(std::monostate())));
  history_.push_back(assistant_message);

  return assistant_message;
}

absl::Status ModelDataProcessor::SendMessageStream(
    const Message& message, std::unique_ptr<MessageCallbacks> callbacks,
    std::optional<DataProcessorArguments> args) {
  if (!std::holds_alternative<nlohmann::ordered_json>(message)) {
    return absl::InvalidArgumentError("Json message is required for now.");
  }
  auto json_message = std::get<nlohmann::ordered_json>(message);
  ASSIGN_OR_RETURN(const std::string& single_turn_text,
                   GetSingleTurnText(message));
  {
    absl::MutexLock lock(&history_mutex_);  // NOLINT
    history_.push_back(message);
  }

  ASSIGN_OR_RETURN(
      const auto session_inputs,
      ToInputDataVector(single_turn_text,
                        nlohmann::ordered_json::array({json_message}),
                        args.value_or(std::monostate())));

  auto internal_callbacks_adapter = InternalCallbacksAdapter::Create(
      this, std::move(callbacks), args.value_or(std::monostate()));

  InternalCallbacksAdapter::CompleteMessageCallback complete_message_callback =
      [this](const Message& complete_message) {
        absl::MutexLock lock(&this->history_mutex_);  // NOLINT
        this->history_.push_back(complete_message);
      };
  internal_callbacks_adapter->SetCompleteMessageCallback(
      std::move(complete_message_callback));

  RETURN_IF_ERROR(session_->RunPrefill(session_inputs));
  RETURN_IF_ERROR(
      session_->RunDecodeAsync(std::move(internal_callbacks_adapter)));
  return absl::OkStatus();
}

std::unique_ptr<InternalCallbacksAdapter> InternalCallbacksAdapter::Create(
    ModelDataProcessor* model_data_processor,
    std::unique_ptr<MessageCallbacks> user_callbacks,
    DataProcessorArguments processor_args) {
  return std::unique_ptr<InternalCallbacksAdapter>(new InternalCallbacksAdapter(
      model_data_processor, std::move(user_callbacks), processor_args));
}

void InternalCallbacksAdapter::SetCompleteMessageCallback(
    CompleteMessageCallback complete_message_callback) {
  complete_message_callback_ = complete_message_callback;
}

void InternalCallbacksAdapter::OnNext(const Responses& responses) {
  const auto& response_text = responses.GetResponseTextAt(0);
  if (!response_text.ok()) {
    user_callbacks_->OnError(response_text.status());
    return;
  }
  auto status = ProcessResponseText(*response_text);
  if (!status.ok()) {
    user_callbacks_->OnError(status);
    return;
  }
}

void InternalCallbacksAdapter::OnDone() {
  // Send the remaining text to the user callbacks when done.
  if (cursor_ < accumulated_response_text_.size()) {
    SendMessage(absl::string_view(accumulated_response_text_).substr(cursor_));
  }
  Responses responses(1);
  responses.GetMutableResponseTexts()[0] = accumulated_response_text_;
  const auto& complete_message =
      model_data_processor_->ToMessage(responses, processor_args_);
  if (!complete_message.ok()) {
    user_callbacks_->OnError(complete_message.status());
    return;
  }
  user_callbacks_->OnComplete();
  if (complete_message_callback_) {
    complete_message_callback_(*complete_message);
  }
  complete_message_callback_ = nullptr;
}

void InternalCallbacksAdapter::OnError(const absl::Status& status) {
  // TODO: b/435001805 - handle the max kv-cache size reached situation more
  // robustly.
  if (absl::StrContainsIgnoreCase(status.message(),
                                  "Maximum kv-cache size reached")) {
    ABSL_LOG(INFO) << "Maximum kv-cache size reached.";
    OnDone();
    return;
  }
  user_callbacks_->OnError(status);
}

InternalCallbacksAdapter::InternalCallbacksAdapter(
    ModelDataProcessor* model_data_processor,
    std::unique_ptr<MessageCallbacks> user_callbacks,
    DataProcessorArguments processor_args)
    : model_data_processor_(model_data_processor),
      user_callbacks_(std::move(user_callbacks)),
      processor_args_(processor_args) {}

void InternalCallbacksAdapter::SendMessage(absl::string_view text) {
  if (text.empty()) {
    return;
  }
  Responses responses(1);
  responses.GetMutableResponseTexts()[0] = text;
  const auto& message =
      model_data_processor_->ToMessage(responses, processor_args_);
  if (!message.ok()) {
    user_callbacks_->OnError(message.status());
    return;
  }
  user_callbacks_->OnMessage(*message);
}

absl::Status InternalCallbacksAdapter::ProcessResponseText(
    absl::string_view response_text) {
  accumulated_response_text_.append(response_text);
  if (model_data_processor_ == nullptr) {
    return absl::InvalidArgumentError(
        "model_data_processor_ must not be null.");
  }
  absl::string_view code_fence_start = model_data_processor_->CodeFenceStart();
  absl::string_view code_fence_end = model_data_processor_->CodeFenceEnd();

  while (cursor_ < accumulated_response_text_.size()) {
    if (!inside_tool_call_) {
      size_t code_fence_start_pos =
          accumulated_response_text_.find(code_fence_start, cursor_);
      if (code_fence_start_pos != std::string::npos) {
        // The text from the cursor up to the code fence is normal text.
        SendMessage(absl::string_view(accumulated_response_text_)
                        .substr(cursor_, code_fence_start_pos - cursor_));

        // Move cursor up to code_fence_start.
        cursor_ = code_fence_start_pos;
        inside_tool_call_ = true;
      } else {
        // code_fence_start not found, but we still need to check
        // if there's a partial match at the end of the string.
        size_t overlap = SuffixPrefixOverlap(
            absl::string_view(accumulated_response_text_).substr(cursor_),
            code_fence_start);

        if (overlap > 0) {
          // There's a partial match of the code fence at the end of the
          // string.
          size_t possible_start_pos =
              accumulated_response_text_.size() - overlap;

          // Call the callback with text up to the potential start of the
          // code fence.
          SendMessage(absl::string_view(accumulated_response_text_)
                          .substr(cursor_, possible_start_pos - cursor_));

          // Move cursor up to potential start of code fence.
          cursor_ = possible_start_pos;
          break;
        } else {
          // Remaining string is text.
          SendMessage(
              absl::string_view(accumulated_response_text_).substr(cursor_));
          cursor_ = accumulated_response_text_.size();
        }
      }
    }

    if (inside_tool_call_) {
      // Look for code fence end.
      size_t code_fence_end_pos = accumulated_response_text_.find(
          code_fence_end, cursor_ + code_fence_start.size());
      if (code_fence_end_pos != std::string::npos) {
        SendMessage(absl::string_view(accumulated_response_text_)
                        .substr(cursor_, code_fence_end_pos +
                                             code_fence_end.size() - cursor_));

        // Move cursor to end of tool code block.
        cursor_ = code_fence_end_pos + code_fence_end.size();
        inside_tool_call_ = false;
      } else {
        // We're inside a tool call but the code fence end has not been
        // found. Break for the next token.
        break;
      }
    }
  }

  return absl::OkStatus();
}

}  // namespace litert::lm