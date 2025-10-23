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

#include <iostream>
#include <string>

#include "absl/base/log_severity.h"  // from @com_google_absl
#include "absl/flags/parse.h"  // from @com_google_absl
#include "absl/log/globals.h"  // from @com_google_absl
#include "absl/status/status.h"  // from @com_google_absl
#include "litert/c/internal/litert_logging.h"  // from @litert
#include "runtime/engine/model_manager.h"

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  // Overrides the default for FLAGS_minloglevel to error.
  absl::SetMinLogLevel(absl::LogSeverityAtLeast::kError);
  LiteRtSetMinLoggerSeverity(LiteRtGetDefaultLogger(), LITERT_SILENT);

  if (argc < 2) {
    std::cerr << "Usage: lit <pull|run|rm|list>" << std::endl;
    return 1;
  }

  litert::lm::ModelManager model_manager;
  std::string command = argv[1];
  absl::Status status;

  if (command == "pull") {
    if (argc < 3) {
      std::cerr << "Usage: lit pull <model_alias_or_url>" << std::endl;
      return 1;
    }
    status = model_manager.Pull(argv[2]);
  } else if (command == "run") {
    if (argc < 3) {
      std::cerr << "Usage: lit run <model_alias_or_name>" << std::endl;
      return 1;
    }
    status = model_manager.Run(argv[2]);
  } else if (command == "rm") {
    if (argc < 3) {
      std::cerr << "Usage: lit rm <model_name>" << std::endl;
      return 1;
    }
    status = model_manager.Remove(argv[2]);
  } else if (command == "list") {
    status = model_manager.List();
  } else {
    std::cerr << "Unknown command: " << command << std::endl;
    return 1;
  }

  if (!status.ok()) {
    std::cerr << "Error: " << status.message() << std::endl;
    return 1;
  }

  return 0;
}
