#ifndef THIRD_PARTY_ODML_LITERT_LM_RUNTIME_COMPONENTS_KV_CACHE_COPIER_CPU_H_
#define THIRD_PARTY_ODML_LITERT_LM_RUNTIME_COMPONENTS_KV_CACHE_COPIER_CPU_H_

#include <memory>

#include "absl/status/status.h"  // from @com_google_absl
#include "litert/cc/litert_tensor_buffer.h"  // from @litert
#include "runtime/components/kv_cache_copier.h"

namespace litert::lm {

class KVCacheCopierCpu : public KVCacheCopier {
 public:
  class CopyPass : public KVCacheCopier::CopyPass {
   public:
    CopyPass() = default;

    absl::Status CopyKeyBuffer(const ::litert::TensorBuffer& input_tensor,
                               ::litert::TensorBuffer& output_tensor) override;
    absl::Status CopyValueBuffer(
        const ::litert::TensorBuffer& input_tensor,
        ::litert::TensorBuffer& output_tensor) override;
    absl::Status Submit() override;
  };

  std::unique_ptr<KVCacheCopier::CopyPass> CreateCopyPass() override;
};

}  // namespace litert::lm

#endif  // THIRD_PARTY_ODML_LITERT_LM_RUNTIME_COMPONENTS_KV_CACHE_COPIER_CPU_H_
