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

#include "runtime/engine/batch_io/batch_io_utils.h"

#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/status/status.h"  // from @com_google_absl
#include "absl/status/statusor.h"  // from @com_google_absl
#include "runtime/engine/io_types.h"
#include "runtime/util/test_utils.h"  //NOLINT

namespace litert {
namespace lm {
namespace {

class BatchIoUtilsTest : public ::testing::Test {};

TEST_F(BatchIoUtilsTest, GetTextFromBatchIoInputSuccess) {
  BatchIoInput batch_io_input;
  batch_io_input.input_data.emplace_back(InputText("input"));

  absl::StatusOr<std::string> result = GetTextFromBatchIoInput(batch_io_input);
  ASSERT_OK(result);
  EXPECT_EQ(*result, "input");
}

TEST_F(BatchIoUtilsTest, GetTextFromBatchIoInputEmptyFails) {
  // Empty input_data vector.
  const BatchIoInput batch_io_input;

  absl::StatusOr<std::string> result = GetTextFromBatchIoInput(batch_io_input);
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.status().code(), absl::StatusCode::kFailedPrecondition);
  EXPECT_EQ(result.status().message(),
            "Expected at least one input data, but got 0.");
}

TEST_F(BatchIoUtilsTest, GetTextFromBatchIoInputUnsupportedTypeFails) {
  BatchIoInput batch_io_input;
  // Use InputImage as the unsupported type.
  batch_io_input.input_data.emplace_back(InputImage(""));

  absl::StatusOr<std::string> result = GetTextFromBatchIoInput(batch_io_input);
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.status().code(), absl::StatusCode::kInvalidArgument);
  EXPECT_EQ(result.status().message(), "Unsupported input data type.");
}

TEST_F(BatchIoUtilsTest, GetTextFromBatchIoInputUsesFirstElement) {
  BatchIoInput batch_io_input;
  batch_io_input.input_data.emplace_back(InputText("first element"));
  batch_io_input.input_data.emplace_back(InputText("second element"));
  batch_io_input.input_data.emplace_back(InputImage(""));

  // The function should only look at the first element and succeed.
  absl::StatusOr<std::string> result = GetTextFromBatchIoInput(batch_io_input);
  ASSERT_OK(result);
  EXPECT_EQ(*result, "first element");
}

}  // namespace
}  // namespace lm
}  // namespace litert
