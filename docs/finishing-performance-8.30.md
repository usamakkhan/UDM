# Finishing performance: 8.30 / browser 0.71.94

## Change and ordering

The 8.29 sequential 64 MiB durable-tail loss included two completion-catalog writes: publication saved Complete before the transfer's final hash, timing and metadata were available, and the owning worker then saved those additions. This candidate removes that duplicate durable checkpoint for bounded ordinary HTTP transfers. It does not omit the final file flush or move unfinished work beyond the measured process exit.

`PreparedTransferCommit` is a same-thread handoff between the transfer and its actual Manager worker. The transfer prepares the destination-local stage, applies metadata, flushes the downloaded file, joins its hash work and finalizes transfer fields. The worker then executes the existing journaled publication transaction with those final fields. Its completion save contains the final digest. Parts remain until that save is acknowledged; the published callback follows acknowledgement. There is no new background thread or persisted acceptance flag.

The final worker checkpoint is skipped only after accepted publication, matching active-worker ownership, an acknowledged current catalog sequence, no pending outcomes or storage error, an unchanged model snapshot, and a matching pinned physical catalog. Dirty, failed, scanner-modified or unverifiable state keeps the original forced save. Cleanup and callbacks cannot revoke an accepted file.

Cancellation before intent creation removes the unaccepted stage and retains resumable parts. A failed first completion save reports failure without a successful completion notification, retaining output, journal and parts for startup recovery. Destination and worker identity are revalidated before publication. Recovery intent creation, output identity checks and transaction ordering remain in the existing publication implementation.

Eligibility is deliberately bounded: known positive size through 64 MiB, ordinary raw HTTP, fixed local disk, no cross-volume copy, compression stripping, sparse data, replacement, POST, FTP or postprocessing. Initial prefetch workers and media/adaptive jobs keep their existing sequencing. This is not a claim to accelerate every download type.

## Timing semantics

Eligible transfer timing ends at the prepared handoff, before the Manager publication transaction. `PreparedTransferFinalizeSeconds` describes that boundary; `CommitStrategy` is `prepared-final-checkpoint`. Persisted `PublicationPhases` has scope `through-rename-before-final-catalog-save`. Later catalog, acknowledgement, pin release and cleanup durations are not fabricated into that already accepted record. Complete publication timings remain in the worker's runtime handoff object.

The external comparison still measures from the last response body byte through actual process exit, including final checkpoint and application-owned background work. A separate symmetric endpoint opens each completed output exclusively for writing, calls FlushFileBuffers and closes it after process exit, before independent hashing. Both endpoints and all trials must be reported; the latter cannot replace a failed original score. Neither endpoint measures a completion-dialog animation or all unrelated Windows service activity.

## Qualification

All **1,873 maintained checks** passed, including 133 completed-checkpoint checks, 274 completion-dialog component checks and 43 live gzip/prefetch/resume checks. New real-worker checks cover one acknowledged complete checkpoint, retained parts while acknowledgement is held, final digest availability, failed first-save recovery, cancellation and shutdown waiting. Twenty-five binaries were built from 492 frozen inputs with 42 canonical core objects; Core, Duplicates, Transfer and Bridge were rebuilt and 38 unchanged core objects retained. Shared media tools were reused.

## Installation and comparison outcome

The first installation attempt refused a running UDM main process before staging or replacing files. The second elevation request ended with Windows reporting "The operation was canceled by the user"; its helper did not start. After the user reported UDM closed, **the third attempt succeeded**. All seven replacements and registry version verify 8.30, both manifest files verify 0.71.94, both personal catalogs and shared tools remain unchanged. This verifies installed files, not a browser extension reload or live dialog animation.

