# Single-Period DASH assembly contract review

## Confirmed source gap

Extension 0.63.72 filters recorded explicit timeline segment references against the Period window. That alone does not implement sample-level clipping or common-clock assembly.

- `browser/chromium/media.js::dashCatalog` sends one-Period presentations through the legacy choice path. Ordinary audio/video tracks only carry `clock` when `describe` is enabled; MP4 WebVTT is a separate exception.
- `native/AdaptiveTransferLayout.hpp::adaptiveTransferLayout` marks a layout for Period assembly only when the plan contains `periods`.
- `native/Adaptive.cpp` feeds a legacy single-Period plan to the generic FFmpeg merge command. That command does not apply declared per-track presentation offsets or an explicit Period start/end bound.
- `native/DashPeriodAssembly.hpp::assembleDashPeriods` does apply per-track offsets, copy timestamps, a zero start and the declared Period duration, followed by stream and coarse duration checks.
- `native/AdaptiveCapture.hpp::adaptiveSelection` deliberately removes `durationMs` because it is display metadata. Changing that field alone does not invalidate a recovered or refreshed selection. Reusing it as an authoritative trim boundary would therefore be unsafe.

These are source observations. Extra boundary samples or audio/video displacement are possible consequences, not measured output failures. FFmpeg behavior and packet dependencies still require runtime acceptance when testing resumes.

## Required implementation

The next implementation must carry a recovery-bound assembly contract for a single Period: explicit duration and per-track timescale/presentationTimeOffset, plus the information required to distinguish already-normalized subtitle clocks. Preserve display-only durationMs semantics for legacy captures. Either use the existing validated one-entry `periods` contract (which accepts 1 through 128 Periods) where its required track descriptions are available, or add an explicit single-Period timing contract with equivalent validation and identity binding.

Before handoff, capability-check the desktop. Preserve the new contract through audio/subtitle selection, index expansion, request-resource attachment and URL refresh. Assembly must shift each media input from its declared sample clock, retain overlapping packets as required for decodability, enforce the declared interval, and avoid shifting MP4 WebVTT captions twice. Unknown-duration and older captures must retain an explicitly identified legacy path; do not infer a trustworthy clip boundary from a display hint.

Recovery must reject changed assembly timing before reusing staged final output. Exact-size presentation must continue to measure the actual retained muxed file that will be published; it must not substitute track totals or predicted overhead. Candidate acceptance must eventually cover different audio/video offsets, boundary-straddling segments, single-Period M4A, plaintext and MP4 WebVTT subtitles, indexed tracks, refreshed URLs and changed timing with saved partial data.

## Current artifact

No source or installer change was made in this review. UDM 1.1.0 / extension 0.63.72 remains the latest packaged candidate. Runtime/parser/browser/decoder tests remain deferred at user request. No installation or publication was performed. [Source evidence](validation/dash-single-period-contract-review-20261029.json) records hashes for the inspected implementation.
