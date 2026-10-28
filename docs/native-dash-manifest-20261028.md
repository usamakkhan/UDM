# Native DASH manifest reader

`DashManifest.cpp` adds a native MPD reader for the forthcoming live-refresh path. It uses Windows XmlLite and an in-memory stream, with DTD processing prohibited, the external resolver disabled, document conformance enabled and a nesting limit. This avoids delegating continuing playlist parsing to the browser.

The tree retains element namespace URI, local name, namespace-qualified attributes, text and child indices. Limits are 2 MiB of input, 12000 nodes, 32 nested elements, 64 attributes per element and 4 MiB of decoded strings. Cancellation is checked during parsing. Protected-content markers are rejected. No resources are fetched and external references are not followed.

Root metadata distinguishes static/OnDemand from dynamic/Live. Duration, update interval, time-shift buffer and suggested delay use optional values, preserving omitted versus zero. Availability start/end and publication time use explicit-timezone timestamps with fractional seconds normalized to microseconds. Calendar dates and offsets are checked; dynamic presentations require an availability start. Unsupported syntax, including leap-second spelling and calendar-month durations, is rejected explicitly.

The implementation uses the [Microsoft XmlLite reader properties](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ms752855(v=vs.85)) and [CreateXmlReader](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ms752822(v=vs.85)). The bundled Windows SDK provides the header and import library; production linking now includes xmllite.lib.

## Integration and validation limits

The reader is compiled into the production source set but is not yet invoked by a live transfer. Native segment addressing/timeline expansion, availability calculations, refresh scheduling, durable recording and browser admission remain unfinished. Dynamic MPDs continue to be rejected by the browser path.

Runtime XML corpus and live recording tests are deferred at the user's request. Compilation is recorded in `validation/native-dash-manifest-build-20261028.json`; successful compilation is not XML conformance or live-download acceptance. Browser 0.63.61 is unchanged. The candidate remains unsigned, uninstalled and unpublished.
