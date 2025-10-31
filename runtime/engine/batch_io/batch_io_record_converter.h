#ifndef THIRD_PARTY_ODML_LITERT_LM_RUNTIME_ENGINE_BATCH_IO_BATCH_IO_RECORD_CONVERTER_H_
#define THIRD_PARTY_ODML_LITERT_LM_RUNTIME_ENGINE_BATCH_IO_BATCH_IO_RECORD_CONVERTER_H_

#include <string>

#include "absl/status/statusor.h"  // from @com_google_absl
#include "runtime/engine/io_types.h"

namespace litert {
namespace lm {

// Abstract base class that defines a contract for converting raw records
// from a data source into `BatchIoInput`, and converting LLM responses
// back into a serialized string.
class BatchIoRecordConverter {
 public:
  virtual ~BatchIoRecordConverter() = default;

  // Abstract method to decode a serialized input into `BatchIoInput`.
  //
  // Args:
  //   input: The serialized input string read from the input source.
  //
  // Returns:
  //   The decoded input data on success, or a non-OK status on error.
  virtual absl::StatusOr<BatchIoInput> DecodeInput(
      const std::string& input) = 0;

  // Abstract method to encode a response string into a serialized string,
  // together with the corresponding input.
  //
  // Args:
  //   input: The serialized input string read from the input source that
  //     corresponds to the response.
  //   response: The response string to be encoded.
  //
  // Returns:
  //   The encoded string on success, or a non-OK status on error.
  virtual absl::StatusOr<std::string> EncodeResponse(
      const std::string& input, const std::string& response) = 0;
};

}  // namespace lm
}  // namespace litert

#endif  // THIRD_PARTY_ODML_LITERT_LM_RUNTIME_ENGINE_BATCH_IO_BATCH_IO_RECORD_CONVERTER_H_
