# Finish attribution and automatic-checkpoint coalescing: 8.33 / browser 0.71.97

## Measured blocking

This increment does not change the 8.32 file-flush pacing. Instead, an owned diagnostic build adds QPC/thread timestamps around final output flushing, recovery-intent writing, catalog write/acknowledgement, cleanup, notification, integration stop, Manager stop, window teardown and application exit. Events use a bounded memory-mapped trace, with no synchronous log write at each checkpoint. Normal builds compile the additional finish points to no-ops; the existing startup trace keeps its default 128-event layout. Only the owned finish diagnostic uses 2,048 events and the existing fixture/path guard.

The first diagnostic group ran one paired trial per case. The split trace caught an automatic catalog write beginning **69.816 ms after final file flushing began**, completing at 95.643 ms. It therefore introduced about **25.827 ms of additional catalog IO during finalization**. The final output flush lasted 140.617 ms; a further 50.004 ms of advisory hash/transfer work followed. The mandatory final catalog write took 17.251 ms, acknowledgement approximately 0.001 ms, integration stop 0.178 ms and Manager stop 1.114 ms. Windows/MFC close and exit work remained in the process-exit measurement.

The initial samples were relatively fast. Two further diagnostic trials per case collected slower traces while native compilation was active. That load is an explicit confound: these samples locate blocking operations under load and **are not shipping speed qualification**. All eighteen diagnostic outputs matched independent size/SHA-256, all settings were restored, and all nine UDM traces had zero dropped events.

| Slower split sample, diagnostic follow-up trial 1 | Observed duration |
| --- | ---: |
| Process-exit tail, including all observed app work | 1,201.484 ms |
| Final file flushing | 446.563 ms |
| Recovery-intent write/publication | 139.562 ms |
| Intent's FlushFileBuffers call | 67.240 ms |
| Mandatory completion-catalog write/publication | 299.829 ms |
| Catalog's FlushFileBuffers call | 256.118 ms |
| Catalog flush-end to publication notification | 41.194 ms |
| Catalog acknowledgement after write | approximately 0.001 ms |
| Integration stop | 3.345 ms |
| Manager stop | 3.805 ms |
| CFrameWnd close call | 59.667 ms |
| ExitInstance | 16.440 ms |

Nested measurements are not additive. These timestamps establish where the sampled waits occurred, not which filesystem filter/device/scheduler caused each kernel wait. They do not retroactively identify the exact cause of older 16.7-second or 2.7-second stalls. The sampled slow path was dominated by file/journal/catalog IO, not an acknowledgement timeout or a pipe shutdown timer. Acknowledgement and Manager stop were fast. The trace is not an exhaustive OS service trace; actual process exit still covers uninstrumented application/runtime work.

[All trace summaries](validation/finishing-8.33/diagnostic-trace-summary.json) · [Initial diagnostic group](validation/finishing-8.33/diagnostic-comparison.json) · [Loaded diagnostic follow-up](validation/finishing-8.33/diagnostic-comparison-blocker-followup.json)

## Fix: remove eligible redundant automatic IO

`PreparedTransferCommit` now owns a short-lived finalization marker for eligible ordinary full-body transfers, after metadata preparation and before the mandatory final file flush/hash finishes. Eligibility remains the existing prepared-publication path: positive known size through 64 MiB, local fixed volume, same-file publication, non-prefetch raw ordinary HTTP, without replacement/copy/compression/FTP/POST/postprocessing fallbacks.

An automatic catalog save may coalesce its IO only when every active Cancel owner is marked as finalizing, there is no storage error and the detached snapshot has no admission/publication capture requiring acknowledgement. Explicit saves bypass the guard. Other active work, cancellation, storage-error retries and pending receipt/outcome captures retain actual IO. Markers are removed before publication's existing receipt protocol starts and on all unwind paths. The owning worker still performs the mandatory final transaction. Stop/failure forced saves and the physical acknowledged-catalog proof remain unchanged.