The comparison retained all eighteen transfers: three alternating cold-launch pairs each for 4 MiB sequential, 64 MiB sequential and 64 MiB eight-connection split. It used the same memory-backed loopback origin, per-connection rate, latency and symmetric post-exit flush as 8.29. All independent final sizes/SHA-256 and all final UDM Complete-record digests matched. All measured writeback operation errors were zero. IDM connection settings were restored. [Raw results and limits](validation/finishing-8.30/comparison.json)

| Case | UDM median exit tail | IDM median exit tail | UDM median full elapsed | IDM median full elapsed |
| --- | ---: | ---: | ---: | ---: |
| 4 MiB sequential | 361.439 ms | 556.640 ms | 1.336 s | 2.355 s |
| 64 MiB sequential | 419.426 ms | 370.395 ms | 9.183 s | 10.018 s |
| 64 MiB split | 494.146 ms | 445.039 ms | 2.213 s | 3.687 s |

Original process-exit tails favored UDM in **6/9 pairs** and full elapsed times in **9/9**. This includes application-owned work and shutdown; **the no-slower target remains unmet**. Sequential 64 MiB UDM tails were 133.932, 505.240 and 419.426 ms against IDM 371.978, 370.395 and 357.722 ms. Split tails were 494.146, 735.937 and 274.098 ms against IDM 597.544, 445.039 and 365.236 ms. No slower trial is omitted.

| Case | UDM median tail through post-exit flush | IDM median tail through post-exit flush | UDM median full durable elapsed | IDM median full durable elapsed |
| --- | ---: | ---: | ---: | ---: |
| 4 MiB sequential | 441.312 ms | 865.576 ms | 1.407 s | 2.678 s |
| 64 MiB sequential | 482.932 ms | 438.553 ms | 9.247 s | 10.192 s |
| 64 MiB split | 595.795 ms | 702.400 ms | 2.315 s | 3.945 s |

The separate output-durability endpoint favored UDM in **7/9 tails** and **9/9 full elapsed times**. It also fails the all-pairs target, and does not replace the original failed score. Existing IDM profile, device activity, inter-trial cache state and request scheduling remain confounds. These trials establish observed outcomes, not a code-only causal improvement over 8.29 or a guarantee for other sites/disks.

## Remaining waits

Sequential 64 MiB trial 2 spent 31.122 ms in output flushing and 113.834 ms in recovery-intent flush/close. Trial 3 spent 177.917 ms and 102.810 ms respectively. Their file opens took 0.190 and 0.122 ms. The slower split trial spent 296.570 ms in file flushing and 198.642 ms in intent flush/close; its open was 0.139 ms. Current blocking is flushing rather than opening. These are measured portions of the tail, not a complete attribution of every remaining millisecond: final catalog/acknowledgement and shutdown happen beyond the persisted prefix timing scope.

The one-completion-write optimization passed the real-worker fault/acknowledgement checks, but did not eliminate storage-latency variation. Next work should investigate bounded writeback scheduling or journal/catalog IO without removing required barriers, moving work outside the full measurement, falsely reporting completion, or promising flush latency independent of Windows/storage activity. The historical 16.7 s and 2.7 s split events did not recur; their exact original causes are not established by these new measurements.

## Cleanup and final verification

After all qualification completed, verified all 25 linked target-object hashes and removed 228 generated fixture/compiler files individually. After the comparison processes exited and all hashes/sizes matched, removed eighteen benchmark output payloads. Total: **246 files / 1,032,827,287 logical bytes**. Boundary, reparse, link-count where applicable, size and SHA checks preceded deletion. Preserve all 25 qualified binaries, 42 canonical core objects, sources, evidence/catalogs, rollback and shared tools. Logical bytes are not measured physical reclaimed allocation. No setup package, Git commit, push or release.

[Qualification](validation/finishing-8.30.json) · [Installation attempts](validation/finishing-8.30/installation-attempts.json) · [Installed hashes](validation/finishing-8.30/installed.json) · [Cleanup](validation/finishing-8.30/cleanup-total.json) · [Final verification](validation/finishing-8.30/final-verification.json)
