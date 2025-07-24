// Copyright 2024 The ODML Authors.
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

#include <fcntl.h>
#include <sys/stat.h>

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>

#include "absl/log/absl_check.h"  // from @com_google_absl
#include "absl/status/status.h"  // from @com_google_absl
#include "absl/status/statusor.h"  // from @com_google_absl
#include "absl/strings/string_view.h"  // from @com_google_absl
#include "runtime/util/scoped_file.h"
#include "runtime/util/status_macros.h"  // IWYU pragma: keep

namespace litert::lm {

// static
absl::StatusOr<ScopedFile> ScopedFile::Open(absl::string_view path) {
  int fd = open(path.data(), O_RDONLY);
  RET_CHECK_GE(fd, 0) << "open() failed: " << path;
  return ScopedFile(fd);
}

// static
absl::StatusOr<ScopedFile> ScopedFile::OpenWritable(absl::string_view path) {
  int fd = open(path.data(), O_RDWR);
  RET_CHECK_GE(fd, 0) << "open() failed: " << path;
  return ScopedFile(fd);
}

// static
void ScopedFile::CloseFile(int file) { close(file); }

// static
absl::StatusOr<size_t> ScopedFile::GetSizeImpl(int file) {
  struct stat info;
  int result = fstat(file, &info);
  if (result < 0) {
    return absl::ErrnoToStatus(errno, "Failed to get file size");
  }
  return info.st_size;
}

absl::Status ScopedFile::Truncate(size_t size) {
  ABSL_CHECK_LE(size, std::numeric_limits<int64_t>::max());

  int result = -1;
  do {
#if defined(__LP64__)
    result = ftruncate(file_, size);
#else
    result = ftruncate64(file_, size);
#endif
  } while (result == -1 && errno == EINTR);
  if (result < 0) {
    return absl::ErrnoToStatus(errno, "Failed to truncate file");
  }
  return absl::OkStatus();
}

absl::StatusOr<size_t> ScopedFile::SeekImpl(SeekFrom whence, int64_t offset) {
  static_assert(static_cast<int>(SeekFrom::kBeginning) == SEEK_SET);
  static_assert(static_cast<int>(SeekFrom::kCurrent) == SEEK_CUR);
  static_assert(static_cast<int>(SeekFrom::kEnd) == SEEK_END);

  off_t result;
#if defined(__LP64__)
  result = lseek(file_, offset, static_cast<int>(whence));
#else
  result = lseek64(file_, offset, static_cast<int>(whence));
#endif
  if (result < 0) {
    return absl::ErrnoToStatus(errno, "Failed to seek file");
  }
  return result;
}

absl::StatusOr<size_t> ScopedFile::WriteAtCurrentPosition(
    absl::string_view data) {
  ssize_t bytes_written = -1;
  do {
    bytes_written = write(file_, data.data(), data.size());
  } while (bytes_written == -1 && errno == EINTR);
  if (bytes_written < 0) {
    return absl::ErrnoToStatus(errno, "Failed to write to file");
  }
  return bytes_written;
}

}  // namespace litert::lm
