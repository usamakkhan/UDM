# Ordered HTTP body candidate — UDM 8.62 / extension 0.72.26

## Delivery boundary

Both extension source manifests advance to 0.72.26 and native definitions/resources to 8.62. Five normal products and five private qualification products are built. **Normal builds retain synchronous disk writes.** The new ordered HTTP path requires the in-process compiler define `UDM_ORDERED_BODY_QUALIFICATION`; no browser/IPC preference enables it. It is not installed, published or promoted as an IDM speed fix.

The private candidate now integrates the real network path, shared body storage, disk-credit catalog leases, range retry checks and final hashing. This moves beyond the earlier standalone writer. Remaining lifecycle/fault and balanced performance gates below must pass before default activation.

## Implemented data flow

`OrderedBodyWriter.hpp` holds one existing bounded cache allocation (at most 64 MiB), accepted intervals and acknowledged written intervals, each bounded to 4,096 entries. The HTTP worker copies accepted bytes into stable storage and advances its accepted/read cursor using the existing ownership and in-flight boundaries. Normal IO emits only an ascending contiguous prefix, in 256 KiB windows. Queued tails wait in memory rather than extending the file across an unwritten gap. This does not change regular-file reservation or the final flush policy.

The storage callback runs outside the buffer/model locks. It obtains the generation-bound disk-write lease, performs the exact positioned write and acknowledges it, then captures the written sniff prefix and advances optional prefinish writeback from **written**, not network-received bytes. Storage failure is recorded before notifying the cancellation group outside the buffer mutex. Notification exceptions cannot replace the original failure. The controller joins network workers and the ordered writer before writeback/hash/publication handling. No detached IO thread outlives its bytes or file.

The old full-body cache transfers its allocation into the writer, then accepts that same uniquely owned allocation back only after joined successful complete disk coverage. No second body-sized allocation or completion copy is introduced. Failed/paused/aborted/incomplete state cannot supply the completed hash body. Allocation/constructor admission failure keeps the existing synchronous fallback; no allocation-exhaustion injection is claimed.

### Range ownership and retry bytes

The original full response, helper admission, validator/Content-Range checks, live split boundaries and one-use response reuse remain. The existing in-flight span protects the interval while RAM copying happens outside Manager::mutex; queued progress becomes the network cursor after admission. Conflicting accepted overlaps produce a typed representation error before mutation.

A reconnect with accepted progress verifies its overlap against the exact accepted RAM bytes, even when their disk write is pending. It never reads zeros from a preallocated unwritten region as saved content. Adjacent-range digests still receive the existing completed-boundary validation after the writer joins. Ordered workers no longer open unused per-response output handles; only the storage thread needs the ordered output handle.

### Accounting, checkpoints and cancellation

Ordered reads charge actual decoded network bytes once, including reads whose later admission fails. `Manager::progress` has an explicit already-accounted mode that advances accepted progress and worker cursors without charging those bytes again; legacy calls retain their default accounting. Disk acknowledgment does not increment quota or transfer counters a second time.

The 8.61 catalog leases persist only acknowledged disk prefixes while Job/Manager UI views remain live. A paused/network-failed generation drains already accepted spans, including gaps, before retirement reconciles its live resume counters. The explicit AcceptedBuffer write intent preserves owned queued bytes after cancellation; it cannot change the generation or admit a new network operation. Ordinary new-write admission remains canceled. The native writer's destructor also joins/drains accepted data during controller unwinding; explicit abort remains available to a controller choosing discard.

Per-generation ordered/disk-credit diagnostic fields are cleared before each HTTP generation. A resumed legacy generation therefore does not inherit a previous paused generation's usage record. This fixes the stale diagnostic exposed during qualification. Final explicit file flushing remains mandatory; a callback acknowledgment alone is not a power-loss durability guarantee.

## Final evidence

[Selected native suites](validation/ordered-body-8.62/suite-results.json) pass **1,780 checks in twenty final-binary-matched runs**. Earlier intermediate passes are excluded. Cases include:

- 56 native ordered-writer checks per drive: accepted-vs-written coverage, gaps, identical/conflicting overlaps, reentrant failure notification, pending-IO retry reads, abort/pause/failure closure, metadata bounds, real-file flush/readback and destructor preservation.
- 106 cache checks per drive: the original cross-thread/token checks plus actual native-writer handoff, exact allocation identity, final hash-cache adoption and alias/size refusal.
- 76 native disk-credit and 49 actual Manager integration checks per drive, including preservation after cancellation, stale-generation refusal and one-time buffered/legacy accounting.
- Existing prefix/writeback, checkpoint, catalog/admission/coordinator, publication and recovery suites.

