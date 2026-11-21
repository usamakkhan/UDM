# Why 9.62 did not actually overlap final publication IO

Fifteen private App diagnostic downloads isolate a repeatable source-query wait. All final bytes and saved Complete records verify, all mapped trace records pass readiness/PID/frequency/zero-drop checks, and all payloads are removed. These are instrumented attribution builds, not normal-product performance qualification or an IDM comparison. The installed app is unchanged.

## Same-binary control

One private 9.62 binary runs the owned body path and the original serial body path in opposite orders over two trials of 4 MiB sequential, 64 MiB sequential and 64 MiB split. Only a private runtime gate selects the final writeback path; it is absent from production source. Core, Queue, Transfer and Duplicates are compiled with consistent private headers. The App entry and all other code remain identical between both modes.

In all six owned-path runs, the body writeback interval ends **before** durable intent writing starts. The intended overlap is zero. Source preparation consumes 22–89 ms in the sequential observations and 71–84 ms in split observations, while the corresponding serial preparation takes about 0.18–0.21 ms. The owned writeback had already started, so a file metadata query in preparation waits behind it.

Final accepted-file release takes approximately 0.03–0.04 ms in the paired batch. It does not account for the observed tens-of-milliseconds stalls. Worker join, source query, intent, catalog and shutdown spans share one QueryPerformanceCounter clock; overlapping work is not added as serial time.

## Exact query follow-up

A second diagnostic epoch keeps the frozen App entry, Core, Queue and Transfer objects and instruments only the individual source queries in Duplicates' preparation header. Three further owned-path downloads show:

| Case | Body writeback | GetFileInformationByHandle | GetFileSizeEx |
| --- | ---: | ---: | ---: |
| 4 MiB sequential | 34.035 ms | 33.874 ms | 0.004 ms |
| 64 MiB sequential | 85.487 ms | 85.346 ms | 0.004 ms |
| 64 MiB split | 158.635 ms | 97.491 ms | 0.004 ms |

All three again have zero overlap between body writeback and intent writing. The split worker also spends about 59 ms finishing its independent advisory hash from the retained write buffer before it reaches publication. The source query then waits for the remaining flush interval. No kernel disk, filesystem-filter or scheduling cause is inferred from these user-mode intervals.

Named-source validation appears cheap in these observations because the constructor's earlier query has already waited for the flush. Removing only that first query would not establish that the later file query cannot serialize behind a still-running flush. The fix must retain the same-object proof without querying the active body before intent, and still revalidate the actual named file after the owned join.

## Scope and limitations

The first instrumented launch reaches the first App trace marker at 286.5 ms; later launches reach it at about 43–49 ms. That first marker follows trace initialization, so this batch cannot assign the extra delay to the native loader, antivirus or production startup. The slow first observation remains in the record. The origin also retains complete per-request ranges, timestamps, sent bytes and final connection retirement; that observer retirement is outside the app's measured lifetime.

The 12 paired runs use one binary. The three per-query runs use a separately hashed epoch with the same frozen entry objects. Extra markers and a mapped trace file affect these binaries, so their times do not replace the normal 9.62-versus-9.60/IDM comparison. All 15 raw traces are reconstructable from their saved header, used event bytes, zero tail length and SHA-256. A preflight path-type error happened before any compilation or transfer and is recorded separately.

## Required implementation

Capture file information and stable identity **before** launching asynchronous flush, on the same object that excludes other writers. Carry that capture in a move-only capsule that callers cannot construct from arbitrary metadata. Prepare the intent from that capture, then join the owned body and verify the actual source name and identity before rename. Bind recovery to both source identity and digest so a foreign replacement with identical bytes is preserved rather than moved. Preserve forced body flush, exact catalog acceptance, cancellation, copied/replacement paths, unknown-hash fallback and full worker lifetime.

This implementation is being built as 9.63 / source extension 0.73.27. Its success and speed must be established independently; the diagnosis alone does not prove a fix or full IDM parity.

[Same-binary and query findings](validation/publication-attribution-9.62/analysis.json), [paired inputs and controls](validation/publication-attribution-9.62/paired/inputs.json), [per-query epoch](validation/publication-attribution-9.62/per-query/inputs.json), [diagnostic status](validation/publication-attribution-9.62/status.json).
