# Finishing wait attribution, 8.63 / browser 0.72.27

Artifact availability update: the subsequent [8.65 cleanup](accepted-catalog-text-8.65.md#cleanup-and-older-artifact-retirement) retires ten 8.63 product executables, five product entries, App.res and the unused Bridge object. Shared Transfer objects required by the 8.64 baseline remain. The historical qualification below stays unchanged; its product hashes do not imply present availability.

UDM now records the waits that the earlier finishing aggregate could not separate. This is an observability improvement, **not a latency fix or a faster-build claim**. The existing flush thresholds, cancellation behavior, required final durability barrier and publication ordering remain. Normal products still use synchronous writes; ordered IO remains a private qualification path.

## Recorded boundaries

`PrefinishWriteback` now retains:

| Field | Meaning |
| --- | --- |
| LongestFlushSeconds | Longest completed optional flush callback in this transfer |
| FinishWaitSeconds | First teardown's elapsed stop/notify/join/stat synchronization time |
| InFlightAtFinish | Whether an optional flush callback was already running when teardown acquired its lock |
| DiscardedPending | Whether a coalesced, unstarted optional request was pending at that boundary |

Existing Flushes, Errors, Seconds, Adjustments and LeadBytes remain. A repeated finish/destructor preserves the first teardown's measurements. DiscardedPending is a boolean for one coalesced request; it is not a count of milestones or discarded file bytes. No received data is discarded by this diagnostic.

Each HTTP generation clears stale `HttpJoinWaits` before work. After settlement, the record contains:

- ReceiverThreadsSeconds: time spent in the loop joining receiver threads. This starts after thread launch and **includes normal live reception**, so an eight-second value is not an eight-second finishing stall. Use the existing last-byte/worker-joined timestamps to locate the post-reception part.
- OrderedSettlementSeconds: ordered pause/finish, complete-buffer adoption and its diagnostic publication, or zero when ordered IO is inactive.
- PrefinishFinishSeconds: the same first-teardown wait reported by FinishWaitSeconds above.
- PrefixHasherSeconds: elapsed prefix-hasher stop/join, captured before taking the Manager lock to store the result.

These are scoped call durations, not a disjoint decomposition of wall time. Background flush service can overlap reception. A slow longest flush is not proof that all of it delayed finishing; InFlightAtFinish and FinishWaitSeconds distinguish that case. No wait is shortened or a thread detached just to improve a displayed result.

## Build and affected checks

Both manifests advance to 0.72.27; all native version definitions advance to 8.63. [Build](validation/join-waits-8.63/build-result.json) and [normal-build](validation/join-waits-8.63/default-build-result.json) receipts cover 524 frozen inputs, ten products, and 43 distinct core objects across the two variants. [Dependency audit](validation/join-waits-8.63/native-closure-audit.json) requires Transfer and Bridge rebuilds, retaining forty qualified core objects. Transfer is additionally compiled without the private define for normal products. Five product entry objects are retained. All [fifteen executable resources](validation/join-waits-8.63/resource-versions.json), including fixtures before removal, report 8.63.

All **696 selected assertions in ten runs** pass on C: and D: — 58 prefinish, 56 ordered-writer, 49 Manager disk-credit, 145 completion-checkpoint and 40 catalog-checkpoint checks per drive. [Suite record](validation/join-waits-8.63/suite-results.json). These are the affected checks selected for this increment, not a rerun of all earlier 8.62 suites.

Seven new prefinish assertions cover idle teardown, a deterministically entered teardown with a held callback, queued-request discard versus in-flight work, finite nonnegative waits, longest-callback accounting and immutable repeated-finish evidence. The test uses a locked friend accessor to confirm teardown entered before releasing the held operation, rather than relying on a sleep to infer that ordering. Existing failure, coalescing, pacing and real-file flush checks continue to pass.

## Actual App samples

Four ordinary 64 MiB downloads pass independent exact size/SHA-256, catalog, normal process exit and separate post-exit output flush checks. Sequential samples use the normal product; split samples use the private candidate. These are single correctness/diagnostic samples per case, **not balanced performance comparisons**.

| Drive / case | Optional flushes | Longest callback (ms) | Teardown wait (ms) | Ordered settlement (ms) |
| --- | ---: | ---: | ---: | ---: |
| C: sequential | 3 | 36.496 | 0.209 | 0 |
| C: split | 1 | 4.801 | 0.146 | 6.946 |
| D: sequential | 3 | 179.090 | 0.110 | 0 |
| D: split | 1 | 112.793 | 0.124 | 5.963 |

All four report InFlightAtFinish false. The long earlier stall did not recur, so these samples cannot attribute its cause retrospectively. They establish that a callback taking 179 ms can finish before teardown, which then waits only around 0.1 ms. The new records can distinguish that from a genuinely outstanding flush in a future slow run. [App records](validation/join-waits-8.63/app-smoke.json).

## Cleanup, preservation and next work

[Cleanup](validation/join-waits-8.63/cleanup.json) removes 176 generated files / 40,851,717 logical bytes in fourteen owned fixture roots, including fixture executables and entry objects. All four 64 MiB payloads, another 268,435,456 logical bytes, were independently checked and removed immediately after their samples. Deletion scopes resolved paths, refuses reparse traversal, verifies hashes and accounts for complete owned hardlink groups. These are not physical allocation or disk-wear measurements. No source/toolchain/FFmpeg copies were created; the six prior changed inputs are retained as small compressed snapshots for provenance.

[Final verification](validation/join-waits-8.63/final-verification.json) rechecks the inputs, ten products, 43 core objects, five product entry objects and three protected installed/personal files. The [six-file source delta](../benchmarks/speed/experiments/join-waits-8.62-to-8.63.patch) reconstructs qualified 8.62 inputs and passes reverse-application checking.

Installed UDM remains unchanged. No installer, deployment, extension reload, administrator prompt, push or release occurred. The private ordered path still fails the earlier no-regression promotion decision; this increment does not override that evidence.

Next, capture these boundaries in a slow full-lifetime run and distinguish outstanding optional IO from receiver/writer or publication-journal blocking. Only then change the operation responsible, advance the next native/browser versions and compare complete lifetimes again, retaining every slower case. Installed IDM/browser parity and the wider feature/design objective remain open.

The subsequent [8.64 pinned catalog reader](pinned-catalog-read-8.64.md) removes redundant pathname work in completion/shutdown proofs and measures a component gain. It does not resolve the earlier long-stall attribution or promote ordered writes.
