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

#ifndef THIRD_PARTY_ODML_LITERT_LM_RUNTIME_CONVERSATION_MODEL_DATA_PROCESSOR_MODEL_DATA_PROCESSOR_H_
#define THIRD_PARTY_ODML_LITERT_LM_RUNTIME_CONVERSATION_MODEL_DATA_PROCESSOR_MODEL_DATA_PROCESSOR_H_

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "absl/base/nullability.h"  // from @com_google_absl
#include "absl/base/thread_annotations.h"  // from @com_google_absl
#include "absl/status/status.h"  // from @com_google_absl
#include "absl/status/statusor.h"  // from @com_google_absl
#include "absl/strings/string_view.h"  // from @com_google_absl
#include "absl/synchronization/mutex.h"  // from @com_google_absl
#include "nlohmann/json.hpp"  // from @nlohmann_json
#include "runtime/components/prompt_template.h"
#include "runtime/conversation/io_types.h"
#include "runtime/conversation/model_data_processor/config_registry.h"
#include "runtime/engine/engine.h"
#include "runtime/engine/io_types.h"

namespace litert::lm {

// ModelDataProcessor is a model-specific component that converts between the
// generic Json messages and the Litert LM InputData type.
class ModelDataProcessor {
 public:
  virtual ~ModelDataProcessor() = default;

  // - The complete message from the LLM.
  absl::StatusOr<Message> SendMessage(
      const Message& message,
      std::optional<DataProcessorArguments> args = std::nullopt);

  // Sends a message to the LLM and process the asynchronous message results via
  // the callbacks.
  // Args:
  // - `message`: The message to be sent to the LLM.
  // - `callbacks`: The callbacks to receive the message events.
  // - `args`: The optional arguments for the corresponding model data
  //    processor. Most of the time, the users don't need to provide this
  //    argument.
  // Returns :
  // - absl::OkStatus if the message is sent and processing successfully,
  //   otherwise the error status.
  absl::Status SendMessageStream(
      const Message& message, std::unique_ptr<MessageCallbacks> callbacks,
      std::optional<DataProcessorArguments> args = std::nullopt);

  // Converts a rendered template prompt and a list of messages to a vector of
  // InputData, which is the input to the LLM Session.
  virtual absl::StatusOr<std::vector<InputData>> ToInputDataVector(
      const std::string& rendered_template_prompt,
      const nlohmann::ordered_json& messages,
      const DataProcessorArguments& args) = 0;

  // Converts a list of responses from the LLM Session to a Message, which is
  // the output to the user.
  virtual absl::StatusOr<Message> ToMessage(
      const Responses& responses, const DataProcessorArguments& args) = 0;

  // Converts a message into the Jinja template input for that message.
  //
  // Although the message is already a JSON object, some models require
  // additional processing to convert the message into the input needed by the
  // Jinja template.
  //
  // For example, messages represent tool calls as a list of JSON objects, but a
  // model's Jinja template may expect the tool calls to already be formatted
  // in a particular tool calling syntax.
  virtual absl::StatusOr<nlohmann::ordered_json> MessageToTemplateInput(
      const nlohmann::ordered_json& message) const = 0;

  // Formats the provided tools to be inserted into the system/developer
  // instruction of the prompt.
  virtual absl::StatusOr<nlohmann::ordered_json> FormatTools(
      const nlohmann::ordered_json& tools) const = 0;

  // Returns the start of tool call blocks.
  virtual absl::string_view CodeFenceStart() = 0;

  // Returns the end of tool call blocks.
  virtual absl::string_view CodeFenceEnd() = 0;

  std::unique_ptr<Engine::Session> session_;
  PromptTemplate prompt_template_;
  Preface preface_;
  mutable absl::Mutex history_mutex_;
  std::vector<Message> history_ ABSL_GUARDED_BY(history_mutex_);

  absl::StatusOr<std::string> GetSingleTurnText(const Message& message) const;

