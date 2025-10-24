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

#ifndef THIRD_PARTY_ODML_LITERT_LM_RUNTIME_ENGINE_MODEL_DOWNLOADER_H_
#define THIRD_PARTY_ODML_LITERT_LM_RUNTIME_ENGINE_MODEL_DOWNLOADER_H_

// A simple C++ class to download models using libcurl.
// It manages the libcurl global state using RAII.

#include <cstddef>
#include <string>

#include "absl/status/status.h"  // from @com_google_absl

class ModelDownloader {
 public:
  // Constructor that initializes libcurl.
  ModelDownloader();

  // Destructor that cleans up libcurl.
  ~ModelDownloader();

  /**
   * Downloads a file from a given URL to a specified output path.
   *
   * @param url The full URL of the file to download.
   * @param output_path The local file path to save the downloaded content.
   * @param hf_token The Hugging Face (or other) Bearer token for
   * authentication.
   * @return absl::OkStatus() if the download was successful, an error status
   * otherwise.
   */
  absl::Status DownloadFile(const std::string& url,
                            const std::string& output_path,
                            const std::string& hf_token);

 private:
  /**
   * Callback function for libcurl to write received data.
   * This function is called by libcurl as soon as data is received.
   * It writes the data chunk into the file pointed to by 'userdata'.
   *
   * @param ptr Pointer to the data chunk.
   * @param size Size of one data item.
   * @param item_count Number of data items.
   * @param userdata A void pointer, which we cast to (std::ofstream*) to write
   * to.
   * @return The number of bytes successfully written.
   */
  static size_t WriteDataCallback(void* ptr, size_t size, size_t item_count,
                                  void* userdata);
};

#endif  // THIRD_PARTY_ODML_LITERT_LM_RUNTIME_ENGINE_MODEL_DOWNLOADER_H_
