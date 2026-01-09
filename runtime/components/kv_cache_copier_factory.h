#ifndef THIRD_PARTY_ODML_LITERT_LM_RUNTIME_COMPONENTS_KV_CACHE_COPIER_FACTORY_H_
#define THIRD_PARTY_ODML_LITERT_LM_RUNTIME_COMPONENTS_KV_CACHE_COPIER_FACTORY_H_

#include <memory>

#include "absl/status/statusor.h"  // from @com_google_absl
#include "litert/cc/litert_environment.h"  // from @litert
#include "litert/cc/litert_ranked_tensor_type.h"  // from @litert
#include "runtime/components/kv_cache_copier.h"
#include "runtime/executor/executor_settings_base.h"

namespace litert::lm {

absl::StatusOr<std::unique_ptr<KVCacheCopier>> CreateKVCacheCopier(
    ::litert::lm::Backend backend);

}  // namespace litert::lm

#endif  // THIRD_PARTY_ODML_LITERT_LM_RUNTIME_COMPONENTS_KV_CACHE_COPIER_FACTORY_H_
