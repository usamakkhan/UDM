# Ordered range writer: component and integration plan

Status: [8.62 native ordered-body candidate](ordered-body-8.62.md), source manifests 0.72.26. The ordered network path now runs in a private qualification build; normal products remain synchronous. Shared storage is qualified, native root checkpoint capture/retirement is integrated, and actual ordered completion, retry and pause/reload/resume controls pass. Storage-fault/operation races, adversarial response cases, child bindings and balanced performance/promotion gates remain open. Installed UDM remains 8.32; no installer, installation or privileged worker was used.

## Problem and intended behavior

The 8.58 observations locate a 564 ms wait inside the final native flush, and long first writes at offsets far beyond the valid written prefix. The gap-extension control identifies avoidable regular-file work, but both sparse candidates lost the overall promotion gate. See [the retained measurements](finishing-flush-attribution-8.58.md). This component explores a different path: receive ranges independently in bounded memory and write ordinary-file bytes in ascending order. It does not establish a faster download or eliminate the final flush.

The [prototype header](../benchmarks/speed/ordered_range_buffer.hpp) separates accepted byte intervals from successfully written intervals. Incoming disjoint ranges are copied into stable owned storage. The IO thread selects only a contiguous written-prefix extension during normal operation. Received tails remain buffered until their gap closes. Incoming identical overlaps are checked without rewriting bytes that the IO thread may be reading; conflicting bytes are refused before mutation. Coverage metadata is bounded to 4,096 intervals, and the body to 64 MiB.

The storage callback runs outside the buffer mutex. It must finish every requested byte or throw, including on partial writes. The component prepares its next credit map before IO, then swaps that prepared map into place after success; a later allocation cannot erase an earlier acknowledgment. A failed operation receives no credit even if it physically changed part of the file. The real storage adapter must check both WriteFile status and the exact byte count.

## Close, pause and resume-credit contract

| Operation | Queued gaps | Publication eligibility | Resume credit |
| --- | --- | --- | --- |
| finish | Only contiguous work is written; remaining gaps cause incomplete status | Successful finish requires complete written coverage and a joined writer | Contiguous written prefix within each requested segment |
| pause | Accepted noncontiguous spans are drained in order | Paused state is not completion | Each acknowledged segment prefix; gaps are never counted by summing spans |
| abort | Stops accepting data and starts no new writes | Always closed to completion, including when all bytes happened to be written | An operation already in flight is credited only if it actually succeeds |
| write failure | Stops the generation and propagates to producers | No completed body | Only prior successful operations; partial failed operations are not credited |

One controller chooses the close policy. Repeated closure does not change an already settled policy or issue more IO. Snapshot/put are synchronized with that controller; concurrent close calls are not supported. The writer cannot close and join itself. The destructor aborts and joins before storage destruction. A callback and any file handle it uses must remain alive through that join. An arbitrary blocked OS write cannot be made safe by detaching its thread or freeing its buffer after a timeout.

Written means that the storage callback returned success. It does **not** mean power-loss durability. The component performs no final file flush; tests explicitly retain the caller's FlushFileBuffers operation. Integration must retain the existing final barrier and publication/recovery ordering.

## Earlier isolated-budget component validation

The final binary passed **44 checks on C: and 44 on D:**. The earlier 37-check in-memory run is recorded by its private console log/build hashes and is excluded from the final 88-check aggregate. Final cases include:

- Tail-first admission with no write across its missing prefix, ascending window order and exact complete bytes.
- Buffered-only data refused as resume/completion credit, incomplete finish, and fixed close policy.
- Pause drain of a noncontiguous tail and per-segment prefix clipping.
- Abort while a callback is held, admission closure before join, acknowledgment of the already issued successful operation and no subsequent writes.
- Identical retries while IO is held, conflicting-byte refusal and exact output preservation.
- Failed/partially failed writes retaining earlier credit without exposing a complete body.
- Eight concurrent producers, size/offset overflow checks, zero-length admission, the earlier exclusive prototype budget and successor reuse, and atomic refusal at the metadata bound.
- Safe rejection of a callback trying to close its own writer.
- Real regular-file positioned writes from byte zero after tail-first input, explicit caller flush, exact readback and a real paused-tail file with an uncredited leading gap.

