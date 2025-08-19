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

#ifndef THIRD_PARTY_ODML_LITERT_LM_RUNTIME_EXECUTOR_FAKE_LLM_EXECUTOR_H_
#define THIRD_PARTY_ODML_LITERT_LM_RUNTIME_EXECUTOR_FAKE_LLM_EXECUTOR_H_

#include <vector>

#include "absl/status/status.h"  // from @com_google_absl
#include "absl/status/statusor.h"  // from @com_google_absl
#include "absl/strings/string_view.h"  // from @com_google_absl
#include "litert/cc/litert_tensor_buffer.h"  // from @litert
#include "runtime/executor/llm_executor.h"
#include "runtime/executor/llm_executor_io_types.h"
#include "runtime/executor/llm_executor_settings.h"

namespace litert::lm {

// Fake LLM executor for testing.
class FakeLlmExecutor : public LlmExecutor {
 public:
  // Creates a fake LLM executor with the given prefill and decode tokens.
  // - vocab_size: The vocabulary size of the LLM. It is used to determine the
  //   shape of the output logits TensorBuffer.
  // - prefill_tokens_set:The prefill tokens ([num_calls, num_tokens]) are the
  //   tokens that are expected to be passed in at each time. The Prefill
  //   function will only return OkStatus if the input tokens match the expected
  //   tokens.
  // - decode_tokens_set: The decode tokens ([num_calls, batch_size]) are the
  //   tokens that will be returned at each time the Decode function is called.
  // - batch_size: The batch size of the LLM.
  // - secondary_decode_tokens_set: An optional secondary and less-likely decode
  // tokens ([num_calls, batch_size]) that will be used to test scoring.
  FakeLlmExecutor(
      int vocab_size, const std::vector<std::vector<int>>& prefill_tokens_set,
      const std::vector<std::vector<int>>& decode_tokens_set,
      int batch_size = 1,
      const std::vector<std::vector<int>>& secondary_decode_tokens_set = {});

  absl::Status Prefill(const ExecutorInputs& inputs) override;
  absl::Status Prefill(const ExecutorInputs& inputs,
                       const ExecutorPrefillParams& prefill_params) override;

  absl::Status Decode(::litert::TensorBuffer& output_tokens) override;

  absl::Status Decode(const ExecutorInputs& inputs,
                      ::litert::TensorBuffer& output_logits) override;

  absl::StatusOr<::litert::TensorBuffer> DecodeLogits(
      const ExecutorInputs& inputs) override;

  absl::string_view ExecutorBackendName() const override {
    return "FakeLlmExecutorBackend";
  };

  absl::StatusOr<int> GetVocabSize() override { return vocab_size_; }

  absl::StatusOr<LlmExecutorSettings> GetExecutorSettings() const override {
    return executor_settings_;
  };
  absl::StatusOr<LlmExecutorSettings*> GetMutableExecutorSettings() {
    return &executor_settings_;
  };
  absl::StatusOr<int> GetCurrentStep() const override { return current_step_; }

  // This disables the strict input token validation during DecodeLogits, which
  // is necessary for scoring arbitrary target sequences.
  void SetScoringMode(bool scoring_mode) { scoring_mode_ = scoring_mode; }

 private:
  int vocab_size_;
  std::vector<std::vector<int>> prefill_tokens_set_;
  std::vector<std::vector<int>> decode_tokens_set_;
  std::vector<std::vector<int>> secondary_decode_tokens_set_;
  int batch_size_;

  // The number of times the Prefill function has been called.
  int prefill_times_;
  // The number of times the Decode function has been called.
  int decode_times_;

  // The executor settings.
  LlmExecutorSettings executor_settings_;

  // The current step of the executor.
  int current_step_;

  // Whether the executor is in scoring mode.
  bool scoring_mode_ = false;
};

}  // namespace litert::lm

#endif  // THIRD_PARTY_ODML_LITERT_LM_RUNTIME_EXECUTOR_FAKE_LLM_EXECUTOR_H_
