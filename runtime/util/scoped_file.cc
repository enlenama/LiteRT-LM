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

#include "runtime/util/scoped_file.h"

#include <cstddef>
#include <cstdint>
#include <limits>

#include "absl/log/absl_check.h"  // from @com_google_absl
#include "absl/status/status.h"  // from @com_google_absl
#include "absl/status/statusor.h"  // from @com_google_absl
#include "absl/strings/string_view.h"  // from @com_google_absl
#include "runtime/util/status_macros.h"  // IWYU pragma: keep

namespace litert::lm {

namespace {

bool IsFileValid(ScopedFile::PlatformFile file) {
  return file != ScopedFile::kInvalidPlatformFile;
}

}  // namespace

// static
absl::StatusOr<size_t> ScopedFile::GetSize(PlatformFile file) {
  if (!IsFileValid(file)) {
    return absl::FailedPreconditionError("Scoped file is not valid");
  }
  return GetSizeImpl(file);
}

absl::Status ScopedFile::Write(size_t offset, absl::string_view data) {
  ASSIGN_OR_RETURN(size_t _, Seek(offset));

  while (!data.empty()) {
    ASSIGN_OR_RETURN(size_t bytes_written, WriteAtCurrentPosition(data));
    data = data.substr(bytes_written);
  }
  return absl::OkStatus();
}

absl::StatusOr<size_t> ScopedFile::Seek(size_t offset) {
  ABSL_CHECK_LE(offset, std::numeric_limits<int64_t>::max());
  return SeekImpl(SeekFrom::kBeginning, offset);
}

absl::StatusOr<size_t> ScopedFile::GetCurrentPosition() {
  return SeekImpl(SeekFrom::kCurrent, /*offset=*/0);
}

}  // namespace litert::lm