The marker registry is private, scoped and tied to exact active owners. It changes no Manager/Job layout, starts no thread and stores no synthetic durable acceptance flag. Failed optional registration/observation retains the normal automatic IO path. It does not cancel an automatic write already in flight, remove the final file/journal/catalog barriers or guarantee that a storage stall cannot recur. The measured overlap proves redundant work existed; it does not prove that eliminating its roughly 26 ms accounts for every millisecond of the larger historical delays.

## Qualification

All **1,913 maintained checks passed**, including 145 completed-checkpoint checks, 51 existing pacing checks, 274 completion-dialog component checks and 43 live gzip/prefetch/resume checks. Twelve new checks cover automatic IO deferral, forced save/retry exceptions, other active work, marker release, cancellation, prefetch exclusion and pending-admission acknowledgement.

The first new pending-admission fixture failed because its live catalog was clean: the existing changed-only early return ran before the new guard. The corrected fixture makes live state dirty and retains the original requirement for a real write and acknowledged admission. It then passes. Final qualification also makes optional observation failure return false, preserving automatic IO as the fallback. Initial failure and correction records remain available; they are not hidden or counted as passes.

All 25 binaries qualified from 492 frozen inputs and 42 canonical core objects. Core, Duplicates, Transfer and Bridge rebuilt; 38 unchanged core objects retained. After the correction, the three affected core modules and affected test rebuilt, and 24 unchanged target objects relinked. All final source, binary and core hashes verify. Shared media tools were reused without copies. Browser source manifests are 0.71.97; browser JavaScript is unchanged.

[Qualification](validation/finishing-8.33.json) · [Initial fixture failure](validation/finishing-8.33/initial-fixture-failure.json) · [Correction](validation/finishing-8.33/qualification-correction.json)

## Installation and performance status

The tested update was prepared with rollback and seven verified replacements, but Windows ended elevation with **“The operation was canceled by the user” before the deployment helper started**. No installed file changed. All seven previous installed hashes, both personal catalogs, shared tools and product registration remain verified **8.32**. No installed-app 8.33 comparison has run. The qualified executable was subsequently tested from its build folder, as distinguished below; installed performance remains unverified. The latest installed-app comparison remains the failed [8.32 comparison](validation/finishing-8.32/comparison.json).

Next: complete Windows approval, install verified 8.33, then repeat all eighteen alternating transfers after compilation has stopped. Retain original actual process-exit and separate symmetric post-exit output-flush metrics, complete elapsed time, every losing pair, independent final file size/hash and saved Complete-record digest. Do not use the loaded diagnostic's favorable relative outcomes as a replacement for that required comparison. Live completion-dialog animation and all-system hardware durability remain outside the CLI endpoints.

## Qualified portable comparison before installation

After rechecking all frozen source and executable hashes and confirming no compiler/linker remained active, ran the exact qualified 8.33 executable from its canonical build folder. Its SHA-256 matches the pending install payload. The shared tools junction points to the installed canonical tools. UDM still used isolated test catalogs. This is a **portable qualification comparison**, not an installed update or a substitute for the pending installed-path run. The installed product remains 8.32.

All eighteen expected sizes/independent SHA-256 values matched, as did final saved UDM Complete-record digests. Pacing counters retained the existing three sequential/one split early operations, with zero errors. IDM settings were restored. All trials and both endpoint families remain in [portable comparison evidence](validation/finishing-8.33/portable-comparison.json).

| Case | UDM median process-exit tail | IDM median process-exit tail | UDM median full elapsed | IDM median full elapsed |
| --- | ---: | ---: | ---: | ---: |
| 4 MiB sequential | 89.084 ms | 337.232 ms | 1.096 s | 1.989 s |
| 64 MiB sequential | 417.495 ms | 1,766.026 ms | 9.234 s | 11.213 s |
| 64 MiB split | 262.504 ms | 2,356.344 ms | 2.327 s | 9.141 s |

Actual process-exit tails favored UDM in **8/9 pairs**, full elapsed times **9/9**. All six sequential/split tail pairs favored UDM. The 4 MiB control trial 2 lost: **502.675 versus 299.680 ms**. Its file flush was 53.538 ms and intent flush/close 112.231 ms; the persisted publication prefix does not attribute every remaining millisecond. The original all-pairs target remains false.

