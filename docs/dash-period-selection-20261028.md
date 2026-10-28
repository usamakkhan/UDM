# DASH cross-Period selection

The new Period catalog now retains MIME type, declared codecs, width, roles, channel configuration, sample rate, declared label, adaptation/representation IDs, timescale and presentation-time offset. These fields are only added to `dashPresentation` catalogs. Existing single-Period handoff plans retain their existing identity shape, avoiding an unnecessary refresh incompatibility.

`dashPeriodSelection` constructs a selection covering every Period. It accepts a quality index and optional audio/subtitle keys from the first Period and supports audio-only M4A. Later tracks must match the original codec, dimensions, frame rate, language, roles, channel configuration, sample rate and label. Declared IDs break ties only after these properties match; otherwise the closest declared bandwidth is selected. Equally ranked candidates are rejected. Missing tracks and format changes are explicit errors rather than implicit quality/language substitutions. Declared codecs are required for audio/video matching. Channel descriptors are compared without relying on their XML ordering.

The result keeps Period index, ID, start/duration and selected tracks separate. Initialization/index resources and each track's media clock remain available for native assembly. It requires finite positive Period durations and enforces the aggregate 10000-segment and 4 MiB draft-plan bounds. SIDX expansion will still require a subsequent aggregate-budget check before any eventual handoff.

## Scope and next integration

This selection API is not called by the browser download path yet. Multi-Period handoff remains rejected; native resource enumeration, credential scopes, checkpointing, per-Period muxing, joining and address-refresh identity must be implemented before enabling it. Timing metadata alone does not prove that streams are compatible: the native assembly path must inspect actual codec configuration and account for each media clock. Gaps, overlaps, changing formats and track layouts remain open work, not completed parity.

## Build

Extension 0.63.59 has matching Chromium/Firefox media.js files and passed JavaScript syntax checks. Packaging uses the unchanged compiled native 1.0.4 payload. See `validation/dash-period-selection-build-20261028.json`. Runtime tests are deferred at the user's request. The candidate is unsigned, uninstalled and unpublished.
