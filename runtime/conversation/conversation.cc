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

#include "runtime/conversation/conversation.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "absl/log/absl_log.h"  // from @com_google_absl
#include "absl/memory/memory.h"  // from @com_google_absl
#include "absl/status/status.h"  // from @com_google_absl
#include "absl/status/statusor.h"  // from @com_google_absl
#include "absl/strings/str_cat.h"  // from @com_google_absl
#include "absl/strings/string_view.h"  // from @com_google_absl
#include "absl/synchronization/mutex.h"  // from @com_google_absl
#include "nlohmann/json.hpp"  // from @nlohmann_json
#include "runtime/components/prompt_template.h"
#include "runtime/conversation/io_types.h"
#include "runtime/conversation/model_data_processor/config_registry.h"
#include "runtime/conversation/model_data_processor/model_data_processor.h"
#include "runtime/conversation/model_data_processor/model_data_processor_factory.h"
#include "runtime/engine/engine.h"
#include "runtime/engine/io_types.h"
#include "runtime/proto/llm_model_type.pb.h"
#include "runtime/util/status_macros.h"

namespace litert::lm {

absl::StatusOr<std::unique_ptr<Conversation>> Conversation::Create(
    std::unique_ptr<Engine::Session> session, std::optional<Preface> preface,
    std::optional<PromptTemplate> prompt_template,
    std::optional<DataProcessorConfig> processor_config) {
  if (!preface.has_value()) {
    preface = JsonPreface();
  }

  if (!prompt_template.has_value()) {
    // Get template from the session or model file when the template is not
    // provided by the user.
    ABSL_LOG(INFO)
        << "Prompt template is not provided, using default template.";
    prompt_template =
        PromptTemplate(session->GetSessionConfig().GetJinjaPromptTemplate());
  }

  const proto::LlmModelType& llm_model_type =
      session->GetSessionConfig().GetLlmModelType();
  processor_config = processor_config.value_or(std::monostate());
  ASSIGN_OR_RETURN(
      std::unique_ptr<ModelDataProcessor> model_data_processor,
      CreateModelDataProcessor(llm_model_type, std::move(session),
                               *prompt_template, *preface, *processor_config));
  auto conversation =
      absl::WrapUnique(new Conversation(std::move(model_data_processor)));
  return conversation;
}

absl::StatusOr<Message> Conversation::SendMessage(
    const Message& message, std::optional<DataProcessorArguments> args) {
  return model_data_processor_->SendMessage(message, args);
}

absl::Status Conversation::SendMessageStream(
    const Message& message, std::unique_ptr<MessageCallbacks> callbacks,
    std::optional<DataProcessorArguments> args) {
  return model_data_processor_->SendMessageStream(message, std::move(callbacks),
                                                  args);
};

}  // namespace litert::lm