| Case | UDM median tail through output flush | IDM median tail through output flush | UDM median full durable elapsed | IDM median full durable elapsed |
| --- | ---: | ---: | ---: | ---: |
| 4 MiB sequential | 117.088 ms | 437.548 ms | 1.106 s | 2.057 s |
| 64 MiB sequential | 484.377 ms | 1,821.937 ms | 9.300 s | 11.269 s |
| 64 MiB split | 271.348 ms | 4,735.837 ms | 2.335 s | 10.445 s |

The separate symmetric output-flush endpoint also favored UDM in **8/9 tails** and **9/9 full elapsed times**. It does not erase the losing control pair. UDM sequential tails were 111.910, 417.495 and 569.557 ms; split tails 262.479, 262.504 and 958.533 ms. Split trial 3 still spent **800.791 ms in final file flushing**. This comparison therefore does not establish elimination of storage latency. Large IDM outliers, existing profile, request scheduling, file location/cache state and background storage activity are retained confounds; no code-only causal improvement or universal guarantee follows. No unchanged-build repeat was used to seek a passing score.

## Next IO attribution layer

Read-only PE inspection confirms that the actual IDM executable (SHA-256 recorded in [import audit](validation/finishing-8.33/idm-import-audit.json)) imports FlushFileBuffers, WriteFile, CreateFileMappingA, MapViewOfFile, MoveFileExW and RegFlushKey. Imports do not establish whether, when, how often or for which files those functions run. They do not justify omitting UDM's final barriers or claiming either app's internal durability sequence.

The retained [Windows IO profile](../benchmarks/speed/finish-io.wprp) is accepted by this PC's WPR parser. It configures 64 KiB × 256 system buffers (**16 MiB**) and kernel file/disk IO, process/thread, context-switch/ready-thread events plus disk-init/FileFlush/FileOpEnd stacks. Validation only enumerated/details-checked the profile; **no system recording or ETL was created**. Final ETL size can exceed configured buffers because stop/rundown metadata is additional. [Validation output](validation/finishing-8.33/wpr-profile-validation.json)

The next privileged recording must enumerate existing recordings/kernel sessions, use a unique owned instance, have a bounded watchdog (45 seconds maximum), stop only its own instance and keep raw ETL private. Correlate only known download PIDs/paths and matching start/end IO events. Check dropped/incomplete events before attributing disk queue/service, filter or scheduling delays. Save a sanitized summary, verify it and delete the owned ETL afterward. Default-instance WPR status alone does not establish that all named sessions are idle. The required live trace and installed comparison remain incomplete. Memory-mode and instance-name options are documented by [Microsoft WPR command-line reference](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/wpr-command-line-options).

## Bounded recorder implementation and tooling amendment

The normal-token controller and separate administrator recorder are now implemented in [finish_io_trace.py](../benchmarks/speed/finish_io_trace.py). No elevation is launched automatically. The fresh request is hash-bound; PID plus creation-time handles bind both roles. The recorder owns only its random WPR instance, checks existing visible sessions and confirms the physical kernel collector so ETL merging is distinct from active recording. The configured 20–45 second budget reserves 15 seconds for cleanup. Controller death and deadline expiry stop collection; the workload itself is never killed for a tracing deadline. Failed export, unknown state and actual overruns cannot produce a complete capture. If named cleanup fails, an identifiable live supervisor keeps retrying its own instance; it does not silently exit with an unverified collector. Command history is bounded to avoid unbounded recovery memory growth.

All **19 supplemental profiler tests passed**, using injected recorder faults plus real owned Windows process identity/exit checks. The actual read-only plan passed, created no request directory and requested no elevation. The unelevated recorder entry point refused before starting WPR. None of these checks is a privileged live collection or proof of event coverage under actual load. That capture and ETL attribution remain pending. [Lifecycle results](validation/finishing-8.33/profiler-lifecycle-tests.json) · [Controller checks and tooling hashes](validation/finishing-8.33/profiler-controller-validation.json)

