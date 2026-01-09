#include "runtime/components/kv_cache_copier_cpu.h"

#include <cstring>
#include <memory>

#include "absl/status/status.h"  // from @com_google_absl
#include "litert/cc/litert_macros.h"  // from @litert
#include "litert/cc/litert_tensor_buffer.h"  // from @litert
#include "runtime/components/kv_cache_copier.h"

namespace litert::lm {

namespace {

absl::Status CopyBuffer(const TensorBuffer& buffers_from,
                        TensorBuffer& buffers_to) {
  LITERT_ASSIGN_OR_RETURN(auto read_lock,
                          ::litert::TensorBufferScopedLock::Create(
                              buffers_from, TensorBuffer::LockMode::kRead));
  LITERT_ASSIGN_OR_RETURN(auto write_lock,
                          ::litert::TensorBufferScopedLock::Create(
                              buffers_to, TensorBuffer::LockMode::kWrite));

  LITERT_ASSIGN_OR_RETURN(auto buffer_size, buffers_from.PackedSize());
  memcpy(write_lock.second, read_lock.second, buffer_size);
  return absl::OkStatus();
}

}  // namespace

absl::Status KVCacheCopierCpu::CopyPass::CopyKeyBuffer(
    const ::litert::TensorBuffer& input_tensor,
    ::litert::TensorBuffer& output_tensor) {
  return CopyBuffer(input_tensor, output_tensor);
}

absl::Status KVCacheCopierCpu::CopyPass::CopyValueBuffer(
    const ::litert::TensorBuffer& input_tensor,
    ::litert::TensorBuffer& output_tensor) {
  return CopyBuffer(input_tensor, output_tensor);
}

absl::Status KVCacheCopierCpu::CopyPass::Submit() { return absl::OkStatus(); }

std::unique_ptr<KVCacheCopier::CopyPass> KVCacheCopierCpu::CreateCopyPass() {
  return std::make_unique<KVCacheCopierCpu::CopyPass>();
}

}  // namespace litert::lm