The real files were independently checked against expected contents and SHA-256 before removal: 256 KiB complete output and an 8 KiB paused output on each drive. [Checks and hashes](validation/ordered-buffer-prototype/results.json), [C: cases](validation/ordered-buffer-prototype/checks-C.json), [D: cases](validation/ordered-buffer-prototype/checks-D.json). Twelve generated files including the test executable/object were individually scoped, link/reparse checked, hashed and removed: 4,421,434 logical bytes. [Cleanup](validation/ordered-buffer-prototype/cleanup.json). No native application source, qualified product, installed app or personal catalog changed.

## Integration gates, in order

### 1. Share one buffer and budget

Implemented in 8.60: `BodyBufferStorage` owns the byte vector and thread-independent atomic lease. Cache invalidation can transfer the allocation without copying, and the standalone writer refuses shared mutable ownership or the wrong size before starting IO. Only joined successful finish exports storage for hashing; the final reader retains the global token. Actual C:/D: checks cover pointer identity, last-reader destruction on another thread, occupied admission, unique-owner/size refusal and failed/incomplete export. Constructor failure uses RAII, but allocation-exhaustion injection is not claimed. The ordinary production disk fallback remains. [Details and limits](shared-body-storage-8.60.md).

The earlier isolated-budget prototype had 88 checks. The shared-storage component now passes 94 separate checks (47 per drive), with additional native cache/ownership checks included in the 1,303 native aggregate. Neither aggregate enables ordered network IO or closes the remaining integration gates.

### 2. Add a generation-bound disk-credit view

The earlier [standalone checkpoint adapter](disk-credit-checkpoint-plan.md) established the 148-check research contract. [Native 8.61 integration](native-disk-credit-8.61.md) now connects root catalog capture, active-generation/membership validation, persisted-view dirty checks and transfer retirement on the existing synchronous cache-eligible path. Actual Manager held-save/admission/reload tests and the real App smoke pass. Full buffered reception, child ownership and actual cancellation/refresh/folder/reset/removal/rollback fault coverage remain open; this gate is not complete for ordered network IO.

Currently `Job::snapshot` copies `data` and child tracks, `Manager::snapshot` collects those snapshots, and `saveCatalog` adds the admission/publication overlays before serialization. `Manager::progress` advances Segments.Done, Received and TransferredBytes together after the synchronous write. Buffered reception must not make those persisted Done counts describe RAM-only data.

Add a runtime-only writer binding to the exact active job/cancel generation and file identity. Separate received/read cursor from written credit. A persistent snapshot must clip each segment's Done to its acknowledged contiguous written prefix and recompute saved Received, while preserving actual transferred-byte/quota accounting. Do not serialize the writer object, pointers, RAM-only coverage or a placeholder claiming cached bytes are on disk. Audit every snapshot consumer: a UI status view, export/rollback state and a catalog checkpoint do not necessarily want the same Received representation. Avoid silently changing every Job snapshot into a disk-only view.

Capture the credit view under the existing catalog lease, and bind it to that serialized snapshot through coordinator acceptance. Later writes may advance credit; an older snapshot remains a conservative lower bound. A reset or new representation must not clear/reuse the file while an old-generation checkpoint can still be accepted. Keep the existing reset-pending and publication ownership proofs; test a held old checkpoint against reset, cancellation, folder changes, a newer save and failure rollback.

### 3. Preserve range-response ownership

The 8.62 private native path now uses accepted progress for network cursors and reads retry overlaps from exact accepted buffer bytes. Actual slow-prefix, 22 dropped-connection and pause/reload/resume controls pass. Default activation still awaits the adversarial and lifecycle cases listed in the [candidate report](ordered-body-8.62.md); the original standalone validation below is historical.

[Actual Manager response qualification](ordered-responses-8.62.md) now adds 24 cases / 126 native assertions on C:/D: for one-use/no-range responses, capacity/helper refusal, wrong ranges/tags, truncated bodies and a held-save representation change. The original full body is retained in fallback cases; the new representation response is reused after old-save settlement. This closes those response cases, while direct refresh/folder/removal/shutdown races and balanced performance remain open.

Keep the original full response for one-use URLs, and retain validated helper admission, exact Content-Range/identity checks, capacity handling and sequential fallback. Network read position may advance ahead of written position, so range claims/splits must use admitted/queued ownership rather than steal queued bytes. Each in-flight read and accepted span must stay inside its assigned representation and generation.

