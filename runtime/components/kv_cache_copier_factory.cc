#include "runtime/components/kv_cache_copier_factory.h"

#include <memory>

#include "absl/status/status.h"  // from @com_google_absl
#include "absl/status/statusor.h"  // from @com_google_absl
#include "absl/strings/str_cat.h"  // from @com_google_absl
#include "runtime/components/kv_cache_copier.h"
#include "runtime/components/kv_cache_copier_cpu.h"
#include "runtime/executor/executor_settings_base.h"

namespace litert::lm {

absl::StatusOr<std::unique_ptr<KVCacheCopier>> CreateKVCacheCopier(
    ::litert::lm::Backend backend) {
  switch (backend) {
    case Backend::CPU:
    // TODO(b/472518008): Implement KVCacheCopierGpu.
    case Backend::GPU:
      return std::make_unique<KVCacheCopierCpu>();
    default:
      return absl::InvalidArgumentError(
          absl::StrCat("Unsupported backend: ", backend));
  }
}

}  // namespace litert::lm
