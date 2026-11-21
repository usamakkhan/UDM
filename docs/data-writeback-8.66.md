# Split writeback, 8.66 / browser 0.72.30

The [8.65 trace](app-finishing-phase-analysis-8.65.md) placed 55 linked write requests, totaling roughly 49 MiB of transfer bytes, inside a 927 ms split-body flush. Its final disk-flush commands began about 906 ms into the call. The existing split path had performed only one early full optional flush. This supported testing data drainage during reception instead of another adjustment to that single flush's timing.

## Implementation and invariants

Eligible fixed-volume split transfers in the existing 32–64 MiB / at most eight-connection window now use one `DataOnlyWriteback` worker. Each additional 8 MiB of credited successful writes can request data drainage. Requests coalesce; the worker never invokes two operations concurrently and never holds the Manager lock during IO. This is a trigger, not a hard bound on outstanding dirty data: receivers continue if storage falls behind.

The worker dynamically resolves `NtFlushBuffersFileEx` and uses `FLUSH_FLAGS_FILE_DATA_ONLY`. Microsoft documents that this writes cached file data without requiring metadata writeback or a storage-cache synchronization. It is deliberately **not** a durability proof. [Microsoft API semantics](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntflushbuffersfileex).

Credit arriving during a call is not assumed drained by it. A failed/throwing optional request disables further drainage for that worker instead of generating repeated failures. Shutdown discards unstarted requests and joins in-flight IO before closing the file handle. Missing exports retain the prior optional strategy; unsupported operations can fail conservatively while mandatory completion remains responsible for durability.

The final full flush, publication metadata/journal ordering, file hash, disk-credit rules, pause/resume accounting and recovery transaction remain unchanged. The separate ordered-body qualification path remains private. Sequential transfers retain their existing strategy. The catalog records the actual optional mode and operation/error/join statistics.

## Build and correctness

All five production outputs have verified **8.66** resources and hashes; both browser manifests advance to **0.72.30**. Bridge and normal Transfer rebuild against the frozen 8.65 object set; no private ordered-writer define is used. Eight targeted suites run on both drives: **1,336 assertions pass**. Seventeen new assertions per drive exercise serialized drainage, progress during a held operation, failure/exception handling, joined shutdown and real data-only plus final full-flush behavior.

Actual 32 MiB HTTP transfers on C: and D: pause after roughly 12 MiB, join successful data-only work, exit normally, reload the saved catalog in a fresh process, resume and complete with the independently expected file hash. [Build identities](validation/data-writeback-8.66/build.json), [suite assertions](validation/data-writeback-8.66/checks.json), [HTTP recovery](validation/data-writeback-8.66/http-recovery.json).

The build's source freeze caught a concurrent **diagnostic comparator** edit after native compilation. All compiled native inputs were independently verified unchanged before reusing those outputs and running the checks. The comparator's original/current fingerprints remain recorded; no native binary was relabeled or rebuilt merely to hide a provenance failure.

## Actual completion against IDM and 8.65

Three D: 64 MiB split trials rotate the order of 8.66, the frozen normal 8.65 App and installed IDM. The synthetic loopback origin uses the same seed, 20 ms response delay and 8 MB/s per-connection rate. Both apps receive eight configured connections; their actual scheduling may differ. These are uninstrumented App executables, not the Frida/MFC observation wrapper.

Timings include actual application-process exit, joined application background work and `/q` shutdown. A symmetric exclusive output flush after exit separately measures a durable endpoint. The original process-exit endpoint is retained. This does not measure GUI animation or attribute shared operating-system/service activity exclusively to either application. IDM's two connection settings are restored afterward. Its documented command-line `/n` and `/q` options supply silent download and exit behavior. [IDM command-line documentation](https://www.internetdownloadmanager.com/support/command_line.html).

| Median, three trials | UDM 8.66 | UDM 8.65 | IDM |
| --- | ---: | ---: | ---: |
| Last origin body send to actual exit | 329.1 ms | 643.9 ms | 395.0 ms |
| Last origin body send through post-exit flush | 345.5 ms | 684.8 ms | 1,558.5 ms |
| Full elapsed through actual exit | 2.696 s | 2.541 s | 6.896 s |
| Full elapsed through post-exit flush | 2.712 s | 2.577 s | 7.554 s |

8.66 wins all three paired IDM comparisons at each of these four endpoints. All nine files have exact expected length/hash, normal exit and complete availability. Its optional requests report zero errors and no operation in flight at finish. Median final body-flush time falls from 504.8 to 127.2 ms.

**Retained regression:** versus 8.65, median full elapsed is about 155 ms longer, and median full durable elapsed about 136 ms longer. Moving data IO into reception improves the finishing wait but does not establish a total-time improvement over 8.65. Machine/storage activity and order remain confounds. These results establish the observed D: split improvement against IDM, not universal no-regression or complete inside/out parity. [Every observation, paired score and executable fingerprint](validation/data-writeback-8.66/comparison.json).

## Cleanup and delivery status

All nine comparison payloads were removed immediately after their checks: **603,979,776 logical bytes**. Both HTTP recovery payloads, sixteen owned suite trees and comparison metadata were subsequently removed: **73,006,636 logical tree bytes** in total. Eighteen temporary test executable/object files were removed: **62,763,649 bytes**. Compact evidence, production outputs and required core hashes remain. These counts are not measured physical reclaimed space or drive-wear savings. [Cleanup](validation/data-writeback-8.66/cleanup.json).

This is a tested native development build. It has not been installed or packaged as an installer. Next investigate the retained total-time cost while preserving the measured finishing gain and mandatory durability; do not substitute a shorter displayed completion time for actual process/background-work completion.
