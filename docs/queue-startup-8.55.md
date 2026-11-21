# UDM 8.55: scoped queue rollback and startup work

Native source is **8.55**; both extension source manifests are **0.72.19**. Browser JavaScript is unchanged. Five native products built successfully. Installed desktop remains **8.32**; installation is deferred, with no setup creation, extension deployment, push or release.

## Cause and implementation

The desktop invokes `Manager::startQueuesOnStartup` after creating its frame and before constructing its pipe server. Previously that method deep-copied every job JSON for rollback even when no queue opted into startup. Manual Start and Stop copied the entire history too. With 7,500 completed records and 2 KiB descriptions, the allocation and copying were measurable work on the startup path.

Startup now checks the strictly boolean `StartOnStartup` options first. With no opted-in queue it performs the once-per-session transition and still updates scheduled wake timers, without copying job rows. Otherwise rollback snapshots are limited to the matching queue names and potentially modified rows. Completed nonmembers can retain that same queue label; those passive rows no longer need a backup. Current members and preparing prefetches remain covered. Explicit Start also retains backups for passive rows with `ServerRetryRequiresReview`, because that operation clears the field even on nonmembers.

Rollback stores job pointers with their original JSON and restores by swap. A failed startup save now also restores `schedulePaused`, closing a reproduced transient pause-hint leak. Existing queue state, completion epochs, retry behavior and persist-before-worker-cancellation ordering remain. The implementation still scans jobs for queue commands and retains existing queue preparation; it does not make those operations constant-time.

## Qualification

All **593 checks across 13 final-binary-matched runs** pass, including **176 queue snapshot checks on each of C: and D:**. Cases cover startup/Start/Stop, real write refusal, failures after flush, preservation of unrelated current data, same-queue passive history, review-field rollback, stopped-worker ordering, retry after refusal, wake timers, once-per-session startup, synchronization membership/replacement, queue completion and live scheduled changed/unchanged checks. [Suite records](validation/queue-startup-8.55/suite-results.json).

The expanded fixture initially reused synchronization mode on the reserved Main queue. A separate control against qualified 8.54 confirms that existing policy correctly refuses that switch. The managed-wake fixture now explicitly uses download mode and an owned destination. Production policy was unchanged. Initial compilation fixes, the refused fixture run and superseded intermediate qualification are recorded separately and excluded from the final check count. [Validation notes](validation/queue-startup-8.55/validation-notes.json).

The final build uses **512 frozen inputs and 42 core objects**: Queue and Bridge rebuilt, twenty-two qualified native and eighteen external objects retained. Include-closure checks limited the later scope refinement to Queue and the two changed diagnostic/fixture entry points; every output was relinked and all seventeen candidate file versions verified as 8.55. The exact eight-input delta from qualified 8.54 is verified in both directions, including per-side line endings. [Build evidence](validation/queue-startup-8.55.json), [source reconstruction](../benchmarks/speed/experiments/queue-startup-8.54-to-8.55.json).

A real ordinary 4 MiB download made one request, matched its independently generated length/hash and Complete catalog digest, exited normally after owned worker work and passed a separate output flush. Its payload was removed. Concurrent correctness work may have been running; this is an integrity/lifecycle smoke, not a finishing-speed comparison. [App record](validation/queue-startup-8.55/app-4m-smoke.json).

## Measurements and limits

The API control uses the same controller body linked separately against qualified 8.54 and current 8.55 engines. The baseline entry's version directive alone is pinned to its accurate engine label. Three balanced process pairs (A/B, B/A, A/B), with five samples per case per process, provide fifteen samples per engine per case. Synthetic catalogs contain 0, 2,500 or 7,500 completed nonmembers with 2 KiB descriptions and two selected paused jobs. Manual Start/Stop use a real catalog batch that defers persistence. The startup applied-once flag is reset between API trials. These timings measure that operation phase rather than full durable queue commands.

At 7,500 history rows, median milliseconds were:

| API phase | 8.54 | 8.55 |
| --- | ---: | ---: |
| no-startup | 39.1722 | 0.0132 |
| batch-start | 46.6372 | 11.9198 |
| batch-stop | 44.1881 | 9.4160 |
| main-batch-start | 60.0014 | 28.4205 |
| main-batch-stop | 46.8907 | 12.4971 |

`main-` cases retain the selected queue's label on passive history; other cases retain Archive. On empty history, manual operations were slightly slower by roughly 0.002–0.003 ms; all raw observations and thread-cycle counts are retained. No universal speed-win claim is made. Failed-write controls reproduce the old pause-hint leak and verify its absence in the new engine. [Full API comparison](validation/queue-startup-8.55/api-comparison.json).

Six real hidden desktop launches use one identically reseeded 7,500-record catalog and isolated instance tags. Baseline and candidate diagnostic App entry sources are byte-identical. Their only production-entry additions are two queue-stage timing markers and guarded normal self-close after initial refresh. Each process exits normally and preserves every history row. The median queue stage is **46.3258 ms → 0.0468 ms**; the marker just after configured pipe construction is **693.7648 ms → 627.8318 ms** from app initialization. Three samples per engine, warm OS caches and other startup work limit interpretation. Pipe construction return is not external request acceptance; hidden startup is not visible paint, dialog latency, installed acceptance, reboot-cold startup or an IDM comparison. [Startup results](validation/queue-startup-8.55/startup-comparison.json), [diagnostic patch](validation/queue-startup-8.55/startup-diagnostic.patch).

## Cleanup and next steps

Removed **677 owned generated files / 352,091,647 logical bytes**, plus the separate 4 MiB payload, and 608 empty owned directories. Per-file boundaries, reparse and complete hard-link-group checks precede deletion. Cleanup caches canonical roots instead of repeatedly resolving every root for every file. Five final products, two new core objects, forty shared core references, resources, compact evidence, prior qualified references, shared media tools, personal catalogs and installed executable are preserved. Logical bytes are not a physical disk-reclamation claim. [Cleanup receipt](validation/queue-startup-8.55/cleanup.json).

Next investigate synchronous catalog parse/load/recovery and the constructor's per-row JSON ownership copy, retaining failure rollback and real catalog identity. Full queue persistence remains caller-blocking and is not timed by the batch API control. Other open work includes outer recursive callers, installed/browser acceptance, zone metadata policy and the original IDM finishing/flush delays. This increment does not establish a kernel cause for those flush stalls or solve every IDM parity gap.
