# Detached settlement validation and precise attribution: 8.26

Desktop 8.26; Chromium and Firefox manifests 0.71.90. The previous [8.25 comparison](finishing-performance-8.25.md) retained a 16.748-second split-transfer tail, including 14.345 seconds in the aggregate Settle phase. Its historic aggregate timing cannot establish which individual operation blocked.

## Change

The finish worker previously called `validateGrabberFolder` inside the shared Manager lock, after changing status to Verifying and copying its job state. The validation can inspect destination filesystem paths. A slow response would therefore block model readers, other download workers and UI operations for its duration.

`finishSettlementSnapshot` now updates/copies model state under the lock and performs validation on an immutable owned snapshot after releasing it. No worker is detached and completion remains gated on all final work. `PublicationPreparation` still pins and validates the source/destination, and `publishFile` checks the current destination and membership before/after publication. Moving this preliminary inspection does not authorize stale destinations or bypass redirected-folder checks.

The persisted FinishPhases now separately measure:

- `SettleLockWait`: acquiring the Manager lock after content classification.
- `SettleSnapshot`: job transition/copy and release of the lock.
- `SettleValidateFolder`: detached Grabber destination inspection.
- `SettleDestination`: constructing/ensuring the destination directory.
- `SettleRecoveryCheck`: checking publication/replacement recovery journals.
- `SettleLengthQuery`: querying the actual data-file length.
- `SettleLengthFix`: writer-open/length correction, only when needed.

The original Settle total remains. Time outside these operations, such as stage preparation bookkeeping, remains in that total; do not assume every sub-counter sums exactly to it. None of these counters removes work from the full-exit benchmark or identifies an old uninstrumented stall retroactively.

## Validation

Five additional checks hold a validator on another thread, prove that the actual model mutex remains accessible, inspect the Verifying/time/prefetch transition, change live destination state and verify that validation owns the original snapshot without overwriting newer state, and confirm that the held interval is attributed to validation rather than lock acquisition. Existing publication destination/recovery and completed-checkpoint gates remain in the qualification set.

Build/qualification, installation and installed measurements are separate gates. Compare the same alternating three-case/three-trial loopback workload against IDM, retaining every tail and full elapsed time. Verify output length, independent SHA-256 and UDM's final saved hash only after process exit; restore IDM settings. Remove owned temporary files after their processes finish and preserve reports, source, current binaries/core, personal catalogs and rollback files.

All **1,822 native checks passed**, including 87 completed-checkpoint checks, 274 completion-component checks through observed exit 0 and the real gzip/prefetch/resume workflow. Twenty-five binaries, 490 frozen inputs and 42 core objects are qualified; Transfer and Bridge rebuilt, with 40 other core hashes retained. Shared media tools are linked, not copied. [Qualification](validation/finishing-8.26.json). Installation and installed performance remain separate from this qualification.

## Installed comparison

Installed and verified all seven files, version registration and unchanged personal catalogs. The [eighteen-transfer comparison](validation/finishing-8.26/comparison.json) passed independent output sizes/hashes and UDM final saved hashes; IDM settings were restored. All nine paired full elapsed times favored UDM, but only eight finishing tails did, so the no-slower target remained unmet.

Median UDM/IDM tails: 136.008/345.000 ms (4 MiB), 156.140/344.474 ms (sequential 64 MiB), 301.928/418.521 ms (split 64 MiB). Median full elapsed seconds: 1.008/2.039, 8.897/9.829 and 1.942/3.481 respectively. The one failure was sequential 64 MiB trial 2 at 470.056 versus 340.412 ms. It recorded 236.009 ms in publication and 28.739 ms in final flush; settlement was 1.379 ms and lock acquisition approximately 0.0001 ms.

All three split tails were faster, at 301.928, 326.198 and 297.765 ms. The historical 16.7-second outlier did not recur. Its exact cause therefore remains unproved; the new finer counters cannot retroactively resolve that old event. All observed settlement operations in this run were short. A separately retained Windows storage-error-log query found no matching error events around the old outlier, which does not rule out latency. [Storage-log check](validation/finishing-8.26/historical-storage-log-check.json)

All eighteen benchmark payloads were independently rechecked and individually removed afterward, 830,472,192 logical bytes. [Cleanup](validation/finishing-8.26/comparison-cleanup.json). A subsequent increment targets redundant source opens and finer publication I/O attribution; these results remain unchanged.
