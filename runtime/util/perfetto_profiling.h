#ifndef THIRD_PARTY_ODML_LITERT_LM_RUNTIME_UTIL_PERFETTO_PROFILING_H_
#define THIRD_PARTY_ODML_LITERT_LM_RUNTIME_UTIL_PERFETTO_PROFILING_H_

#ifdef LITERT_LM_PERFETTO_ENABLED
#include <iostream>
#include <string>

#include "third_party/perfetto/include/perfetto/tracing/track_event.h"
#include "third_party/perfetto/include/perfetto/tracing/track_event_category_registry.h"

// To enable perfetto profiling on Android, run your binary with `--define
// LITERT_LM_PERFETTO_ENABLED=1` on a rooted Android device.
//
// To record the profiling, in a separate terminal, run the following:
// $ adb root
// $ adb push \
//     third_party/odml/litert_lm/runtime/util/data/perfetto_detailed.pbtxt \
//     /data/misc
// $ adb shell 'cat /data/misc/perfetto_detailed.pbtxt | \
//     perfetto --txt -c - -o \
//     /data/misc/perfetto-traces/perfetto-litert-lm.trace'
//
// When your trace has been collected, pull it from the device:
// $ adb pull /data/misc/perfetto-traces/perfetto-litert-lm.trace
//
// To view the trace, upload the trace file to https://ui.perfetto.dev/.
//
// NOTE: If you are using run_litert_lm.sh, you can use --add_perfetto_profiling
// and just follow the printed instructions from the script.

// The set of track event categories.
PERFETTO_DEFINE_CATEGORIES_IN_NAMESPACE(
    litert::lm, ::perfetto::Category("LiteRtLM")
                    .SetDescription("LiteRtLM related functions"));
PERFETTO_USE_CATEGORIES_FROM_NAMESPACE(litert::lm);

namespace litert::lm {

class PerfettoTraceScope {
 public:
  // Note that prior to the LiteRtLM graph initialization, InitializePerfetto()
  // needs to be called to allow Perfetto trace events to be recorded.
  explicit PerfettoTraceScope(const std::string event_name) {
    auto begin_lambda = [&event_name](perfetto::EventContext ctx) {
      ctx.event()->set_name(event_name);
      return;
    };
    TRACE_EVENT_BEGIN("LiteRtLM", nullptr, begin_lambda);
  }

  inline ~PerfettoTraceScope() { TRACE_EVENT_END("LiteRtLM"); }
};

}  // namespace litert::lm

// NOTE: Be mindful when adding LiteRtLM Perfetto traces. Too many Perfetto
// traces can affect timing, similar to excessive logging.

// Use to log traces from functions.
#define LITERT_LM_PERFETTO_TRACE_EVENT(event_name) \
  PerfettoTraceScope trace(event_name);

// Instead of rendering a block that spans a duration, this renders an arrow for
// a single point in time.
//
// NOTE: you can use `DynamicString` to use generated event names, e.g.
// LITERT_LM_PERFETTO_TRACE_EVENT_INSTANT(
//    perfetto::DynamicString{absl::StrCat("Event Id: ", id)});
#define LITERT_LM_PERFETTO_TRACE_EVENT_INSTANT(event_name) \
  TRACE_EVENT_INSTANT("LiteRtLM", event_name);

// Starts an asynchronous trace event. This is intended for events that exceed
// the scope of a function call and do not fit the slice to thread mapping
// implied in LITERT_LM_PERFETTO_TRACE_EVENT.
#define LITERT_LM_PERFETTO_TRACE_EVENT_ASYNC_START(event_name, event_id) \
  TRACE_EVENT_BEGIN("LiteRtLM", event_name, perfetto::Track(event_id));

// Ends an asynchronous trace event. Failing to finish an async event leads to
// broken slices in the trace viewer.
#define LITERT_LM_PERFETTO_TRACE_EVENT_ASYNC_END(event_id) \
  TRACE_EVENT_END("LiteRtLM", perfetto::Track(event_id));

#else
#define LITERT_LM_PERFETTO_TRACE_EVENT(event_name)
#define LITERT_LM_PERFETTO_TRACE_EVENT_INSTANT(event_name)
#define LITERT_LM_PERFETTO_TRACE_EVENT_ASYNC_START(event_name, event_id)
#define LITERT_LM_PERFETTO_TRACE_EVENT_ASYNC_END(event_id)
#endif  // LITERT_LM_PERFETTO_ENABLED

namespace litert::lm {
void InitializePerfetto();
}  // namespace litert::lm

#endif  // THIRD_PARTY_ODML_LITERT_LM_RUNTIME_UTIL_PERFETTO_PROFILING_H_
