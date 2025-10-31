#include "runtime/engine/batch_io/batch_io_example_record_converter.h"

#include <string>
#include <variant>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/status/status.h"  // from @com_google_absl
#include "absl/status/statusor.h"  // from @com_google_absl
#include "third_party/odml/infra/tools/llm/proto/bulk_inference.pb.h"
#include "runtime/engine/batch_io/batch_io_utils.h"
#include "runtime/engine/io_types.h"

namespace litert {
namespace lm {
namespace {

using ::odml::infra::tools::llm::DecodeExample;
using ::odml::infra::tools::llm::Example;

class BatchIoExampleRecordConverterTest : public ::testing::Test {
 protected:
  BatchIoExampleRecordConverter converter_;
};

TEST_F(BatchIoExampleRecordConverterTest, DecodeInputSuccess) {
  // Prepare the input: a serialized Example proto.
  Example input_proto;
  input_proto.set_inputs("This is the input text.");
  const std::string raw_record = input_proto.SerializeAsString();

  // Call the Decode method.
  const absl::StatusOr<BatchIoInput> result =
      converter_.DecodeInput(raw_record);

  // Verify the result.
  ASSERT_OK(result);
  // Check that the raw input was preserved.
  EXPECT_EQ(result->raw_input, raw_record);
  // Check that the input_data was correctly parsed.
  const auto input_string = GetTextFromBatchIoInput(*result);
  ASSERT_OK(input_string);
  EXPECT_EQ(*input_string, "This is the input text.");
}

TEST_F(BatchIoExampleRecordConverterTest, DecodeInputFailsOnMalformedInput) {
  const std::string malformed_record = "this is not a valid proto";

  absl::StatusOr<BatchIoInput> result =
      converter_.DecodeInput(malformed_record);

  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.status().code(), absl::StatusCode::kInvalidArgument);
}

TEST_F(BatchIoExampleRecordConverterTest, EncodeResponseSuccess) {
  // Prepare the input context and response.
  Example original_proto;
  original_proto.set_inputs("This is the original input.");
  BatchIoInput original_input;
  original_input.raw_input = original_proto.SerializeAsString();
  original_input.input_data.push_back(InputText(original_proto.inputs()));
  const std::string response = "This is the response.";

  // Call the EncodeResponse method.
  absl::StatusOr<std::string> result =
      converter_.EncodeResponse(original_input.raw_input, response);

  // Verify the result.
  ASSERT_OK(result);

  // Parse the output string back into a DecodeExample proto to check its
  // fields.
  DecodeExample output_proto;
  ASSERT_TRUE(output_proto.ParseFromString(*result));

  // Check that the response was set correctly.
  ASSERT_EQ(output_proto.samples_size(), 1);
  EXPECT_EQ(output_proto.samples(0).prediction(), response);

  // Check that the original Example proto was preserved.
  EXPECT_EQ(output_proto.example().inputs(), original_proto.inputs());
  EXPECT_EQ(output_proto.example().index(), original_proto.index());
}

}  // namespace
}  // namespace lm
}  // namespace litert