[Private build](validation/ordered-body-8.62/build-result.json): 524 frozen inputs, 24 native core units rebuilt after the progress API change, eighteen qualified C codec objects retained, five products and thirteen fixtures. [Normal build](validation/ordered-body-8.62/default-build-result.json) compiles the only source affected by the private define without it and links five normal products using unaffected shared objects/entries. The macro-use audit is recorded; the two variants share 41 of 42 core objects, so only 43 distinct core objects are retained. All 24 recorded executable resources, including the separate HTTP controller, report 8.62.

### Actual application checks

- [64 MiB private App smoke](validation/ordered-body-8.62/app-64m-ordered-smoke.json): OrderedBodyWrites Used/Complete are true and AcceptedBytes/WrittenBytes/ReconciledBytes are exactly 67,108,864. The same buffer supplies advisory hashing. Exact length, independently expected SHA-256, catalog completion, normal process exit and a separate post-exit output flush pass. Observed exit/durable tails are 655.50295/678.9825 ms; this single correctness run is not a balanced speed comparison.
- [8 MiB normal App smoke](validation/ordered-body-8.62/app-8m-default-smoke.json): the same lifecycle/hash checks pass on the normal synchronous path. Disk-credit binding is used, and no ordered-generation record appears. The private define is absent.
- [Actual Manager HTTP controls](validation/ordered-body-8.62/http-results.json): a slow-prefix 16 MiB transfer pauses with 1,146,880 credited bytes. Every saved segment prefix independently matches the synthetic source pattern; saved counters match the real catalog. Reload/resume completes the exact 16 MiB file and SHA-256, with no stale ordered record after that legacy resume. A separate 16 MiB case recovers from 22 scripted connection drops, completes joined ordered writes and matches every final byte/hash. Both finish through normal process stop/exit and a separate output flush. Origin request/byte/drop details are retained; no request-count reduction or performance claim is inferred.

The first HTTP driver forgot to tick the queue and ended at its bounded deadline with zero requests. Its [failed result](validation/ordered-body-8.62/rejected-controller-result.json), [origin/failure record](validation/ordered-body-8.62/rejected-controller-failure-private.json) and [build identity](validation/ordered-body-8.62/rejected-controller-build-result.json) remain separate. That driver failure was corrected before the successful real network cases. Initial candidate passes before removing unused handles/resetting diagnostics also remain historical, excluded from the final aggregate.

## Cleanup and verification

[Cleanup](validation/ordered-body-8.62/cleanup.json) removed 953 generated files / 243,830,430 logical bytes in 48 owned fixture roots, including the fixture/controller executables and entry objects. A further 142,606,336 bytes of smoke payloads were separately removed immediately after verification. Per-file hashes, hardlink groups and reparse checks scope the deletions. Logical bytes are not a physical freed-space claim. No FFmpeg or repository source tree was copied.

[Final verification](validation/ordered-body-8.62/final-verification.json) rechecks 524 qualified inputs, the separately frozen HTTP controller source, both sets of five products, 43 distinct core objects and three protected installed/personal files after cleanup. The [source delta](../benchmarks/speed/experiments/ordered-body-8.61-to-8.62.patch) reconstructs thirteen qualified prior input identities and passes reverse-application checking.

Installed UDM remains 8.32. No installer, installation, extension deployment, push, release or administrator prompt was involved.

## Remaining promotion gates

1. **Passed for scoped WriteFile faults:** [whole-Manager storage qualification](ordered-storage-fault-8.62.md) adds 126 assertions in fourteen C:/D: cases, including corrupt partial/short writes, early/EOF IO holds and failed pause-save rollback, with independent saved-prefix and full-recovery bytes/hashes. Other filesystem failures and operation races below remain open.
2. **Passed for scoped root operation boundaries:** [lifecycle qualification](ordered-lifecycle-8.62.md) adds 62 assertions for active/retiring folder, refresh-address and removal refusal, post-retirement save rollback and exact fresh-process recovery. Root pause/reload/resume, held-checkpoint representation-change/reset and [direct stop with held save and fresh-process recovery](ordered-shutdown-8.62.md) now pass. Other shutdown/save-failure orderings remain open; child/media buffered bindings are still excluded.
3. **Passed for twelve scoped response policies on each drive:** [HTTP response qualification](ordered-responses-8.62.md) adds 24 cases / 126 native assertions against unchanged core objects, independently verifying exact final bytes/hashes, original one-use body retention, denied/malformed helpers, truncated-body recovery and resource-change repair held behind an older save. Other adversarial policies and lifecycle gates remain open.
4. **Run complete; no-regression promotion failed:** [36 normal/private App transfers](ordered-comparison-8.62.md) retain process exit, post-exit flush, every slower pair and outlier. Split wall medians improve, but the C: median finishing tail regresses by 182.852 ms and one split pair is slower overall. Keep normal builds synchronous while investigating writeback/join and publication IO. Installed UDM/IDM and browser comparisons remain pending.
5. Only after those gates pass, enable the selected path in a new versioned normal build. Do not infer default activation, deployment or completion of the IDM parity objective from this candidate's checks.
