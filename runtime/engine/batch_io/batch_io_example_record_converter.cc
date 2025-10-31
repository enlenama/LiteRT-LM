#include "runtime/engine/batch_io/batch_io_example_record_converter.h"

#include <string>
#include <vector>

#include "absl/status/status.h"  // from @com_google_absl
#include "absl/status/statusor.h"  // from @com_google_absl
#include "absl/strings/string_view.h"  // from @com_google_absl
#include "third_party/odml/infra/tools/llm/proto/bulk_inference.pb.h"
#include "runtime/engine/io_types.h"

namespace litert {
namespace lm {

absl::StatusOr<BatchIoInput> BatchIoExampleRecordConverter::DecodeInput(
    const std::string& input) {
  odml::infra::tools::llm::Example example;
  const absl::string_view input_view(input);
  if (!example.ParseFromArray(static_cast<const void*>(input_view.data()),
                              input_view.size())) {
    return absl::InvalidArgumentError("Failed to parse Example proto.");
  }
  BatchIoInput batch_io_input;
  // Store the raw input string for later use.
  batch_io_input.raw_input = input;
  // Write the input string to the first input data element as an input text.
  batch_io_input.input_data.push_back(InputText(example.inputs()));
  // TODO(b/453071109): Add support for multi-modal inputs.
  return batch_io_input;
}

absl::StatusOr<std::string> BatchIoExampleRecordConverter::EncodeResponse(
    const std::string& input, const std::string& response) {
  odml::infra::tools::llm::DecodeExample decode_example;
  const absl::string_view input_view(input);
  // Parse the input record into the `example` field of the DecodeExample proto.
  if (!decode_example.mutable_example()->ParseFromArray(
          static_cast<const void*>(input_view.data()), input_view.size())) {
    return absl::InvalidArgumentError(
        "Failed to parse Example proto from record.");
  }
  // The response is stored in the `prediction` field of the first sample.
  decode_example.add_samples()->set_prediction(response);
  std::string encoded_response;
  if (!decode_example.SerializeToString(&encoded_response)) {
    return absl::InternalError("Failed to serialize DecodeExample proto.");
  }
  return encoded_response;
}

}  // namespace lm
}  // namespace litert
