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

#include "runtime/engine/model_manager.h"

#include <filesystem>  // NOLINT
#include <fstream>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/status/status.h"  // from @com_google_absl

namespace litert {
namespace lm {
namespace {

using ::testing::HasSubstr;
using ::testing::TempDir;
using ::testing::status::StatusIs;

class ModelManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    home_dir_ = TempDir();
    model_dir_ = std::filesystem::path(home_dir_) / ".litertlm" / "models";
    setenv("HOME", home_dir_.c_str(), 1);
    unsetenv("HUGGING_FACE_HUB_TOKEN");
    std::filesystem::create_directories(model_dir_);
  }

  void TearDown() override {
    unsetenv("HOME");
    if (std::filesystem::exists(model_dir_)) {
      std::filesystem::remove_all(model_dir_);
    }
  }

  void CreateDummyFile(const std::string& filename) {
    std::ofstream ofs((model_dir_ / filename).string());
    ofs << "dummy content";
    ofs.close();
  }

  std::string home_dir_;
  std::filesystem::path model_dir_;
};

TEST_F(ModelManagerTest, ListNoModels) {
  ModelManager model_manager;
  ::testing::internal::CaptureStdout();
  absl::Status result = model_manager.List();
  std::string output = ::testing::internal::GetCapturedStdout();
  EXPECT_OK(result);
  EXPECT_THAT(output, HasSubstr("No models found"));
}

TEST_F(ModelManagerTest, ListWithModels) {
  CreateDummyFile("model1.litertlm");
  CreateDummyFile("model2.litertlm");
  ModelManager model_manager;
  ::testing::internal::CaptureStdout();
  absl::Status result = model_manager.List();
  std::string output = ::testing::internal::GetCapturedStdout();
  EXPECT_OK(result);
  EXPECT_THAT(output, HasSubstr("model1.litertlm"));
  EXPECT_THAT(output, HasSubstr("model2.litertlm"));
}

TEST_F(ModelManagerTest, RemoveNonExistentModel) {
  ModelManager model_manager;
  absl::Status result = model_manager.Remove("nonexistent.litertlm");
  EXPECT_THAT(result,
              StatusIs(absl::StatusCode::kNotFound,
                       HasSubstr("Model 'nonexistent.litertlm' not found.")));
}

TEST_F(ModelManagerTest, RemoveExistingModel) {
  CreateDummyFile("model1.litertlm");
  ModelManager model_manager;
  EXPECT_TRUE(std::filesystem::exists(model_dir_ / "model1.litertlm"));
  absl::Status result = model_manager.Remove("model1.litertlm");
  EXPECT_OK(result);
  EXPECT_FALSE(std::filesystem::exists(model_dir_ / "model1.litertlm"));
}

TEST_F(ModelManagerTest, PullRequiresHfToken) {
  ModelManager model_manager;
  absl::Status result = model_manager.Pull("gemma-3n-E2B");
  EXPECT_THAT(
      result,
      StatusIs(
          absl::StatusCode::kFailedPrecondition,
          HasSubstr("HUGGING_FACE_HUB_TOKEN environment variable not set")));
}

TEST_F(ModelManagerTest, PullInvalidModel) {
  ModelManager model_manager;
  absl::Status result = model_manager.Pull("invalid-model");
  EXPECT_THAT(result,
              StatusIs(absl::StatusCode::kNotFound,
                       HasSubstr("'invalid-model' not found in registry")));
}

TEST_F(ModelManagerTest, RunNonExistentModel) {
  ModelManager model_manager;
  absl::Status result = model_manager.Run("nonexistent.litertlm");
  EXPECT_THAT(result, StatusIs(absl::StatusCode::kNotFound,
                               HasSubstr("not found in the model registry")));
}

TEST_F(ModelManagerTest, RunAmbiguousModel) {
  CreateDummyFile("ambiguous1.litertlm");
  CreateDummyFile("ambiguous2.litertlm");
  ModelManager model_manager;
  absl::Status result = model_manager.Run("ambiguous");
  EXPECT_THAT(result, StatusIs(absl::StatusCode::kInvalidArgument,
                               HasSubstr("Ambiguous model name")));
}

}  // namespace
}  // namespace lm
}  // namespace litert
