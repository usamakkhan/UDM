# Verified shutdown checkpoint reuse: 8.25

Desktop 8.25 and Chromium/Firefox manifests 0.71.89. The preceding [8.24 comparison](finishing-performance-8.24.md) had faster medians in all three cases but two slower individual finishing tails. The 4 MiB outlier spent approximately 374 ms in transfer finishing against a 671 ms full-exit tail; sequential 64 MiB spent approximately 271 ms against a 464 ms tail. Work after the transfer remains part of the acceptance metric.

## Change

After joining transfer threads, stopping periodic checkpoints, waiting for relocation and publication ownership, `Manager::stop()` now checks whether its final durable checkpoint already covers the current state. Previously the first stop always forced another atomic catalog write, even after the completion worker had saved the final hash and cleanup record. This required another write/flush/replacement before process exit.

Reuse requires all of the following under the manager lock: no active download, no catalog batch, no storage error, a nonzero acknowledged sequence equal to the latest sequence, no pending admission overlay or publication outcome, and exact current snapshot equality with the acknowledged snapshot. Open the actual catalog with read-only sharing, reject directories/reparse points, parse the pinned file and require exact equality. Any failed/missing proof takes the original real save path. Explicit `save()` calls still force writes. Failed shutdown save still clears stopping and reports the error. No new Manager fields or layout changes.

The final transfer checkpoint, required checksum gates, publication journal, final data flush, advisory hash join, parts cleanup and automatic-exit gates remain. The change removes a duplicate durable write; it does not move necessary work after application exit or declare completion early. External changes after a completed validation remain possible, as with any saved file; this is not an ownership guarantee against arbitrary future external writers.

## Validation plan

Catalog lifecycle checks count actual durable write boundaries and inspect persisted state. Cover clean first/repeated stop; dirty state; removed, malformed or externally changed catalog; failed save and retry; pending admission receipts; and destructor fallback. Existing completion, publication ownership/commit, replacement, recovery and live gzip/prefetch checks remain required. Build and qualification must finish before installation.

Install qualified native files and both manifest updates with rollback copies and hash verification, preserving personal catalogs and shared media tools. Compare the installed executable against IDM with the same three-case, three-trial alternating loopback protocol. Keep all trials and full elapsed times. Measure through actual application exit, including background work and shutdown, before independently verifying size, SHA-256 and the final UDM catalog digest. Restore IDM settings. Remove only owned verified temporary payloads after their processes have exited. Preserve result records, failures and rollback files.

This bounded test cannot promise UDM is never slower under all storage/network conditions. GUI dialog timing is separate from full-process timing. The prior native desktop-control initialization failure does not prevent CLI/process-lifetime comparisons, but no new visual timing is inferred from them.

## Installed outcome: target not met

All **1,817 native checks passed**, including 40 catalog lifecycle checks, 186 publication-ownership checks, 274 completion checks through observed exit 0 and the live gzip/prefetch/resume workflow. Twenty-five binaries, 489 frozen inputs and 42 core objects were qualified; Core and Bridge rebuilt and 40 other core hashes retained. The initial publication fixture expected a needless write of unchanged state; its corrected setup includes an unsaved setting before testing a required save failure. A separate test-environment failure was resolved by linking the canonical installed media tools, with no helper copies. Both initial failures remain separately recorded. [Qualification](validation/finishing-8.25.json)

Installed and verified all seven changed files and version registration, preserving both personal catalogs. A first preflight stopped without replacing files because Firefox was open. The final deployment allowed the unchanged browser runtime to remain open while retaining actual product-process checks, source/target/backup hashes, atomic replacement and rollback on file-lock failure. [Installed receipt](validation/finishing-8.25/installed.json)

The [comparison](validation/finishing-8.25/comparison.json) completed all eighteen transfers with matching independent output hashes and final UDM catalog hashes. IDM settings were restored. C: had approximately 6.84 GB free immediately before the run, compared with different storage conditions in earlier runs; background storage activity was not isolated. No build or cleanup ran in this pipeline during measurement. [Environment](validation/finishing-8.25/comparison-environment.json)

| Case | UDM median finishing tail | IDM median finishing tail | UDM median full elapsed | IDM median full elapsed |
| --- | ---: | ---: | ---: | ---: |
| 4 MiB sequential | 110.888 ms | 343.908 ms | 1.075 s | 2.012 s |
| 64 MiB sequential | 109.311 ms | 372.474 ms | 8.840 s | 9.850 s |
| 64 MiB split | 307.246 ms | 461.824 ms | 5.960 s | 3.724 s |

**Only six of nine paired tails and six of nine full elapsed times favored UDM. The requested no-slower-in-any-test target remains unmet.** All failures remain: split trial 2 was 16,748.316 versus 461.824 ms; 4 MiB trial 3 was 536.785 versus 343.908 ms; sequential 64 MiB trial 3 was 1,230.854 versus 305.981 ms. The split full elapsed median is also slower. Lower tail medians do not override these failures.

The split outlier recorded approximately 14.345 seconds in Settle, 395 ms staging, 1.186 seconds in CommitOutput and 178 ms in final flush; its overlapped buffer hash took 185 ms with less than 0.2 ms foreground hash wait. Settle includes manager-lock acquisition, the verification-state snapshot, destination-directory work, recovery-journal checks and final length query. Existing phase timing cannot identify which individual operation blocked. Do not attribute the delay to disk, antivirus, the new shutdown path or another application without a trace. The source change in this increment is in shutdown, after these phases.

Next: separate lock-wait and filesystem-operation timings within Settle, trace catalog/journal flush and replacement boundaries, and collect bounded storage/scheduling evidence during a controlled reproduction. Preserve every result and investigate the exact blocking call before changing recovery semantics. Do not rerun unchanged builds until a favorable sample appears, omit final work, exclude outliers, or weaken durability to claim parity.

After all processes exited and final size/hash checks completed, eighteen benchmark payloads were individually removed: 830,472,192 logical bytes. Another 362 generated fixture payloads and unique compiler outputs were individually boundary/reparse/link-count/size/hash checked and removed: 201,998,410 logical bytes. Qualified binaries, core, source, catalogs/journals, reports/screenshots, shared tools and rollback copies remain. [Benchmark cleanup](validation/finishing-8.25/comparison-cleanup.json) · [Fixture cleanup](validation/finishing-8.25/fixture-cleanup.json). No setup package, Git push or release.
