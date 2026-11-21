# Preparing publication storage before body reception: 8.28

Desktop 8.28; Chromium/Firefox manifests 0.71.92. In [8.27](finishing-performance-8.27.md), the remaining slow sequential trial spent 115 ms writing its publication intent and 65 ms writing its catalog. Source identity and rename were negligible. The old uninstrumented 16.7-second split stall has not recurred; its historical blocking call remains unknown.

## Preparation and durability

The transfer worker now ensures its publication-journal directory exists before network body reception, alongside its existing output/parts preparation and outside the model lock. A fresh profile no longer needs to create that recovery directory after receiving the last byte. The actual recovery intent is still serialized, written and flushed only after content verification and source/destination preparation. No premature intent exists during the download, and failure to prepare storage fails before spending the body transfer. Journal order, required checksum gates, atomic replacement, final data flush and final completion checkpoint remain.

Five additional actual HTTP-worker checks pause the owned origin before its first body byte and confirm that the publication directory exists while the specific journal does not. Completed-checkpoint checks increase from 87 to 92.

## Finer atomic intent observation

Optional atomic-text checkpoints now report directories-ready, temporary-opened and bytes-written in addition to the existing temporary-flushed boundary. The underlying WriteFile and FlushFileBuffers calls and error checks are unchanged; the normal unobserved catalog path does not create diagnostic records.

Manager-owned publication diagnostics retain the aggregate WriteIntent and add IntentOperations intervals through directory readiness (including argument serialization before entry), temporary-file open, byte write, flush/close, and final atomic publication. Timings include user-mode scheduling and intervening callback work; they are not kernel disk-service times. Other publication/settlement counters and full-exit measurement remain. Do not infer an old event's cause from a later fast run.

## Validation gates

Four shared core modules rebuild (Core, Duplicates, Transfer, Bridge); 38 hashes remain unchanged. Reuse only unchanged target objects with recorded hashes/dependencies, relink all versioned binaries, run the maintained checks, install verified files with rollback/catalog protection, then compare all three alternating workloads through actual process exit. Preserve failed observations, verify final size/independent and saved SHA-256, restore IDM settings and remove only owned temporary outputs afterward. No GUI timing or universal latency guarantee is implied.

## Installed result: remaining delays persist

All **1,832 native checks passed**, including 92 completed-checkpoint checks, 74 publication-preparation checks, 274 completion checks through observed exit 0 and the live gzip/prefetch/resume workflow. Twenty-five binaries, 490 frozen inputs and 42 core objects qualified. All seven installed replacements and version registration were verified; personal catalogs and canonical media helpers were preserved. [Qualification](validation/finishing-8.28.json) · [Installation](validation/finishing-8.28/installed.json)

The [original eighteen-transfer comparison](validation/finishing-8.28/comparison.json) matched every output's independent size/SHA-256 and UDM's final saved digest. IDM settings were restored. **UDM won seven of nine paired finishing tails and eight full elapsed times; the requested target remains unmet.** Both losses and every other observation remain.

| Case | UDM median exit tail | IDM median exit tail | UDM median full elapsed | IDM median full elapsed |
| --- | ---: | ---: | ---: | ---: |
| 4 MiB sequential | 545.544 ms | 534.375 ms | 1.513 s | 2.454 s |
| 64 MiB sequential | 176.831 ms | 369.531 ms | 8.939 s | 10.127 s |
| 64 MiB split | 347.977 ms | 430.169 ms | 2.018 s | 3.822 s |

Sequential 64 MiB won all three tails, including the previously troublesome trial 2 (168.148 versus 369.531 ms). However, 4 MiB trial 1 lost at 588.490 versus 534.375 ms, and split trial 2 lost at **2,714.530 versus 429.354 ms**. Its settlement was only 1.592 ms and model-lock acquisition approximately 0.0002 ms. The observed blocking work was **1,831.202 ms published-file open/flush**, **295.287 ms intent flush/close**, **62.573 ms intent publication** and **128.932 ms catalog writing**. Intent directory readiness/open/byte write together took approximately 2.022 ms; source identity and hashing did not cause this stall. The published-file phase includes opening its write handle and FlushFileBuffers; it does not isolate those two calls. The intent flush interval includes FlushFileBuffers and handle close. These are user-mode wall intervals and can include scheduling, filesystem/filter and device waits, not a hardware diagnosis.

The historical 16.7-second/14.345-second-Settle event has not recurred, and cannot be attributed retroactively to the model-lock hazard or any particular API. This later outlier is a different, now-instrumented flush delay. The safe code changes do not establish that every timing improved, nor justify dropping required flushes to claim parity.

## Supplemental durability endpoint

Windows normally buffers file writes and drains them separately; FlushFileBuffers requests that the specified file's buffered information be sent to its device. Process exit alone is therefore not proof that a file has passed a final explicit durability barrier. [Microsoft file caching](https://learn.microsoft.com/en-us/windows/win32/fileio/file-caching) · [FlushFileBuffers](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-flushfilebuffers)

One additional, explicitly separate split-64-MiB pair used the same installed 8.28 binary and the same base runner, with in-memory instrumentation. After actual application exit and before hashing, open each owned completed output exclusively for writing and call FlushFileBuffers. Preserve the original exit metric and add the post-exit flush and durable endpoint; do not replace the failed acceptance result. All output/final-record hashes matched, and IDM settings were restored. [Supplemental record and method](validation/finishing-8.28/durability-probe.json)

| Metric | UDM | IDM |
| --- | ---: | ---: |
| Original process-exit tail | 345.352 ms | 3,391.550 ms |
| Explicit post-exit FlushFileBuffers call | 0.436 ms | 526.091 ms |
| Last byte through post-exit flush endpoint | 369.276 ms | 3,953.351 ms |
| Full elapsed through that endpoint | 2.155 s | 9.669 s |

This single pair demonstrates why an exit-only endpoint can omit additional flush work; it is not statistical parity evidence, proof that IDM never flushes, or proof that all probe time came from this file's dirty bytes. Device/filesystem contention can also affect a flush. A complete future comparison should keep exit and symmetric durability endpoints separately, alongside GUI timing when accessible. The original primary score remains seven of nine, not an adjusted passing score.

## Cleanup and remaining work

After all processes exited, outputs were independently rechecked and individually removed. Across the three increments and supplemental pair, cleanup removed **789 generated files / 3,311,914,806 logical bytes**, including benchmark/fixture payloads, unique target objects and 80 superseded test executables. Linked target objects were hash-verified before cleanup. Preserve all 25 current binaries, 42 canonical core objects, source, personal catalogs, reports/journals/screenshots, rollback copies, shared tools, required historical fixtures and the excluded preview-http-7.82 tree. Logical sizes are not physical allocation measurements. [Cleanup receipts](validation/finishing-8.28/cleanup-total.json)

Remaining performance work is durable writeback/publication scheduling under storage contention, with recovery correctness preserved and full elapsed time checked. A possible buffering redesign requires aligned-I/O and concurrency/recovery evidence before adoption; it must not simply move work after exit or weaken final flush semantics. No additional unchanged-build reruns were used to seek a favorable acceptance score. No setup package, commit, push or release.