Overlap and boundary validation must obtain the exact accepted buffered bytes when their disk write is pending, or wait for a proven written region before reading the file. Reading a preallocated but unwritten prefix would compare zeros and create false change failures. A conflicting retry is a representation error, not permission to overwrite a buffer currently used by IO. Test shortened ranges, pending reads, stale keep-alive retry, helper denial, a primary response advancing past a proposed split, resource changes and one-use refusal.

### 4. Close every asynchronous error and cancellation path

[Whole-Manager 8.62 storage-fault qualification](ordered-storage-fault-8.62.md) now passes 126 assertions on C:/D: for failed/corrupt-partial/short writes, early/EOF holds, pause persistence rollback and full recovery. This closes those scoped IO cases; the broader refresh/folder/removal/direct-shutdown and child-operation gates remain open. The held-save representation-change/reset case subsequently passes in the response qualification.

[Direct shutdown qualification](ordered-shutdown-8.62.md) subsequently passes 20 assertions in four C:/D: processes: a held detached save blocks stop, shutdown joins ordered writes, every saved prefix is independently verified and fresh-process resumes complete exact files. Other shutdown/save-failure orderings and refresh/folder/removal races remain open.

[Operation lifecycle qualification](ordered-lifecycle-8.62.md) subsequently passes 62 assertions in ten C:/D: processes for active/retiring folder, refresh-address and removal refusal, actual operation-save rollback and complete fresh-process recovery after inactive edits. This closes those scoped root operation boundaries; broader fault interleavings and child/media bindings remain unproven.

A storage failure must notify the parent cancellation group outside the writer mutex so HTTP workers do not remain blocked in unrelated reads. Preserve the original failure and exact successful write credit. Producer-admission failure must not credit the rejected chunk; choose a tested drain-and-fallback transition or fail safely with retained parts.

Pause/network failure/shutdown need an explicit preservation policy. The prototype's pause drains accepted spans even across gaps; this may incur conventional gap-write cost during pause, but avoids silently dropping already received data. User removal can abort when its existing policy discards data. Before removing the runtime overlay, conditionally reconcile live Done/Received with the written map under the model lock. Wait for the writer before closing/moving/deleting its file or releasing its buffer. Test a partial write, failure after an earlier commit, held IO, failed final save, nested cancellation, remove, stop, reset and a newer owner replacing the old binding.

### 5. Retain truthful progress and finalization

Charge actual network bytes once. A later writer acknowledgment must not increment TransferredBytes or quota again. UI received progress, speed and prefetch displays may describe accepted bytes, but recovery progress must describe written bytes. Feed optional prefinish writeback from acknowledged disk-written progress, not received RAM bytes; otherwise its threshold can fire while most buffered bytes are still unwritten. Join the writer before finishing the writeback worker and before content verification, final hash/publication or cleanup. Only complete acknowledged storage may supply the cached advisory body. Preserve the mandatory final flush and exact final checkpoint.

Keep model-lock/file-IO separation: snapshot may take the buffer state lock while holding the model lease, but the IO thread must never hold that buffer lock while entering Manager callbacks. Test reentrant snapshots and held IO explicitly.

### 6. Qualify and decide using the complete result

[The first balanced 36-transfer comparison](ordered-comparison-8.62.md) completes correctly but does not pass the no-regression promotion decision. Split wall medians improve on both drives, while C: finishing tail regresses and one split pair is slower overall. Keep the private gate; investigate optional writeback/join and publication IO without dropping final barriers. Installed IDM/browser timing remains pending.

Start with the existing cache-eligible fresh plain-HTTP path, retain the legacy path for denied admission, and do not claim larger/unknown/resumed/media cases are optimized. Complete all affected resume, publication, cache and lifecycle fault cases before promotion. Then compare balanced uninstrumented whole-lifetime cases, retaining process exit and a separate output flush, every slower result and origin byte/request accounting. Include installed UDM/IDM and browser workflow checks when deployment is resumed. A faster body write or shorter final flush alone is insufficient: both rejected sparse experiments showed why.

Continue advancing native and both extension versions for coherent product improvements, build the matched products, and record installation/publication separately. The shared-storage 8.60, root checkpoint 8.61 and gated HTTP 8.62 increments advance the mechanism. Remaining fault/adversarial/performance gates and the original IDM parity goal are not closed by these selected checks.

The subsequent [8.63 wait-attribution increment](join-waits-8.63.md) separates receiver, ordered, optional-flush and hasher waits without changing IO policy. Its 696 selected checks and four actual App diagnostic samples pass; the long stall did not recur. The prior no-regression promotion failure remains authoritative.
