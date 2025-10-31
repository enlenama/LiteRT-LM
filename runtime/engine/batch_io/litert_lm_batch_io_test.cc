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

#include "runtime/engine/batch_io/litert_lm_batch_io.h"

#include <optional>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/status/status.h"  // from @com_google_absl
#include "absl/status/statusor.h"  // from @com_google_absl
#include "absl/strings/str_cat.h"  // from @com_google_absl
#include "absl/strings/string_view.h"  // from @com_google_absl
#include "runtime/engine/io_types.h"
#include "runtime/engine/batch_io/batch_io_utils.h"
#include "runtime/engine/batch_io/test_utils.h"
#include "runtime/util/status_macros.h"  //NOLINT
#include "runtime/util/test_utils.h"     //NOLINT

namespace litert {
namespace lm {
namespace {

using ::testing::Return;

// A mock implementation of the LiteRtLmBatchIo interface for testing purposes.
class MockLiteRtLmBatchIo : public LiteRtLmBatchIo {
 public:
  MockLiteRtLmBatchIo() : LiteRtLmBatchIo(nullptr) {}
  MOCK_METHOD(absl::StatusOr<std::optional<BatchIoInput>>, NextInput, (),
              (override));
  MOCK_METHOD(absl::Status, WriteResponse,
              (const std::string&, const std::string&), (override));
  MOCK_METHOD(absl::Status, Close, (), (override));
};

class LiteRtLmBatchIoTest : public ::testing::Test {
 protected:
  MockLiteRtLmBatchIo mock_batch_io_;
};

// A simple function to demonstrate how the LiteRtLmBatchIo interface would be
// used in a real batch processing loop.
absl::Status ProcessAllInputs(LiteRtLmBatchIo* batch_io) {
  while (true) {
    ASSIGN_OR_RETURN(const std::optional<BatchIoInput> input_or,
                     batch_io->NextInput());
    if (!input_or.has_value()) {
      // End of file reached.
      break;
    }
    ASSIGN_OR_RETURN(const absl::string_view input_string,
                     GetTextFromBatchIoInput(*input_or));
    // In a real scenario, we would process the input and generate a response.
    const std::string response = absl::StrCat("processed_", input_string);
    const std::string input = absl::StrCat("input_", input_string);
    absl::Status write_status = batch_io->WriteResponse(input, response);
    if (!write_status.ok()) {
      return write_status;
    }
  }
  // Close the batch IO after processing all inputs.
  return batch_io->Close();
}

TEST_F(LiteRtLmBatchIoTest, ProcessAllInputsSuccess) {
  EXPECT_CALL(mock_batch_io_, NextInput())
      .WillOnce(Return(CreateBatchIoInput("input1")))
      .WillOnce(Return(CreateBatchIoInput("input2")))
      .WillOnce(Return(std::nullopt));  // Signal end of file

  EXPECT_CALL(mock_batch_io_, WriteResponse("input_input1", "processed_input1"))
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_batch_io_, WriteResponse("input_input2", "processed_input2"))
      .WillOnce(Return(absl::OkStatus()));

  EXPECT_CALL(mock_batch_io_, Close()).WillOnce(Return(absl::OkStatus()));

  EXPECT_OK(ProcessAllInputs(&mock_batch_io_));
}

TEST_F(LiteRtLmBatchIoTest, NextInputReturnsError) {
  const absl::Status kReadError =
      absl::InternalError("Failed to read from file");
  EXPECT_CALL(mock_batch_io_, NextInput()).WillOnce(Return(kReadError));

  absl::Status status = ProcessAllInputs(&mock_batch_io_);
  EXPECT_FALSE(status.ok());
  EXPECT_EQ(status, kReadError);
}

TEST_F(LiteRtLmBatchIoTest, WriteResponseReturnsError) {
  const absl::Status kWriteError =
      absl::InternalError("Failed to write to file");
  EXPECT_CALL(mock_batch_io_, NextInput())
      .WillOnce(Return(CreateBatchIoInput("input1")));
  EXPECT_CALL(mock_batch_io_, WriteResponse("input_input1", "processed_input1"))
      .WillOnce(Return(kWriteError));

  absl::Status status = ProcessAllInputs(&mock_batch_io_);
  EXPECT_FALSE(status.ok());
  EXPECT_EQ(status, kWriteError);
}

TEST_F(LiteRtLmBatchIoTest, NoInputs) {
  EXPECT_CALL(mock_batch_io_, NextInput()).WillOnce(Return(std::nullopt));
  EXPECT_OK(ProcessAllInputs(&mock_batch_io_));
}

}  // namespace
}  // namespace lm
}  // namespace litert
