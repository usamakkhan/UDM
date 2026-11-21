# Owned final-writeback overlap: UDM 9.61

UDM 9.61 prepares the ordinary publication recovery record while its owned final body writeback runs. It waits for writeback, promotes its metadata snapshot to a strict read pin and rehashes the complete source before renaming it. Exact durable catalog acceptance remains required before completion notification and cleanup. Both source extension manifests advance to 0.73.25; this increment adds no extension capture logic.

This candidate is **rejected for performance** and superseded by the owned-pin implementation being developed for 9.62. It was not installed. The installed app and extension remain unchanged. All observations are preserved; no claim of full IDM parity follows from passing its safety tests.

## Transaction and worker lifetime

Only the existing Manager-owned prepared ordinary path is eligible: known positive bodies up to 64 MiB on a fixed volume, a single assembled output, no sparse normalization, copied-volume output, replacement, POST or FTP. The network receiver and optional prefinish workers have already joined. The old synchronous path remains when asynchronous worker creation fails or the context is ineligible.

The body task owns copied path and normalization values. It captures no Manager, Job, UI or stack-owned hasher. The owning transfer joins it before staging cleanup on failure; the prepared commit owns and joins it after a successful handoff. No detached thread is introduced. Unknown final hashes settle the writeback before ordinary strict preparation, so they cannot create early intents.

A metadata snapshot can inspect size and identity while a write handle is live. It offers **no write exclusion** and cannot be used to hash content. The early intent includes `FlushBeforePublish: true`, the exact hash, target, source and before/after completion identity. Recovery must hash and flush those bytes before acceptance. The existing synchronous producer does not emit this marker.

Before rename, the owning worker consumes the actual writeback result, requires successful open/normalization/flush/close, acquires the ordinary strict read pin, checks volume/file identity and length against the snapshot, rehashes every byte and validates the source name, directories, cancellation and destination inputs again. Changed or unavailable files keep their recovery record and parts. The metadata-only interval is never treated as protected content.

The existing rename, published-name pin, catalog proof, exact acceptance check and accepted-file pin release remain. Parts cleanup and completion callbacks still follow acceptance. A failed intent, failed flush, cancellation or exception cannot return while owned body IO remains live.

## Timing scope

`PublishedFileOpen`, `PublishedFileFlush` and related fields record actual worker operation durations. `PublishedWritebackJoin` measures the owner's wait and result retrieval. They overlap other publication work and must not be summed as independent elapsed time. `JoinWritebackAndProtect` also includes the strict pin and full verification hash. Whole-process and post-exit output-durability comparisons must include all these costs.

The extra verification hash may outweigh the saved intent wait on larger files. Qualification must retain those regressions rather than report only a faster finishing phase.

## Validation and performance

Five native products built at 9.61; 1,273 affected backend assertions pass. This includes 103 overlap/failure/recovery assertions, 145 actual HTTP completion assertions and 31 independent finishing ownership checks. The initial compile failed because the existing Handle type supports move construction but not move assignment; the corrected build and its captured source epoch are separate from that failed compile. Two early functional prechecks are retained and are not counted a second time.

All 27 matched transfers (three cases, three clients, three rotating trial positions) match independent final size and SHA-256 checks. Each payload was removed after verification. All nine first-availability and process-exit finishing pairs lose against 9.60. Median process-exit finishing times:

| Case | 9.61 | 9.60 | IDM |
| --- | ---: | ---: | ---: |
| 4 MiB sequential | 187.790 ms | 167.134 ms | 392.961 ms |
| 64 MiB sequential | 397.516 ms | 188.500 ms | 564.418 ms |
| 64 MiB split | 470.189 ms | 275.971 ms | 454.967 ms |

9.61 beats IDM in six of nine process-exit finishing pairs, one of nine first-availability pairs and nine of nine full elapsed/post-exit-durability pairs. Those IDM wins do not excuse the regression against the prior UDM build. Raw IDM exit codes remain 1 despite verified files; their meaning has not been established. GUI/browser completion timing is not measured by this command-line workload.

[Build, safety checks and source epochs](validation/publication-overlap-9.61/build.json), [all comparisons and failed gates](validation/publication-overlap-9.61/comparison/analysis.json), [retained per-transfer results](validation/publication-overlap-9.61/comparison/results.json).
