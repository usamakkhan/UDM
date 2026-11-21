# App finishing phase analysis, 8.65

`benchmarks/speed/analyze_app_finishing.py` selects the actual final native body flush and, when admitted live records exist, joins that interval to the independently recorded process, thread and disk-request timeline. It is diagnostic infrastructure, not a product speed improvement. Native and browser versions remain 8.65 / 0.72.29.

## Existing observations

The native selector accepts all five retained instrumented App observations. It requires a unique body file, successful contiguous native writes covering the complete expected payload, unique enclosing Win32 calls, and a unique final successful body flush inside the measured MFC invocation. It does not select a flush solely because it is the slowest event.

| Existing observation | Final native flush | Last body write to MFC return |
| --- | ---: | ---: |
| C: 64 MiB, sequential | 5.7262 ms | 60.1205 ms |
| D: 64 MiB, sequential | 90.7708 ms | 217.9973 ms |
| D: 64 MiB, eight connections | 722.2815 ms | 906.7674 ms |
| C: 8 KiB, base observer | 2.3505 ms | 70.9373 ms |
| C: 8 KiB, read observer | 0.7718 ms | 69.8117 ms |

[Calibration receipts](validation/scheduling-filter-8.65/app-native-selection-controls.json) retain the original private-receipt hashes. These are reanalyses of existing samples: no new download, payload hash verification or speed comparison occurred. MFC return excludes the console controller gates and later CRT/global teardown; it is not whole-process completion.

## Joined analysis and its gates

The joined mode reads the [headless capture runner](headless-app-capture-8.65.md) records. It checks process/image identity, matching QPC frequencies and boundaries, normal exit and integrity receipts, collector shutdown and loss/cap accounting, known thread lifetimes, and the exact separately documented version-5 context-switch prefix model. The original full-schema failures remain failures.

The selected native flush must belong to a known thread. Context switches must witness its running boundaries and pair its observed off-CPU intervals. Disk completions must match recorded initiations by request identity, generation, kind, time and initiating thread. Only linked flush-request intervals wholly inside the native call are reported. Overlapping intervals are unioned rather than added twice. Missing links do not establish absence of disk work.

The result deliberately keeps `fullSchemaQualificationPassed`, `kernelCauseVerified`, `performanceComparison` and `fileAttributionFromKernelVerified` false. Correlated request intervals do not distinguish exclusive hardware time, filesystem/filter work, system-worker I/O or scheduling causes. Reported off-CPU time is not exact CPU-time accounting. Integrity is taken from the capture receipt, not reverified against a payload already removed.

## Validation and next step

[Thirty-nine policy controls](validation/scheduling-filter-8.65/app-phase-policy-checks.json) cover valid joins, inconsistent clocks/processes, incomplete body coverage, ambiguous final flushes, missing call envelopes, malformed disk correlations and overlapping request intervals. They use synthetic timelines and do not qualify the live joined path.

## Subsequent live observations

The approved session and filtered 8 KiB fixture admitted two real 64 MiB App captures. Both transfers completed with matching hashes/catalogs, normal exit, zero kernel event/buffer losses and confirmed logger shutdown. The generated payloads and App copies were removed. [Capture/cleanup receipts](validation/scheduling-filter-8.65/live-app-captures.json).

| Case | Final native flush | Linked write requests during call | Write interval union | Final disk-flush interval union |
| --- | ---: | ---: | ---: | ---: |
| Eight connections | 927.2062 ms | 55 / 51,544,064 transfer bytes | 518.3607 ms | 21.2255 ms |
| Sequential | 83.8555 ms | 9 / 4,800,512 transfer bytes | 49.7459 ms | 32.9127 ms |

The split case's two linked disk-flush commands begin 905.9328 ms into the native call. That preceding interval is not idle by inference: numerous linked write requests occur there. The all-request interval unions are 539.5862 ms split and 82.6586 ms sequential. These are attributed request intervals, not exclusive device execution time or proof that every byte belongs to the body file. They point toward substantial pending writeback in this split sample, while leaving request gaps, filter/filesystem work, unrelated activity and instrumentation effects unresolved. The remaining writeback experiments must preserve durability and be judged on complete App/IDM completion, not merely shorten the reported flush.

Both [split](validation/scheduling-filter-8.65/app-split-phase.json) and [sequential](validation/scheduling-filter-8.65/app-sequential-phase.json) records pass the scoped native/disk/scheduling join. The sequential join initially rejected a duplicate switch-out. Inspection located it about 230 ms after flush entry, after both the 84 ms native call and the measured thread's TDH End. A record retained for another target thread must not extend this thread's lifetime merely because an old-thread field matches its ID. The corrected model filters this thread's records by its own known lifetime; malformed records inside that lifetime still fail.

The filtered fixture also ends with a recorded TDH Thread End after a witnessed post-call switch-in; its terminating switch-out is excluded after the collector removes the thread. The analyser explicitly accepts that closed terminal boundary only with a unique start/end and post-call witness. It synthesizes no off-CPU interval and leaves the unfiltered profile's stricter terminal requirement intact.

There are now **43 App-phase controls** and **56 prefix controls**, including write-request unions, transfer counts, partial-failure reporting, closed terminal boundaries and exclusion of foreign-lifetime records. Native/disk fields can remain available after a scheduling failure, but a failed join never becomes a pass on that basis. Full-schema, kernel-cause and performance-parity flags remain false. No production behavior, version, installed app or IDM settings changed.
