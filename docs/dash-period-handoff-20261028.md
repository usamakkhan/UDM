# Recorded multi-Period DASH handoff

This increment connects the browser Period parser/selection, native capture validation, worker layout, journal and assembly path. Native admission now accepts the supported recorded Period contract and advertises `dash-periods`. Browser 0.63.60 requires that capability before handing off a multi-Period selection; older desktop builds receive an update-required message.

## Browser flow

Single-Period catalogs retain the legacy plan shape. Multi-Period catalogs are stored once as internal catalog records shared by their quality offers. Offer references are tied to the player token, frame, signature and expiry; the storage bound retains the new offer/catalog group together. Hidden catalog records are not returned as download choices.

Selection preserves the first Period's chosen quality/audio/subtitle through all Periods using the existing matcher. SIDX expansion occurs per Period under one shared request/byte/reference budget and the existing preparation deadline. The total selected segment count and serialized plan size are checked again after expansion. Cookies and captured headers are collected only for selected resource origins across the complete presentation.

## Native flow

Capture validates every Period and its resources, validates supplied credential origins against every selected segment, and derives display duration from the Period durations. The ordinary job receipt, duplicate check, prefetch, confirmation and final publication flow remains in use.

Address refresh compares the complete ordered Period identity, including media clocks and segment ranges, while omitting signed URLs and resource validators. Accepted refresh still uses the existing retained-byte verification phase before requesting missing segments. Recovery journals remain strict for Period selections and retain the legacy migration policy for older single-Period jobs.

The transfer layout feeds the existing worker pool. Each Period is assembled independently and probed; compatible outputs are joined, subtitles are placed on the combined timeline, and the final file enters the existing exact-size confirmation and publication flow.

## Supported boundary and unverified work

This is implementation for recorded, contiguous Periods with matching selected tracks and compatible actual codec/time-base configurations. Live/dynamic MPDs, gaps/overlaps, codec changes, missing/ambiguous tracks and unresolved timing remain unsupported. Stream-copy boundary behavior, full-file completeness, caption synchronization, restart, address refresh and exact final-size behavior have NOT been tested in a browser or at runtime for this new path. The current half-second duration guard is not proof of frame/sample completeness. Full IDM parity remains incomplete.

## Build and delivery

JavaScript syntax and mirror checks passed for Chromium/Firefox. Native production and setup results are recorded in `validation/dash-period-handoff-build-20261028.json`. Runtime tests are deferred at the user's request. The candidate is unsigned, uninstalled and unpublished. This document supersedes the earlier notes saying the recorded multi-Period path is entirely gated.
