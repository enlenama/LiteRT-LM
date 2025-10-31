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

#ifndef THIRD_PARTY_ODML_LITERT_LM_RUNTIME_ENGINE_BATCH_IO_LITERT_LM_BATCH_IO_H_
#define THIRD_PARTY_ODML_LITERT_LM_RUNTIME_ENGINE_BATCH_IO_LITERT_LM_BATCH_IO_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "absl/status/status.h"  // from @com_google_absl
#include "absl/status/statusor.h"  // from @com_google_absl
#include "runtime/engine/batch_io/batch_io_record_converter.h"
#include "runtime/engine/io_types.h"

namespace litert {
namespace lm {

// Abstract base class for handling batch I/O for the LiteRT LM.
//
// This class defines a contract for reading a sequence of inputs and writing a
// sequence of outputs, which is ideal for batch processing scenarios.
class LiteRtLmBatchIo {
 public:
  explicit LiteRtLmBatchIo(
      std::unique_ptr<BatchIoRecordConverter> record_converter)
      : record_converter_(std::move(record_converter)) {}

  virtual ~LiteRtLmBatchIo() = default;

  // Returns the next input record as `BatchIoInput`.
  // This function acts as an iterator. It's called repeatedly to get the next
  // input.
  //
  // Returns:
  //   - A `BatchIoInput` on success.
  //   - `std::nullopt` if the end of the input sequence is reached.
  //   - A non-OK status on read errors.
  virtual absl::StatusOr<std::optional<BatchIoInput>> NextInput() = 0;

  // Writes a single output response into the output source.
  //
  // Args:
  //   input: The serialized input string that corresponds to the response.
  //   response: The response string to be written.
  //
  // Returns:
  //   - An OK status on success.
  //   - A non-OK status on write errors.
  virtual absl::Status WriteResponse(const std::string& input,
                                     const std::string& response) = 0;

  // Closes the underlying input and output sources.
  // Must be called after all I/O is complete to ensure data is flushed
  // and to check for errors.
  virtual absl::Status Close() = 0;

 protected:
  // Protected accessor to use the record converter.
  BatchIoRecordConverter* record_converter() const {
    return record_converter_.get();
  }

 private:
  // The record converter to use for decoding and encoding records.
  std::unique_ptr<BatchIoRecordConverter> record_converter_;
};

}  // namespace lm
}  // namespace litert

#endif  // THIRD_PARTY_ODML_LITERT_LM_RUNTIME_ENGINE_BATCH_IO_LITERT_LM_BATCH_IO_H_
