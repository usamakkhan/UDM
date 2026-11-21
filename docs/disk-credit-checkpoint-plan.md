# Ordered writes: generation-bound disk-credit checkpoint adapter

> Follow-up: [8.61 native root integration](native-disk-credit-8.61.md) now uses a native implementation in actual catalog saves and the synchronous cache-eligible transfer lifecycle. This document and its 148-check evidence describe the earlier prototype. Buffered network and child integration remain pending.

Status: standalone research code, **not used by production Job/Manager catalog saves or network transfers**. Qualified desktop/source stays 8.60 and both browser manifests stay 0.72.24. This test-only prototype is not a separately delivered product improvement and adds no product increment. Installed UDM remains unchanged. The full ordered-writer integration and IDM parity goal remain open.

## Why a projection alone is insufficient

`Job::snapshot` copies live data and child tracks; `Manager::snapshot` gathers those live views. `Manager::saveCatalog` prepares its detached catalog under the recursive manager mutex, then serializes it and enters `CatalogCommitCoordinator` outside that lock. A newer commit sequence prevents an older snapshot replacing a newer catalog, but a sequence alone does not bind the older snapshot to a still-valid file generation. Reset/truncation can invalidate bytes while the detached save is pending.

A global replacement of snapshot Received/Done with disk counts would also change UI/export/rollback consumers. The adapter instead projects one explicitly selected checkpoint row. Live network counters remain unchanged.

## Implemented contract

[`disk_credit_checkpoint.hpp`](../benchmarks/speed/disk_credit_checkpoint.hpp) binds a bounded known-size representation to an owning cancellation identity and an owning ordinary Windows file handle. Both pointer address and shared ownership identity must match; another shared-pointer control block at the same address is refused. The representation key preserves field presence and value for job ID, URL, total size, destination, ETag/modified/encoding, storage layout and reset/refresh state. Binding refuses a pending parts reset or unvalidated refresh.

A write obtains a lease before IO. It acknowledges only the exact successful requested operation; dropping the lease after a failed/partial write gives that operation no new credit. Credit-map preparation is transactional, bounded to 4,096 intervals, and may fail without changing earlier acknowledgments. That failure must eventually cancel the transfer through its real adapter; this standalone ledger does not notify Manager. Already-issued successful work can be acknowledged after cancellation; cancellation blocks new write admission.

A checkpoint validates bounded, integral, nonoverlapping segments with unique saved indices. Its Done is the minimum of live Done and that segment's contiguous acknowledged written prefix. Written islands beyond a gap do not count. Saved Received is recomputed from those disjoint prefixes. TransferredBytes and unrelated metadata are preserved. A Complete status with missing written credit is refused. Successful callbacks acknowledge writes, **not a power-loss durability barrier**; the existing explicit final file flush is still required.

Each detached snapshot owns a generation pin and a copied immutable row. Its older credit stays conservative while later writes progress. The pin remains with the real save through serialization, file replacement, result acceptance and exception unwinding. `tryRetire` is nonblocking and refuses retirement while a write or checkpoint pin exists. Once retired, that generation rejects all future captures/writes. Controllers must retry outside Manager::mutex, not wait while holding that mutex. The adapter never calls Manager while holding its own lock.

An outstanding snapshot also keeps the bound file owner alive if the controller is destroyed. A successor starts with zero credit even if the same physical file has its full previous length; preallocation or file length cannot establish received content.

## Actual qualification

The final binary passed **74 checks on C: and 74 on D:**. Earlier intermediate runs are excluded from the 148-check aggregate. [Results](validation/disk-credit-checkpoint-prototype/results.json), [C: checks](validation/disk-credit-checkpoint-prototype/checks-C.json), [D: checks](validation/disk-credit-checkpoint-prototype/checks-D.json), [build/source identities](validation/disk-credit-checkpoint-prototype/build-result.json).

The fixture links the qualified 8.60 core and uses actual `atomicText`, `CatalogCommitCoordinator`, Job and Manager APIs:

- RAM-only data, written tails and gaps, live Done clipping, quota preservation and unchanged live JSON.
- Changed generation, ownership block, URL/target/validator/layout/reset identity; malformed/overflow bounds, duplicate indices, overlapping segments and metadata bounds.
- In-flight write pinning, no new IO after cancellation, acknowledgment of prior successful IO and exact actual bytes/hash after the caller's flush.
- A newer actual catalog commit superseding an older detached snapshot, with the old capture retaining its pin until released.
- Both successful and failed real saves held after the temporary-file flush while reset is refused. Injected failure preserves the original catalog and durable sequence; later snapshots retain prior valid disk credit.
- Eight simultaneous producers issuing sixteen real 4 KiB writes, with serialized file positioning and concurrent ledger acknowledgment. Captured credit remains monotonic and conservative; the final file matches independently generated bytes/hash after flush.
- Actual Job and Manager UI snapshots remaining unchanged by a separate persisted projection.
- Controller destruction while a detached checkpoint still owns the file; final snapshot release frees that remaining owner.

The independently checked complete bodies are 64 KiB, avoiding another large performance workload. These tests do not measure download speed, the finishing delay or IDM parity. No allocation-exhaustion injection or full production cancellation/reset integration is claimed.

[Cleanup](validation/disk-credit-checkpoint-prototype/cleanup.json) removed all six owned fixture roots, the test executable/object and intermediate fixture files: 48 files / 8,510,746 logical bytes. Individual hash/link/reparse checks precede deletion. Sources and evidence remain. [Preservation](validation/disk-credit-checkpoint-prototype/preservation.json) rechecks two prototype inputs, 518 qualified inputs, 42 core objects, five retained products and three protected installed/personal files. No administrator prompt, installer, deployment, push or release was used.

## Next integration steps, in order

1. Attach a runtime-only binding to the exact active root/track job, cancellation generation and held parts-file identity. Validate the expected part path and Windows file identity at attachment. The prototype requires an owning ordinary handle; it does not resolve the catalog destination to a parts file or prove delete-sharing flags. Its fixture explicitly denies delete sharing. Controllers must never mutate/reassign the held handle or truncate the file outside the retirement protocol.
2. Add a dedicated persisted catalog preparation step in `saveCatalog`, retaining every selected checkpoint lease through coordinator completion and checkpoint-state acceptance. Keep Job/Manager UI snapshots unchanged. Handle child-track rows explicitly; merge admission/publication overlays without losing the generation pins. Adjust periodic dirty comparisons for this persisted view rather than interpreting normal RAM/disk lag as a saved UI-state match.
3. Route reset, refresh, folder changes, failed-save rollback, removal and shutdown through an explicit generation retirement/reconciliation policy before clearing/reusing bytes or detaching the binding. Refused retirement must be retried outside the model lock and respect cancellation. Test a held old checkpoint against each actual Manager operation, not just the adapter's guard. Finalization must join writes and install exact disk credit before removing the projection.
4. Wire network reception to separate read/queued cursors and successful disk acknowledgments. Preserve original-response ownership, queued overlap validation, helper admission and one-use fallback. Feed prefinish writeback from written progress; charge network/quota bytes once. Propagate write and ledger failures outside their locks.
5. After affected recovery/publication/lifecycle fault cases pass, run balanced whole-lifetime comparisons retaining process exit and separate post-exit flush. Record every slower case. Resume installed/browser/IDM checks only when deployment resumes, and increment desktop and both browser versions for the coherent production improvement.

This establishes the checkpoint-lease mechanism and its real catalog IO behavior. It does not close production gate 2 in [the ordered-writer plan](ordered-range-write-plan.md), nor activate ordered network IO.
