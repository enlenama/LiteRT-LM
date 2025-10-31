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

#ifndef THIRD_PARTY_ODML_LITERT_LM_RUNTIME_ENGINE_BATCH_IO_TEST_UTILS_H_
#define THIRD_PARTY_ODML_LITERT_LM_RUNTIME_ENGINE_BATCH_IO_TEST_UTILS_H_

#include <string>

#include "runtime/engine/batch_io/litert_lm_batch_io.h"

namespace litert {
namespace lm {

// Creates a `BatchIoInput` with a single input text element.
BatchIoInput CreateBatchIoInput(const std::string& input_string);

}  // namespace lm
}  // namespace litert

#endif  // THIRD_PARTY_ODML_LITERT_LM_RUNTIME_ENGINE_BATCH_IO_TEST_UTILS_H_
