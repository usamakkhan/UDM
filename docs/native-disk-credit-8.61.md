# Native disk-credit checkpoints — 8.61 / extension 0.72.25

## Behavior and activation boundary

The existing synchronous, cache-eligible fresh root HTTP path now attaches a generation-bound disk-credit lease. `WrittenDiskCheckpoint.hpp` records successful positioned writes; `CatalogDiskCredit.hpp` binds that ledger to the owning Manager worker/cancellation object and actual expected parts file. This is native integration of the earlier [checkpoint prototype](disk-credit-checkpoint-plan.md), not activation of the ordered network writer.

`Manager::saveCatalog` applies the disk-credit projection only to its detached persisted row. Job/Manager UI snapshots remain live. Saved segment Done is clipped to its acknowledged contiguous disk prefix, and saved Received is recomputed; network/quota accounting is preserved. The Manager's private active map and actual catalog membership validate the generation. Missing/duplicate rows, stale cancellation ownership or changed representation/storage identity fail closed.

The captured leases survive JSON serialization, the catalog coordinator, file replacement and checkpoint-result acceptance or exception unwinding. The periodic dirty check compares this persisted view while a binding exists. It skips an unchanged persisted projection, but preserves actual writes for pending browser admissions/publication proofs. Shutdown/worker shortcuts cannot treat a live disk-credit binding as an already-settled final checkpoint.

After producers and hashing join, transfer retirement waits outside Manager::mutex until captured saves and write leases settle. It then atomically reconciles the live resume counters, retires the generation and detaches it under the model lock. Last parts-file-owner destruction happens outside the model lock. The existing synchronous HTTP loop closes this binding before another generation can repair/reset parts or before final assembly/publication. Original full-response reuse, range-request policy, byte charging and the required final flush remain.

Admission is optional when the expected file cannot be opened; the synchronous disk path remains. Direct transfer callers without the owning worker scope and child/media tracks do not attach this root-only binding. This does not yet cover asynchronous buffered reception, queued ownership/overlap checks, child-track generations or every Manager reset/rollback operation. Those gates remain open in [the ordered-writer plan](ordered-range-write-plan.md).

## Windows handle-pin correction

The first candidate used FILE_READ_ATTRIBUTES and failed its physical pin test: another handle could request DELETE access. That rejected result is [retained](validation/native-disk-credit-8.61/rejected-attribute-only-checks.json), with [build identities](validation/native-disk-credit-8.61/rejected-attribute-only-build-result.json) and [reconstructed header identity](validation/native-disk-credit-8.61/rejected-attribute-only-reconstruction.json). It was not installed or promoted.

The final code opens the expected parts file with GENERIC_READ and read/write sharing, omitting delete sharing. The actual tests verify DELETE-access refusal while the binding is active and successful access after its held catalog save and retirement settle. Microsoft's [CreateFile sharing contract](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew) distinguishes attribute requests from the sharing rules and documents delete/rename sharing. The owning ordinary handle is retained; external in-place writers are not excluded by read/write sharing. App-controlled truncation/reuse must follow retirement; this is not protection against arbitrary external modification.

## Qualification

[Final selected suite results](validation/native-disk-credit-8.61/suite-results.json): **1,644 passing checks in eighteen final-binary-matched runs**. Earlier rejected and intermediate integration runs are excluded from that aggregate.

- 46 actual Manager integration checks on each drive: selective disk/UI views, unchanged network counters, pending browser admission in the same save, installed-admission handoff, automatic dirty decisions, changed representation and active-map refusal, membership loss, physical file pinning, successful/failed held saves, retirement outside the model lock, reconciliation, error retry and actual catalog reload.
- 74 primitive checks on each drive against the actual native implementation, including bounds/metadata refusal, conservative gap/prefix credit, eight concurrent real writers, caller-owned flush/hash checks, detached snapshot lifetime and controller destruction.
- Existing cache, prefix, prefinish writeback, completed/catalog checkpoint, recovery, publication and synchronization suites; additional catalog-admission and coordinator checks.

[Build](validation/native-disk-credit-8.61/build-result.json): five products and twelve fixtures, with all seventeen recorded resources at 8.61 and both extension source manifests at 0.72.25. The final source inventory has 522 inputs. Core, Transfer and Bridge rebuilt; thirty-nine qualified core objects remain. Supplementary test-only compilation extended the fixture contract and noncompiled backend test manifest without changing those core objects or five products.

The [real 64 MiB split App smoke](validation/native-disk-credit-8.61/app-64m-cache-smoke.json) confirms `DiskCreditCheckpoint.Used=true` and ReconciledBytes=67,108,864 before publication. Length, independently expected SHA-256, catalog completion, normal process exit and a separate post-exit output flush pass. Full-body advisory hashing uses the write buffer. Observed exit/durable tails are 680.7141/709.0906 ms; nine origin requests sent 67,672,852 bytes including protocol overlap. This one correctness run is not a balanced speed comparison, request-reduction result or IDM parity claim.

No allocation-exhaustion injection, installed/browser acceptance or complete asynchronous ordered-transfer qualification is claimed. Written acknowledgment is not a power-loss durability barrier; the final explicit flush is retained.

## Cleanup, retained evidence and next work

[Cleanup](validation/native-disk-credit-8.61/cleanup.json) removed 487 generated files / 157,425,750 logical bytes plus the separately removed 67,108,864-byte smoke payload. This includes rejected/intermediate/final fixtures, twelve test executables and entry objects, with per-file hashes and hardlink/reparse checks. Logical bytes are not a physical freed-space measurement. Qualified products, core objects, sources and sanitized evidence remain.

[Final verification](validation/native-disk-credit-8.61/final-verification.json) rechecks 522 current inputs, 42 core objects, five retained products and three protected installed/personal files after cleanup. The [source delta](../benchmarks/speed/experiments/disk-credit-8.60-to-8.61.patch) reconstructs the qualified prior input identities and passes reverse-application checking.

Next integrate separate read/queued ownership and buffered overlap access, asynchronous storage-failure cancellation and exact acknowledgment reconciliation, then join ordered writes before writeback/hash/publication. Add actual Manager cancellation, refresh, folder/reset/removal and failed-save rollback fault cases before enabling buffered reception. Finally qualify complete lifetime performance, retaining slower results and post-exit flush work. Child-track bindings need their own ownership path; they must not silently inherit the root scope.

Installed UDM remains **8.32**, freshly confirmed from its unchanged executable resource and hash. No installer package, installation, extension deployment, push or release was performed. No administrator prompt was needed.
