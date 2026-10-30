# Browser live DASH SegmentBase selection

Extension 0.63.66 adds supported indexed live DASH video, alternate audio and audio-only choices. Inherited SegmentBase attributes, Initialization and RepresentationIndex locators are resolved for catalog selection. The desktop remains authoritative for the fetched manifest, exact index clocks, source versions, capture, recovery and assembly.

Ambiguous or mixed addressing inheritance, external addressing elements, unsupported children, missing initialization/index locators, invalid or excessive ranges and inconsistent embedded/external index alternatives are excluded. In-file initialization and indexes require ranges. Unsupported-only audio remains an explicit error instead of silently producing video without audio. Index ranges are limited to 512 KiB and initialization ranges to 256 MiB, matching the native bounds.

Credential lookup retains one concrete URL per exact origin across media, index and initialization alternatives, bounded to 64 origins per track. The browser does not fetch the index or expand its entries. Existing request-context and optional cookie permission rules still apply.

The final selected tracks carry a SegmentBase marker. Handoff checks the new live-dash-segment-base desktop capability after audio selection. Older native hosts receive an update-required message. The new native host advertises the capability; existing Template and SegmentList checks remain in place.

Chromium and Firefox scripts are mirrored and passed JavaScript syntax checks. Six changed extension files were staged and hash-verified. All five native production targets and the installer compiled successfully; compiler warnings remain. The [build record](validation/browser-live-dash-base-build-20261029.json) records source and installer hashes. The candidate is unsigned. Runtime browser, capture, restart, cancellation, playback and IDM comparison tests remain deferred at user request. No installation or publication is performed.

The native requirement for immutable, strongly versioned media within a Period remains. This does not add support for mutable growing shared files, incomplete low-latency chunks, encrypted streams or incompatible codec transitions.
