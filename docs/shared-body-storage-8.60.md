# Shared body storage — UDM 8.60 / extension 0.72.24

## Change and integration boundary

The production full-body cache and the standalone ordered-range writer now use `native/BodyBufferStorage.hpp`: one byte allocation and one thread-independent process-budget lease, capped at 64 MiB. The lease is declared before the byte vector, so destruction frees the bytes before releasing the budget. Admission failure retains the existing disk fallback. No HTTP request, file allocation, final-flush or publication policy changes.

`WrittenBodyCache::releaseStorage` invalidates the old cache's coverage and transfers its storage reference without copying bytes. The standalone writer accepts only uniquely owned storage of exactly the expected size. A joined, successfully finished writer can transfer that same allocation to an advisory hash reader once. Pause, abort, failure and unsettled work cannot export a completed body. The last storage reference retains the budget; invalidating a cache cannot permit a second allocation while another reader still owns the first.

This completes the shared-storage prerequisite in [the ordered-writer plan](ordered-range-write-plan.md). **The ordered network writer remains inactive.** Persistent disk-credit snapshots, generation/reset coordination, range-response ownership and cancellation/error reconciliation must be integrated before enabling it. Network-received bytes must never become recovery credit merely because they are in RAM. A successful storage callback is not a durability barrier; the mandatory final file flush remains.

## Evidence

- [Native qualification](validation/shared-body-storage-8.60/suite-results.json): 1,303 passing checks in twelve final-binary-matched runs; 99 cache/ownership checks on each drive. Cases retain the prior cross-thread destruction and contention checks and add shared-reader lifetime, pointer identity through cache/writer/completed-body handoff, exact-size and unique-owner refusal, one-budget admission and incomplete export refusal.
- [Standalone qualification](validation/shared-body-storage-8.60/ordered-results.json): 47 passing checks on C: and 47 on D:, recorded separately from the native aggregate. Real 256 KiB ordered outputs and 8 KiB paused outputs match independently constructed contents and SHA-256. Earlier isolated-budget prototype evidence remains historical, not part of this aggregate.
- [Build](validation/shared-body-storage-8.60/build-result.json): five products and eight backend fixtures from 518 frozen inputs. Transfer and Bridge rebuilt; forty qualified core objects retained. All thirteen executable version resources report 8.60; both extension manifests are 0.72.24.
- [Actual App smoke](validation/shared-body-storage-8.60/app-64m-cache-smoke.json): a real 64 MiB split download on D: used the full-body write-buffer advisory hash path, matched exact output length/SHA-256/catalog, exited normally and passed a separate post-exit output flush. Exit tail was 670.0325 ms and separately observed durable tail 678.5746 ms. This is one correctness run, not a controlled speed comparison or an IDM parity result.
- [Reconstruction](validation/shared-body-storage-8.60/source-reconstruction.json) verifies qualified prior source identities; the [reversible source delta](../benchmarks/speed/experiments/shared-buffer-8.59-to-8.60.patch) passes reverse application checking against current sources.

The factory's allocation failure paths use RAII: a failed vector construction releases the moved lease, and failed shared-pointer control-block construction deletes the supplied object. This follows the [shared pointer constructor contract](https://eel.is/c++draft/util.smartptr.shared.const); no injected allocation-exhaustion test is claimed. Shared storage extends lifetime; it does not make arbitrary concurrent mutation safe. Cache operations remain serialized and completed hash access follows writer join.

## Cleanup and deployment

[Native cleanup](validation/shared-body-storage-8.60/cleanup.json) removed 407 generated files / 126,503,413 logical bytes plus the separately removed 67,108,864-byte smoke payload. [Standalone cleanup](validation/shared-body-storage-8.60/ordered-cleanup.json) removed another ten files / 4,472,508 logical bytes, including its executable/object and both sets of payloads. Each deletion was scoped to owned fixtures, checked for links/reparse points and recorded with hashes. Logical bytes are not a physical freed-space measurement.

[Final verification](validation/shared-body-storage-8.60/final-verification.json) rechecks all 518 current inputs, 42 core objects, five retained products and three protected installed/personal files after cleanup. Installed UDM remains 8.32. No installer, installation, extension deployment, push or release was performed; no administrator prompt was needed.