The benchmark gained an optional `--case` selector for bounded paired captures. Its default retains the exact original three case triples/order and complete per-transfer loop body. A two-transfer 4 MiB single-case smoke verified actual filtering, final hashes/digests and settings restoration, then removed its two payloads. It is labeled tooling smoke and is not substituted for the earlier failed 4 MiB qualification pair. [Smoke](validation/finishing-8.33/case-filter-smoke.json)

The helper was part of the 492-input qualification snapshot. This measurement-only amendment is therefore explicit: the original source hash was reconstructed exactly by reversing the four declared filter transformations, default/all selected case values were exercised, and the new helper hash was recorded separately. All 491 other frozen inputs and every native binary/core hash remain unchanged; no native rebuild is claimed. Original portable timings retain the original helper hash and are not relabeled. Future installed verification uses the amended helper's unchanged default full protocol. [Amendment and compatibility evidence](validation/finishing-8.33/measurement-input-amendment.json) · [Exact tooling diff](validation/finishing-8.33/benchmark-case-filter.patch)

The ready desktop remains 8.33/browser files 0.71.97, and the installed desktop remains 8.32. These changes are diagnostic tooling within the pending fix, not another installed application update or a new performance claim.

## Nine-sample finishing wait audit

The follow-up [IO research and implementation gates](finishing-io-research-8.33.md) also audits the retained per-catalog network milestones, identifies the unresolved early-flush join interval in the losing 4 MiB run, and separates C: qualification from the still-unverified historical D: cases. Installation remains deferred at the user's request while research continues.

Re-audited all nine original portable UDM rows without rerunning or replacing a transfer. Final output open took 0.106–0.160 ms, destination-settlement lock wait at most 0.0002 ms, and synchronous final hash bookkeeping at most 0.051 ms. Late hash completion reached 29.026 ms in one sample. Final file flush ranged from 0.526 to 800.791 ms (median 123.053 ms); publication intent writes reached 115.435 ms. These observations prioritize actual flush waits over another reopen or pacing change.

The separate file-availability-to-process-exit interval was 43.304–48.556 ms across these rows. This interval is not an isolated shutdown function: availability polling, publication/catalog work and application shutdown can overlap its boundary. The persisted publication timing stops before final catalog save, so neither an exact residual attribution nor an all-background-work guarantee follows. Early-writeback durations also overlap network/finish work and must not be added as disjoint tail intervals. [Original-row audit](validation/finishing-8.33/finish-wait-audit.json)

## Read-only IDM analysis attempt

REA 6.5.0 admitted the exact audited IDM executable with the existing Ghidra 12.1.3 installation, but analysis timed out at 330 seconds before producing call-site results. No runtime or flush-order conclusion follows. Stopped only the verified owned analysis process tree and individually removed its 41 ephemeral scratch files (95,310,234 logical bytes); original IDM, UDM, shared tools and catalogs were untouched. Raw diagnostics remain private. [Attempt record](validation/finishing-8.33/rea-analysis-attempt.json)

## Cleanup

Deleted **349 generated files / 2,490,997,517 logical bytes** after individual boundary, reparse, link-count where applicable, size and hash checks: eighteen diagnostic download payloads, nine decoded trace binaries, diagnostic compiler/executable/marker artifacts and qualified/initial fixture payloads plus unique target objects, eighteen portable-comparison outputs, two case-filter smoke outputs, the temporary built-in profile export, 41 owned REA/Ghidra scratch files, four temporary disassembly inspection files and seventeen join-attribution diagnostic artifacts. Decoded traces, results, catalogs/journals and correction evidence remain. Target-object hashes were verified before deletion. Preserve all 25 ready binaries, 42 canonical core objects, source, personal catalogs, rollback and canonical tools. Logical bytes are not physical reclaimed allocation.

The deployment/comparison scripts remain ready to resume. Final proof accepts only the exact recorded hashes of target objects intentionally removed by the verified cleanup and preserves the earlier cleanup receipts. No setup package, Git commit, push or release was created.

[Installation status](validation/finishing-8.33/installation-attempts.json) · [Cleanup](validation/finishing-8.33/cleanup-total.json) · [Final verification](validation/finishing-8.33/final-verification.json)
