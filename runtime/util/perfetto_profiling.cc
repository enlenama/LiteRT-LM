#include "runtime/util/perfetto_profiling.h"

#ifdef LITERT_LM_PERFETTO_ENABLED
#include "third_party/perfetto/include/perfetto/tracing/backend_type.h"
#include "third_party/perfetto/include/perfetto/tracing/tracing.h"
#include "third_party/perfetto/include/perfetto/tracing/track_event.h"

// Reserves internal static storage for perfetto tracing categories.
PERFETTO_TRACK_EVENT_STATIC_STORAGE_IN_NAMESPACE(litert::lm);

namespace litert::lm {
void InitializePerfetto() {
  perfetto::TracingInitArgs args;
  args.backends = ::perfetto::kSystemBackend;
  perfetto::Tracing::Initialize(args);

  litert::lm::TrackEvent::Register();
}
}  // namespace litert::lm
#else

namespace litert::lm {
void InitializePerfetto() {}
}  // namespace litert::lm
#endif  // LITERT_LM_PERFETTO_ENABLED
