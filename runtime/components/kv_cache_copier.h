#ifndef THIRD_PARTY_ODML_LITERT_LM_RUNTIME_COMPONENTS_KV_CACHE_COPIER_H_
#define THIRD_PARTY_ODML_LITERT_LM_RUNTIME_COMPONENTS_KV_CACHE_COPIER_H_

#include <memory>

#include "absl/status/status.h"  // from @com_google_absl
#include "litert/cc/litert_tensor_buffer.h"  // from @litert

namespace litert::lm {

class KVCacheCopier {
 public:
  // The KVCacheCopier must outlive the CopyPass.
  class CopyPass {
   public:
    CopyPass() = default;

    virtual ~CopyPass() = default;

    // Not copyable or movable
    CopyPass(const CopyPass&) = delete;
    CopyPass& operator=(const CopyPass&) = delete;

    virtual absl::Status CopyKeyBuffer(
        const ::litert::TensorBuffer& input_tensor,
        ::litert::TensorBuffer& output_tensor) = 0;

    virtual absl::Status CopyValueBuffer(
        const ::litert::TensorBuffer& input_tensor,
        ::litert::TensorBuffer& output_tensor) = 0;
    virtual absl::Status Submit() = 0;
  };

  virtual ~KVCacheCopier() = default;

  // Returns a CopyPass that can be used to copy multiple buffers. The
  // KVCacheCopier must outlive the returned CopyPass.
  [[nodiscard]] virtual std::unique_ptr<CopyPass> CreateCopyPass() = 0;
};

}  // namespace litert::lm

#endif  // THIRD_PARTY_ODML_LITERT_LM_RUNTIME_COMPONENTS_KV_CACHE_COPIER_H_
