# Native DASH Period contract

`DashPeriods.hpp` adds native validation and decomposition of the browser's multi-Period selection draft. It retains source index, optional Period ID, start/duration and an independently validated ordinary DASH plan for each Period. This is preparation for native transfer and assembly, not enabled multi-Period downloading.

Validation checks presentation size, flags, Period count and increasing source indices, unique declared IDs, finite positive durations, contiguous boundaries, track layout, codec descriptions and per-track media clocks. It delegates URLs, ranges, containers, subtitles and resource validators to the existing single-Period validator and applies the aggregate 10000-segment bound. Unexpanded indexes are rejected. Resource bindings belong to each Period rather than an unrestricted presentation-wide list.

Cross-Period identity compares declared format, language, role, channel layout and related selection metadata while allowing adaptation/representation IDs to differ across Periods. A separate recovery identity retains the complete ordered Period plans and media clocks while omitting signed segment URLs and resource validators. This recovery helper is not yet connected to the journal; it does not establish recovery correctness.

The existing native admission function explicitly validates then rejects a nested Period plan with an assembly-unavailable error. It cannot accidentally accept the first Period through a mixed `tracks`/`periods` payload. No capability is advertised and the browser handoff remains gated. The next work is resource enumeration and checkpoint binding, independent Period assembly, codec verification and final joining before enabling admission.

## Build status

Production native compilation and installer packaging are recorded in `validation/dash-period-native-build-20261028.json`. The browser remains 0.63.59. Runtime tests are deferred at the user's request. The candidate is unsigned, uninstalled and unpublished. Successful compilation does not prove transfer, recovery or full IDM parity.
