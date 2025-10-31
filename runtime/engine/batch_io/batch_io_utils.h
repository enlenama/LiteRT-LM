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

#ifndef THIRD_PARTY_ODML_LITERT_LM_RUNTIME_ENGINE_BATCH_IO_BATCH_IO_UTILS_H_
#define THIRD_PARTY_ODML_LITERT_LM_RUNTIME_ENGINE_BATCH_IO_BATCH_IO_UTILS_H_

#include <string>

#include "absl/status/statusor.h"  // from @com_google_absl
#include "runtime/engine/io_types.h"

namespace litert {
namespace lm {

// Returns the text in std::string from `batch_io_input`'s first input data
// element if it is an input text.
// Returns an error if the input data is empty or if the input data type is not
// supported.
absl::StatusOr<std::string> GetTextFromBatchIoInput(
    const BatchIoInput& batch_io_input);

}  // namespace lm
}  // namespace litert

#endif  // THIRD_PARTY_ODML_LITERT_LM_RUNTIME_ENGINE_BATCH_IO_BATCH_IO_UTILS_H_