  explicit ModelDataProcessor(std::unique_ptr<Engine::Session> session,
                              PromptTemplate prompt_template, Preface preface)
      : session_(std::move(session)),
        prompt_template_(std::move(prompt_template)),
        preface_(preface) {};
};

// TypeSafeModelDataProcessor is a ModelDataProcessor that expects a specific
// type of arguments. It guarantees that the model data processor will only be
// called with the expected arguments type.
//
// The model data processor should overwrite the ToInputDataVectorImpl and
// ToMessageImpl to handle the model-specific logic.
template <typename ExpectedConfigT, typename ExpectedArgsT>
class TypeSafeModelDataProcessor : public ModelDataProcessor {
 public:
  // Converts a rendered template prompt and a list of messages to a vector of
  // InputData, with arguments type validated.
  absl::StatusOr<std::vector<InputData>> ToInputDataVector(
      const std::string& rendered_template_prompt,
      const nlohmann::ordered_json& messages,
      const DataProcessorArguments& args) final {
    if (std::holds_alternative<ExpectedArgsT>(args)) {
      return this->ToInputDataVectorImpl(rendered_template_prompt, messages,
                                         std::get<ExpectedArgsT>(args));
    } else if (std::holds_alternative<std::monostate>(args)) {
      return this->ToInputDataVectorImpl(rendered_template_prompt, messages,
                                         ExpectedArgsT{});
    }
    return absl::InvalidArgumentError(
        "DataProcessorArguments does not hold the expected type");
  }

  // Converts a list of responses from the LLM Session to a Message, with
  // arguments type validated.
  absl::StatusOr<Message> ToMessage(const Responses& responses,
                                    const DataProcessorArguments& args) final {
    if (std::holds_alternative<ExpectedArgsT>(args)) {
      return this->ToMessageImpl(responses, std::get<ExpectedArgsT>(args));
    } else if (std::holds_alternative<std::monostate>(args)) {
      return this->ToMessageImpl(responses, ExpectedArgsT{});
    }
    return absl::InvalidArgumentError(
        "DataProcessorArguments does not hold the expected type");
  }

  // Returns the config of the model data processor.
  virtual const ExpectedConfigT& GetConfig() = 0;

  explicit TypeSafeModelDataProcessor(std::unique_ptr<Engine::Session> session,
                                      PromptTemplate prompt_template,
                                      Preface preface)
      : ModelDataProcessor(std::move(session), prompt_template, preface) {};

 private:
  virtual absl::StatusOr<std::vector<InputData>> ToInputDataVectorImpl(
      const std::string& rendered_template_prompt,
      const nlohmann::ordered_json& messages,
      const ExpectedArgsT& typed_args) = 0;

  virtual absl::StatusOr<Message> ToMessageImpl(
      const Responses& responses, const ExpectedArgsT& typed_args) = 0;
};

// An adapter to wrap a `MessageCallbacks` in an `InferenceCallbacks`.
//
// An `InferenceCallbacks` asynchronously receives responses (i.e. tokens)
// from the `Session` as output is generated by the model.
//
// A `MessageCallbacks` asynchronously receives messages from a `Conversation`.
// A message represents a turn or a part of a turn in a conversation between the
// user and the model. A message is composed from one or more tokens and
// contains metadata such as the role (e.g. "user", "assistant") and type (e.g.
// "text", "tool_call") of the message.
class InternalCallbacksAdapter : public InferenceCallbacks {
 public:
  using CompleteMessageCallback = std::function<void(const Message& message)>;

  // Creates an instance of `InternalCallbacksAdapter`.
  //
  // - `model_data_processor` processes the input and output of the model.
  // - `user_callbacks` is the `MessageCallbacks` defined by the user to receive
  //    messages from the `Conversation`.
  // - `processor_args` is the set of arguments to pass to the
  //   `ModelDataProcessor`.
  static std::unique_ptr<InternalCallbacksAdapter> Create(
      ModelDataProcessor* model_data_processor,
      std::unique_ptr<MessageCallbacks> user_callbacks,
      DataProcessorArguments processor_args);

  // Sets a callback to be called with the complete message when inference has
  // finished successfully.
  void SetCompleteMessageCallback(
      CompleteMessageCallback complete_message_callback);

  // Called when a new response is generated.
  void OnNext(const Responses& responses) override;

  // Called when inference has successfully finished.
  void OnDone() override;

  // Called when an error is encountered during inference.
  void OnError(const absl::Status& status) override;

 private:
  explicit InternalCallbacksAdapter(
      ModelDataProcessor* model_data_processor,
      std::unique_ptr<MessageCallbacks> user_callbacks,
      DataProcessorArguments processor_args);

  void SendMessage(absl::string_view text);

  absl::Status ProcessResponseText(absl::string_view response_text);

  ModelDataProcessor* absl_nonnull model_data_processor_;
  std::unique_ptr<MessageCallbacks> user_callbacks_;
  CompleteMessageCallback complete_message_callback_;
  DataProcessorArguments processor_args_;
  std::string accumulated_response_text_;
  size_t cursor_ = 0;
  bool inside_tool_call_ = false;
};

}  // namespace litert::lm

#endif  // THIRD_PARTY_ODML_LITERT_LM_RUNTIME_CONVERSATION_MODEL_DATA_PROCESSOR_MODEL_DATA_PROCESSOR_H_