# DASH recording start and stop interval

Native capture now accepts an optional intended interval in seconds relative to the selected Period. The protected index persists captureStart and optional captureEnd before resource requests. A saved start cannot move during recording. A stop can be supplied later, but cannot be changed or removed once set. Bounds must be finite, nonnegative, ordered and within the current seven-day presentation limit. Omitting the interval on later calls preserves existing metadata.

Assembly uses an explicit interval to distinguish deliberate clipping of overlapping segment boundaries from accidental loss of recorded content. Every selected track must contain the requested start and, when set, the requested end. Missing coverage fails instead of silently shortening the result. Segment bytes outside a proven requested interval can be excluded, which accommodates audio/video segment boundaries that do not align. Actual stream-copy cuts remain packet-granular and need runtime verification.

If only a start is known, assembly uses the common saved end; the caller must set the requested stop boundary when that is required by the recording lifecycle. Older indexes without an explicit interval retain the previous conservative coverage guard. The returned staging metadata reports the chosen start and duration.

## Integration status

Inspection of the current live job dispatch confirmed it still accepts HLS only. The interval contract is a prerequisite for connecting DASH to its start/resume/stop controls without confusing segment overlap with missing coverage. This change does not enable browser live DASH or install a running job loop. Capture credential scoping, polling/window selection, Period transitions and final publication still require integration.

Runtime and regression tests remain deferred at the user's request. Production compilation and packaging are recorded in `validation/native-dash-capture-interval-build-20261028.json`. Browser 0.63.61 is unchanged. The candidate remains unsigned, uninstalled and unpublished; complete IDM parity is not established.
