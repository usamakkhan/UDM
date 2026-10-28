# Native DASH track continuity across refresh

Native track selection now stores the Period, AdaptationSet and Representation IDs together with resolved template addressing and a declared-format fingerprint. Selection requires nonempty, bounded, control-free IDs and declared codecs. Duplicate sibling track IDs are rejected.

The format fingerprint retains inherited MIME/content type, codecs, dimensions, frame rate, sampling rate, language, sample aspect ratio and scan type. Role, accessibility, channel, essential/supplemental descriptors and labels are retained in canonical sorted form. Metadata is bounded. This is deliberately strict: equivalent declarations written differently may still require future normalization. It does not prove elementary-stream compatibility without inspecting downloaded media.

On refresh, the rematcher finds the same IDs within the same Period, resolves current resource addressing, and compares declared format, timescale, presentationTimeOffset and Period start. Missing, duplicate or incompatible tracks fail explicitly. XML position, closest resolution and bandwidth proximity are not used as substitutes for the selected identity. URL tokens and timelines can refresh without changing the declared selection, but previously recorded segment binding/overlap checks still belong to the forthcoming recorder.

## Integration limits

The functions consume the native manifest snapshot and call the template resource resolver. They are not yet called by a running recorder. Automatic Period transitions, compatible replacements across different Period IDs, transfer journaling, clock/refresh orchestration and browser live admission remain unfinished. A missing old Period can indicate normal expiry; the caller still needs an explicit transition policy rather than treating the missing ID as a new selection.

Runtime and regression testing remains deferred at the user's request. Production compilation and installer packaging are recorded in `validation/native-dash-continuity-build-20261028.json`. Extension 0.63.61 is unchanged. The candidate remains unsigned, uninstalled and unpublished; full IDM parity is not established.
