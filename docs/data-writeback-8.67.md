# Progress writeback overhead, 8.67 / browser 0.72.31

UDM 8.67 removes unnecessary progress-side locking and condition-variable notifications from the data-only writeback path. All five native products and both manifest updates are installed and hash-verified. The installed executable wins all nine paired IDM tests at actual process exit, post-exit durability, full elapsed and full durable elapsed endpoints. This is a measured result on this PC, not universal timing parity or proof that the earlier 155 ms observation had one isolated cause.

## Concrete change

8.66 took the worker bookkeeping mutex and called `notify_one` for every credited chunk. Most updates neither crossed the next 8 MiB trigger nor needed to wake a worker already inside synchronous IO.

8.67 maintains an atomic written-byte high-water mark and next threshold. Sub-trigger updates and updates during active IO bypass the worker mutex. A notification is issued only when new pending work needs a wakeup. The worker publishes the next threshold before checking the high-water mark on completion; sequentially consistent atomic ordering prevents a concurrent final update from falling between those two operations without either being observed or entering the scheduling path.

The trigger, data-only API, operation serialization, conservative completion snapshot, optional-error handling, pause/resume credit and final full durability/publication barrier are unchanged. Shutdown diagnostics still distinguish in-flight IO from unstarted work, including newly credited bytes accumulated while notifications are suppressed. `WakeNotifications` counts notification calls, not kernel context switches.

The existing queued writer was separately tested before this change. It improved total elapsed but increased median finishing from 471 to 859 ms against the contemporaneous normal 8.66 build. It remains private and was not enabled in 8.67. All nine probe payloads, generated metadata and probe executable/object were removed. [Rejected experiment and every observation](validation/queued-data-probe-8.66/result.json), [cleanup](validation/queued-data-probe-8.66/cleanup.json).

## Build and recovery

Five production binaries have verified 8.67 resources/hashes. Bridge and normal Transfer rebuild against preserved qualified cores. Both manifests are 0.72.31; no ordered-writer qualification define is used. **1,342 assertions pass across sixteen runs on C: and D:.** The twenty data-only assertions on each drive include progress under a deliberately held bookkeeping mutex, no redundant notification during a held operation, requeue of later bytes, exception/failure containment, joined shutdown and actual data-only plus mandatory full-flush IO.

Real 32 MiB transfers on both drives pause after approximately 12 MiB, join successful data-only IO, exit, reload the saved catalog in a fresh process and resume to the independently expected full-file hash. [Build identities](validation/data-writeback-8.67/build.json), [assertions](validation/data-writeback-8.67/checks.json), [HTTP recovery](validation/data-writeback-8.67/http-recovery.json).

## Four-way development comparison

Four D: 64 MiB split trials rotate 8.67, frozen normal 8.66, frozen normal 8.65 and installed IDM so each occupies every order position once. All sixteen transfers use the same synthetic origin seed, 20 ms response latency, 8 MB/s per-connection rate and eight configured connections. Actual request scheduling can differ. All original samples, including the first 8.67 total-time outlier, remain in the data.

| Median | 8.67 | 8.66 | 8.65 | IDM |
| --- | ---: | ---: | ---: | ---: |
| Last origin body send to actual exit | 384.9 ms | 367.5 ms | 662.9 ms | 438.8 ms |
| Last body send through post-exit flush | 407.7 ms | 388.9 ms | 681.0 ms | 1,085.3 ms |
| Full elapsed through actual exit | 2.555 s | 2.576 s | 2.677 s | 3.696 s |
| Full elapsed through post-exit flush | 2.577 s | 2.597 s | 2.697 s | 4.346 s |

The earlier 155 ms total-time regression does **not reproduce for 8.66 in this new comparison**: its median is already below 8.65. Therefore this run cannot establish that the hot-path change causally removed exactly 155 ms. 8.67 has the lowest total/durable median of the four, and retains a large finishing advantage over 8.65 and IDM. Its finishing median is nevertheless **17.4 ms longer than 8.66** in this run. Exact no-loss performance versus 8.66 remains unproven; neither the outlier nor individual losses are removed to manufacture a pass. [All observations and medians](validation/data-writeback-8.67/development-comparison.json), [explicit decision limits](validation/data-writeback-8.67/development-decision.json).

The private four-client harness copy was deleted after capture. Its small [reproduction patch](../benchmarks/speed/experiments/four-client-comparison-8.67.patch) and [original hash/cleanup receipt](validation/data-writeback-8.67/harness-copy-cleanup.json) preserve the test definition without retaining another full source copy.

## Installation and actual installed comparison

One successful Windows approval replaced five native binaries and both manifests transactionally, with verified backups and rollback handling. Installed product registration reads 8.67. Both saved personal catalogs remain byte-for-byte unchanged. All 94 installed extension payload files match source; only their manifests required replacement. These checks establish installed files, not a live browser-extension reload or browser capture test. [Installation receipt](validation/data-writeback-8.67/installed.json).

The actual installed executable then completed three alternating paired trials for each of 4 MiB sequential, 64 MiB sequential and 64 MiB split against IDM. No Frida hooks or MFC observation wrapper were used. Timings include actual application-process exit, joined application work and `/q` shutdown. A symmetric exclusive output open/flush/close after exit measures a separate durable endpoint. Shared OS/service work is not attributed exclusively to either app, and GUI animation is not measured. Both IDM connection settings are restored.

| Installed case | UDM exit tail | IDM exit tail | UDM durable tail | IDM durable tail | UDM full elapsed | IDM full elapsed |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 4 MiB sequential | 186.7 ms | 412.0 ms | 203.1 ms | 466.8 ms | 1.136 s | 2.265 s |
| 64 MiB sequential | 281.0 ms | 406.6 ms | 313.4 ms | 1,094.9 ms | 9.047 s | 9.988 s |
| 64 MiB split | 348.3 ms | 397.3 ms | 371.6 ms | 1,075.9 ms | 2.597 s | 3.604 s |

UDM wins **all nine paired comparisons at each of the four endpoints**, including full durable elapsed. Every file matches expected size/hash and exits normally. The full durable-elapsed medians are respectively 1.159 / 2.365 s, 9.080 / 10.676 s and 2.620 / 4.238 s for UDM / IDM. [Every installed observation, executable hash and paired score](validation/data-writeback-8.67/installed-comparison.json).

## Cleanup and remaining qualification

All thirty-four development/installed comparison payloads were removed immediately after verification: **1,904,214,016 logical bytes**. Both HTTP payloads, sixteen suite roots and generated comparison metadata were subsequently removed: **73,347,640 logical tree bytes**, including the two 32 MiB recovery outputs. Eighteen temporary test executable/object files were removed: **62,887,473 bytes**. The temporary four-client harness copy was also deleted. These are logical counts, not measured physical-space or drive-wear savings. [Cleanup](validation/data-writeback-8.67/cleanup.json).

Installed and development products, required cores, compact evidence and the verified installation rollback backup remain. No new setup package, Git push or release was created. The original exact no-loss requirement versus 8.66 and universal IDM parity remain unproven; the supported result is the concrete hot-path reduction and the recorded installed comparisons, with all limitations retained.
