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

#include <cstdlib>
#include <filesystem>  // NOLINT
#include <iostream>
#include <string>
#include <system_error>  // NOLINT
#include <vector>

#include "absl/base/no_destructor.h"  // from @com_google_absl
#include "absl/container/flat_hash_map.h"  // from @com_google_absl
#include "absl/status/status.h"  // from @com_google_absl
#include "absl/status/statusor.h"  // from @com_google_absl
#include "absl/strings/match.h"  // from @com_google_absl
#include "absl/strings/str_cat.h"  // from @com_google_absl
#include "absl/strings/str_join.h"  // from @com_google_absl
#include "runtime/engine/litert_lm_lib.h"
#include "runtime/engine/model_downloader.h"

namespace litert {
namespace lm {

namespace {

static const absl::NoDestructor<absl::flat_hash_map<std::string, std::string>>
    kModelRegistry({
        {"gemma-3n-E2B",
         "https://huggingface.co/google/gemma-3n-E2B-it-litert-lm/resolve/main/"
         "gemma-3n-E2B-it-int4.litertlm"},
        {"gemma-3n-E4B",
         "https://huggingface.co/google/gemma-3n-E4B-it-litert-lm/resolve/main/"
         "gemma-3n-E4B-it-int4.litertlm"},
        {"gemma3-1b",
         "https://huggingface.co/litert-community/Gemma3-1B-IT/resolve/main/"
         "gemma3-1b-it-int4.litertlm"},
        {"qwen2.5-1.5b",
         "https://huggingface.co/litert-community/Qwen2.5-1.5B-Instruct/"
         "resolve/"
         "main/Qwen2.5-1.5B-Instruct_multi-prefill-seq_q8_ekv4096.litertlm"},
        {"phi-4-mini",
         "https://huggingface.co/litert-community/Phi-4-mini-instruct/resolve/"
         "main/"
         "Phi-4-mini-instruct_multi-prefill-seq_q8_ekv4096.litertlm"},
    });

// Extracts the filename from a URL path.
std::string GetFilenameFromUrl(const std::string& url) {
  size_t last_slash_pos = url.find_last_of('/');
  if (last_slash_pos != std::string::npos) {
    return url.substr(last_slash_pos + 1);
  }
  return url;
}

std::vector<std::string> ListModelsInDir(
    const std::filesystem::path& model_dir) {
  std::vector<std::string> models;
  if (!std::filesystem::exists(model_dir) ||
      !std::filesystem::is_directory(model_dir)) {
    return models;
  }
  for (const auto& entry : std::filesystem::directory_iterator(model_dir)) {
    if (entry.is_regular_file()) {
      models.push_back(entry.path().filename().string());
    }
  }
  return models;
}

// Returns:
// 0: no match
// 1: unique match in unique_match_out
// >1: ambiguous, error message printed to stderr
absl::StatusOr<int> FindMatchesAndHandleAmbiguity(
    const std::string& user_input, const std::vector<std::string>& candidates,
    const std::string& context_for_error, std::string* unique_match_out) {
  std::vector<std::string> matches;
  for (const auto& candidate : candidates) {
    if (candidate.find(user_input) != std::string::npos) {
      matches.push_back(candidate);
    }
  }

  if (matches.size() == 1) {
    *unique_match_out = matches[0];
    return 1;
  } else if (matches.size() > 1) {
    std::string error_context_str =
        context_for_error.empty() ? "" : " in " + context_for_error;
    return absl::InvalidArgumentError(absl::StrCat(
        "Ambiguous model name '", user_input, "'", error_context_str,
        ".\n\nPlease be more specific. It could refer to any of "
        "the following:\n  - ",
        absl::StrJoin(matches, "\n  - ")));
  }
  return 0;
}

// Holds the result of resolving a model for pulling.
struct PullResolutionResult {
  std::string download_url;
  std::string file_name;
};

// Resolves a model name or URL for pulling.
absl::StatusOr<PullResolutionResult> ResolveModelForPull(
    const std::string& model_or_url) {
  auto it = kModelRegistry->find(model_or_url);
  if (it != kModelRegistry->end()) {
    std::cout << "Found '" << model_or_url << "' in registry." << std::endl;
    return PullResolutionResult{it->second, GetFilenameFromUrl(it->second)};
  }
  if (absl::StartsWith(model_or_url, "http://") ||
      absl::StartsWith(model_or_url, "https://")) {
    return PullResolutionResult{model_or_url, GetFilenameFromUrl(model_or_url)};
  }
  return absl::NotFoundError(
      absl::StrCat("'", model_or_url,
                   "' not found in registry. If providing a URL, it must start "
                   "with http:// or https://"));
}

// Holds the result of resolving a model for running.
struct RunResolutionResult {
  std::string model_name;
  std::string file_name;
  bool is_registry_alias;
};

// Resolves a user input to a model for running, checking local files,
// registry, and partial matches.
absl::StatusOr<RunResolutionResult> ResolveModelForRun(
    const std::string& user_input, const std::filesystem::path& model_dir) {
  std::string model_name;
  std::string file_name;
  bool is_registry_alias = false;

  // 1. check if user_input is a filename in modelDir
  std::filesystem::path potential_file = model_dir / user_input;
  if (std::filesystem::exists(potential_file) &&
      std::filesystem::is_regular_file(potential_file)) {
    model_name = user_input;
    file_name = user_input;
  } else {
    // 2. If not a file, check for an exact match in model registry
    auto it = kModelRegistry->find(user_input);
    if (it != kModelRegistry->end()) {
      model_name = user_input;
      is_registry_alias = true;
    } else {
      // 3. If no exact match, search for a unique partial match in registry.
      std::vector<std::string> registry_candidates;
      for (const auto& pair : *kModelRegistry) {
        registry_candidates.push_back(pair.first);
      }
      std::string unique_match;
      absl::StatusOr<int> match_count = FindMatchesAndHandleAmbiguity(
          user_input, registry_candidates, "", &unique_match);
      if (!match_count.ok()) {
        return match_count.status();
      }
      if (*match_count == 1) {
        model_name = unique_match;
        is_registry_alias = true;
        std::cout << "Assuming you meant '" << model_name << "'" << std::endl;
      }
    }
  }

  // 4. After checking the registry, check the local cache using the same
  // logic.
  if (model_name.empty()) {
    std::vector<std::string> local_models = ListModelsInDir(model_dir);
    std::string unique_match;
    absl::StatusOr<int> match_count = FindMatchesAndHandleAmbiguity(
        user_input, local_models, "local cache", &unique_match);
    if (!match_count.ok()) {
      return match_count.status();
    }
    if (*match_count == 1) {
      model_name = unique_match;
      file_name = unique_match;
      is_registry_alias = false;  // It's a file from local cache.
      std::cout << "Assuming you meant '" << model_name
                << "' from your local cache" << std::endl;
    }
  }

  if (model_name.empty()) {
    return absl::NotFoundError(
        absl::StrCat("Model '", user_input,
                     "' not found in the model registry or the local cache"));
  }

  if (is_registry_alias) {
    file_name = GetFilenameFromUrl(kModelRegistry->at(model_name));
  }

  return RunResolutionResult{model_name, file_name, is_registry_alias};
}

// Returns a singleton ModelDownloader instance. Constructor and destructor
// are updating the global state, so we need to have only one instance.
ModelDownloader& GetModelDownloader() {
  static absl::NoDestructor<ModelDownloader> downloader;
  return *downloader;
}

}  // namespace

ModelManager::ModelManager() {
  const char* home_dir_cstr = std::getenv("HOME");
  // If HOME is not set, try USERPROFILE. This is useful for Windows.
  if (home_dir_cstr == nullptr) {
    home_dir_cstr = std::getenv("USERPROFILE");
  }
  if (home_dir_cstr != nullptr) {
    model_dir_ = std::filesystem::path(home_dir_cstr) / ".litertlm" / "models";
  }
}

absl::Status ModelManager::Pull(const std::string& model_or_url) {
  std::cout << "Pulling model..." << std::endl;

  // Resolve the model name and file name.
  absl::StatusOr<PullResolutionResult> resolution =
      ResolveModelForPull(model_or_url);
  if (!resolution.ok()) {
    return resolution.status();
  }
  const auto& [downloadURL, modelName] = *resolution;

  // Get the singleton downloader instance.
  // Its constructor calls curl_global_init() on first access.
  ModelDownloader& downloader = GetModelDownloader();

  if (model_dir_.empty()) {
    return absl::InternalError(
        "Could not find HOME or USERPROFILE environment variable.");
  }

  // Create directories if they don't exist
  std::error_code error;
  std::filesystem::create_directories(model_dir_, error);
  if (error) {
    return absl::InternalError(
        absl::StrCat("Error creating directory ", model_dir_.string()));
  }

  std::string output_filename = (model_dir_ / modelName).string();

  // Get Hugging Face token from environment variable
  const char* hf_token_cstr = std::getenv("HUGGING_FACE_HUB_TOKEN");
  if (hf_token_cstr == nullptr) {
    return absl::FailedPreconditionError(
        "HUGGING_FACE_HUB_TOKEN environment variable not set.");
  }
  std::string my_hf_token(hf_token_cstr);

  // --- Execute Download ---
  absl::Status status =
      downloader.DownloadFile(downloadURL, output_filename, my_hf_token);

  if (!status.ok()) {
    return status;
  }
  std::cout << "File successfully downloaded to: " << output_filename
            << std::endl;

  return absl::OkStatus();
}

absl::Status ModelManager::Run(const std::string& user_input) {
  std::cout << "Running model..." << std::endl;

  if (model_dir_.empty()) {
    return absl::InternalError(
        "Could not find HOME or USERPROFILE environment variable.");
  }

  // Resolve the model name and file name.
  absl::StatusOr<RunResolutionResult> resolution =
      ResolveModelForRun(user_input, model_dir_);
  if (!resolution.ok()) {
    return resolution.status();
  }
  const auto& [model_name, file_name, is_registry_alias] = *resolution;

  std::string model_path = (model_dir_ / file_name).string();

  // Check if model file exists
  if (!std::filesystem::exists(model_path)) {
    std::string pull_hint = model_name;
    if (!is_registry_alias) {
      for (const auto& pair : *kModelRegistry) {
        if (GetFilenameFromUrl(pair.second) == file_name) {
          pull_hint = pair.first;
          break;
        }
      }
    }
    return absl::NotFoundError(
        absl::StrCat("Model '", model_name,
                     "' not found in local cache. Please run 'lit pull ",
                     pull_hint, "' first"));
  }

  // If model path is resolved, create settings and call RunLiteRtLm.
  litert::lm::LiteRtLmSettings settings;
  settings.model_path = model_path;
  settings.multi_turns = true;
  settings.backend = "cpu";

  std::cout << "Loading model '" << model_name << "'..." << std::endl;

  absl::Status status = litert::lm::RunLiteRtLm(settings);

  if (!status.ok()) {
    return status;
  }

  return absl::OkStatus();
}

absl::Status ModelManager::List() {
  std::cout << "Listing models..." << std::endl;
  if (model_dir_.empty()) {
    return absl::InternalError(
        "Could not find HOME or USERPROFILE environment variable.");
  }
  std::vector<std::string> local_models = ListModelsInDir(model_dir_);
  if (local_models.empty()) {
    std::cout << "No models found in " << model_dir_ << std::endl;
  } else {
    for (const auto& model : local_models) {
      std::cout << model << std::endl;
    }
  }
  return absl::OkStatus();
}

absl::Status ModelManager::Remove(const std::string& model_name_to_remove) {
  std::cout << "Removing model..." << std::endl;
  if (model_dir_.empty()) {
    return absl::InternalError(
        "Could not find HOME or USERPROFILE environment variable.");
  }
  std::filesystem::path model_path = model_dir_ / model_name_to_remove;
  if (!std::filesystem::exists(model_path)) {
    return absl::NotFoundError(
        absl::StrCat("Model '", model_name_to_remove, "' not found."));
  }
  std::error_code ec;
  std::filesystem::remove(model_path, ec);
  if (ec) {
    return absl::InternalError(
        absl::StrCat("Error removing model '", model_name_to_remove, "'"));
  }
  std::cout << "Removed model '" << model_name_to_remove << "'." << std::endl;
  return absl::OkStatus();
}

}  // namespace lm
}  // namespace litert
