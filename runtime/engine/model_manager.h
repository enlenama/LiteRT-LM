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

#ifndef THIRD_PARTY_ODML_LITERT_LM_RUNTIME_ENGINE_MODEL_MANAGER_H_
#define THIRD_PARTY_ODML_LITERT_LM_RUNTIME_ENGINE_MODEL_MANAGER_H_

#include <filesystem>  // NOLINT
#include <string>

#include "absl/status/status.h"  // from @com_google_absl

namespace litert {
namespace lm {

class ModelManager {
 public:
  ModelManager();

  // Downloads a model from a URL or registry alias.
  absl::Status Pull(const std::string& model_or_url);
  // Runs a model from the local cache.
  absl::Status Run(const std::string& user_input);
  // Lists models in the local cache.
  absl::Status List();
  // Removes a model from the local cache.
  absl::Status Remove(const std::string& model_name);

 private:
  std::filesystem::path model_dir_;
};

}  // namespace lm
}  // namespace litert

#endif  // THIRD_PARTY_ODML_LITERT_LM_RUNTIME_ENGINE_MODEL_MANAGER_H_
