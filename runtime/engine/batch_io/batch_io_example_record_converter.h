#ifndef THIRD_PARTY_ODML_LITERT_LM_RUNTIME_ENGINE_BATCH_IO_BATCH_IO_EXAMPLE_RECORD_CONVERTER_H_
#define THIRD_PARTY_ODML_LITERT_LM_RUNTIME_ENGINE_BATCH_IO_BATCH_IO_EXAMPLE_RECORD_CONVERTER_H_

#include <string>

#include "absl/status/statusor.h"  // from @com_google_absl
#include "runtime/engine/batch_io/batch_io_record_converter.h"
#include "runtime/engine/io_types.h"

namespace litert {
namespace lm {

// A concrete implementation of `BatchIoRecordConverter` that handles
// encoding/decoding of `Example` and `DecodeExample` protos from the
// `bulk_inference.proto`.
class BatchIoExampleRecordConverter : public BatchIoRecordConverter {
 public:
  // Decodes a `Example` proto in a serialized string into
  // `BatchIoInput`.
  //
  // Args:
  //   input: The serialized input string of the `Example` proto read from the
  //    input source.
  //
  // Returns:
  //   The decoded input data on success, or a non-OK status on error.
  absl::StatusOr<BatchIoInput> DecodeInput(const std::string& input) final;

  // Encodes a response string into a `DecodeExample` proto in a serialized
  // string, together with the corresponding input.
  //
  // Args:
  //   input: The serialized input string of the `Example` proto that
  //     corresponds to the response.
  //   response: The response string to be encoded.
  //
  // Returns:
  //   The encoded `DecodeExample` proto in a serialized string on success, or
  //   a non-OK status on error.
  absl::StatusOr<std::string> EncodeResponse(const std::string& input,
                                             const std::string& response) final;
};

}  // namespace lm
}  // namespace litert

#endif  // THIRD_PARTY_ODML_LITERT_LM_RUNTIME_ENGINE_BATCH_IO_BATCH_IO_EXAMPLE_RECORD_CONVERTER_H_
