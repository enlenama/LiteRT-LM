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

// Implementation of the ModelDownloader class.

#include "runtime/engine/model_downloader.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "absl/status/status.h"  // from @com_google_absl
#include "absl/strings/str_cat.h"  // from @com_google_absl
#include "third_party/curl/curl.h"
#include "third_party/curl/easy.h"

namespace {
// Custom deleters for CURL handles to use with std::unique_ptr for RAII.
struct CurlEasyCleanup {
  void operator()(CURL* handle) const {
    if (handle) {
      curl_easy_cleanup(handle);
    }
  }
};
struct CurlSlistFreeAll {
  void operator()(struct curl_slist* list) const {
    if (list) {
      curl_slist_free_all(list);
    }
  }
};

using CurlEasyHandle = std::unique_ptr<CURL, CurlEasyCleanup>;
using CurlSlist = std::unique_ptr<struct curl_slist, CurlSlistFreeAll>;
}  // namespace

ModelDownloader::ModelDownloader() {
  // Initialize the libcurl global state
  curl_global_init(CURL_GLOBAL_ALL);
}

ModelDownloader::~ModelDownloader() {
  // Clean up the libcurl global state
  curl_global_cleanup();
}

size_t ModelDownloader::WriteDataCallback(void* ptr, size_t size,
                                          size_t item_count, void* userdata) {
  // Cast the userdata pointer back to the std::ofstream* we passed in
  std::ofstream* file = static_cast<std::ofstream*>(userdata);
  size_t realsize = size * item_count;

  // Write the data from the buffer (ptr) to the file
  file->write(static_cast<const char*>(ptr), realsize);

  if (file->fail()) {
    return 0;
  }

  // Return the number of bytes actually written
  return realsize;
}

// Returns true if the download was successful, false otherwise.
absl::Status ModelDownloader::DownloadFile(const std::string& url,
                                           const std::string& output_path,
                                           const std::string& hf_token) {
  CurlEasyHandle curl_handle(curl_easy_init());
  if (!curl_handle) {
    return absl::InternalError("Failed to initialize curl easy handle.");
  }
  curl_easy_setopt(curl_handle.get(), CURLOPT_PROTOCOLS,
                   CURLPROTO_HTTP | CURLPROTO_HTTPS);

  // Open the output file in binary write mode
  std::ofstream output_file(output_path, std::ios::binary);
  if (!output_file.is_open()) {
    return absl::InternalError(
        absl::StrCat("Failed to open output file: ", output_path));
  }

  // --- Prepare Authentication Header ---
  CurlSlist headers(nullptr);
  if (!hf_token.empty()) {
    headers.reset(curl_slist_append(
        nullptr, absl::StrCat("Authorization: Bearer ", hf_token).c_str()));
  }

  // --- Set curl Options ---
  char errbuf[CURL_ERROR_SIZE] = {0};

  // 1. Set the URL to download
  curl_easy_setopt(curl_handle.get(), CURLOPT_URL, url.c_str());

  // 2. Set the custom HTTP headers (for authentication)
  if (headers) {
    curl_easy_setopt(curl_handle.get(), CURLOPT_HTTPHEADER, headers.get());
  }

  // 3. Set the callback function to write the data
  curl_easy_setopt(curl_handle.get(), CURLOPT_WRITEFUNCTION, WriteDataCallback);

  // 4. Pass our FILE* handle to the callback function
  curl_easy_setopt(curl_handle.get(), CURLOPT_WRITEDATA, &output_file);

  // 5. Follow HTTP redirects (Hugging Face /resolve/ uses this)
  curl_easy_setopt(curl_handle.get(), CURLOPT_FOLLOWLOCATION, 1L);

  // 6. Provide a buffer for human-readable error messages
  curl_easy_setopt(curl_handle.get(), CURLOPT_ERRORBUFFER, errbuf);

  // 7. Enable progress meter
  curl_easy_setopt(curl_handle.get(), CURLOPT_NOPROGRESS, 0L);

  // 8. In case the CURL_CA_BUNDLE environment variable is set, use it. This is a
  // path to a custom CA bundle file that will be used by libcurl to verify the
  // server's certificate.
  const char* ca_env = getenv("CURL_CA_BUNDLE");
  if (ca_env && strlen(ca_env) > 0) {
    curl_easy_setopt(curl_handle.get(), CURLOPT_CAINFO, ca_env);
    std::cerr << "Using CA bundle from CURL_CA_BUNDLE: " << ca_env << std::endl;
  } else {
    std::cerr
        << "CURL_CA_BUNDLE not set. Relying on libcurl's default CA searching."
        << std::endl;
  }
  // Ensure VERIFYPEER/HOST is always ON
  curl_easy_setopt(curl_handle.get(), CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(curl_handle.get(), CURLOPT_SSL_VERIFYHOST, 2L);

  std::cout << "Starting download from: " << url << std::endl;
  std::cout << "Saving to: " << output_path << std::endl;

  // --- Perform the Download ---
  CURLcode res = curl_easy_perform(curl_handle.get());

  // --- Cleanup ---
  output_file.close();

  // --- Check for Errors ---
  if (res != CURLE_OK) {
    // Delete the potentially incomplete/corrupt file
    std::remove(output_path.c_str());
    std::string error_message =
        absl::StrCat("curl_easy_perform() failed: ", curl_easy_strerror(res));
    if (errbuf[0]) {
      absl::StrAppend(&error_message, ". Error details: ", errbuf);
    }
    return absl::InternalError(error_message);
  }

  std::cout << "\nDownload successful!" << std::endl;
  return absl::OkStatus();
}
