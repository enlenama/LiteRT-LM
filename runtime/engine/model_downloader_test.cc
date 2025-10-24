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

#include "runtime/engine/model_downloader.h"

#include <filesystem>  // NOLINT
#include <string>

#include "net/util/ports.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/status/status.h"  // from @com_google_absl
#include "absl/strings/str_cat.h"  // from @com_google_absl

namespace litert {
namespace lm {
namespace {

using ::testing::HasSubstr;
using ::testing::TempDir;
using ::testing::status::StatusIs;

class ModelDownloaderTest : public ::testing::Test {
 protected:
  void SetUp() override { temp_dir_ = TempDir(); }

  void TearDown() override {}

  std::string temp_dir_;
};

TEST_F(ModelDownloaderTest, DownloadFileFailsToOpenFile) {
  ModelDownloader downloader;
  // Attempt to write to a path where ofstream will fail.
  absl::Status result = downloader.DownloadFile("http://localhost:1/dummy",
                                                "/proc/dummy_file", "");
  EXPECT_THAT(result, StatusIs(absl::StatusCode::kInternal,
                               HasSubstr("Failed to open output file")));
}

TEST_F(ModelDownloaderTest, DownloadFileFailsConnection) {
  ModelDownloader downloader;
  // Attempt to connect to a port that is guaranteed to be unused.
  std::string url =
      absl::StrCat("http://localhost:", net_util::PickUnusedPortOrDie());
  absl::Status result = downloader.DownloadFile(
      url, (std::filesystem::path(temp_dir_) / "output.txt").string(), "");
  EXPECT_THAT(result, StatusIs(absl::StatusCode::kInternal,
                               HasSubstr("curl_easy_perform() failed")));
}

}  // namespace
}  // namespace lm
}  // namespace litert
