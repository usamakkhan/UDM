# UDM 8.47: recovery catalog checkpoints outside the model lock

UDM 8.47 releases recovery's own model lease during catalog serialization, durable persistence and captured-snapshot acknowledgement. Runtime ownership retains the affected job tree and destinations, while the journal and verified file handles remain held. Native version is 8.47 and both extension source manifests are 0.72.11; browser JavaScript is unchanged.

## Reproduced blocking boundary

A fresh six-case probe linked against qualified 8.46 objects holds the actual catalog temporary-file flush. Direct recovery, Resume and File Info Start, each with legacy and interrupted-journal completion, all find the shared model mutex unavailable. Exact recovered bytes and existing File Info warnings still pass. Compilation ran concurrently with this baseline, so these are availability observations, not timing measurements. [Baseline and exact hashes](validation/recovery-catalog-checkpoint-8.47/baseline-lock-proof.json).

The new fixture holds that same real boundary through nine direct/public entry cases: direct, Resume, File Info Start and project admission for both legacy and journal recovery, plus journal-backed schedule creation. Each can take a UI-facing snapshot, preserve independent metadata/settings, and observe the job and destination reserved. Competing Resume cannot queue another transfer; Remove and Configure cannot take over the recovering job.

## Transaction and proof

Recovery captures current catalog membership, top-level registration, destination inputs, before/applied rows and the existing checkpoint observer under the model lease. It reserves the complete affected descendant graph and target, staging and previous-version paths through the existing publication ownership registry. It advances ownership to commit state before verification, preventing a Pause or shutdown from treating the operation as an ordinary cancellable transfer.

`PublicationCatalogProof::prepareRecovery` uses the existing in-process captured-snapshot registry. It binds every recovery-owned row change, every removed field, core identity/completion fields and exact visible-row membership or private-row absence. A private Complete outcome additionally requires its exact durable publication receipt. A legacy recovery has a fresh in-process lease identity; no new persisted marker or sequence-only acceptance shortcut is introduced. Paused outcomes and both roles of a replacement use the same captured-snapshot contract.

The recovery method releases its own model lease for `save` and proof resolution. A normal return from a deferred save is insufficient. An unrelated or noncontaining checkpoint cannot acknowledge recovery; an actually captured later containing writer can acknowledge it after the original writer fails. Unknown acknowledgement preserves the journal and requires explicit recovery. On failure, conditional rollback restores only still-owned applied fields, preserving independent edits; it erases only a matching receipt inserted by this attempt.

After durable acceptance, recovery reacquires the model lease, rebuilds invalidated indexes, and checks current job identity/membership, registration, destination inputs and the owned row projection. Only accepted and still-valid recovery retires the exact held journal. Byte and journal pins retain the 8.41/8.42/8.46 ownership protections. Existing flush, intent, catalog and filesystem safety barriers remain.

## Backend caller preparation

Resume, Remove, Configure, stopped File Info Start/Later, schedule creation/revision, Move/Rename, recycle, project admission and replacement predecessor preparation no longer impose their own previous outer recovery lease. Their continuations reacquire the model and repeat the relevant membership, activity, completion, destination or draft checks. Project admission reconciles selected existing downloads before opening its deferred catalog batch, then rebuilds and checks its selected-job mapping and saved project. Its already-reconciled batch continuation avoids reentering recovery.

`presentOffer` and live-HLS finish release their own Resume lease. Active internal publication children retain the previous Resume no-op even though they are absent from the top-level list; inactive commands still validate catalog membership. This ordering was restored after the existing ownership regression found the initial candidate rejected the child.

## Qualification

All **2,829 focused checks across 16 runs passed**. Final qualification, exact source/object/product hashes, all selected suites and the lifecycle smoke are recorded in the [validation receipt](validation/recovery-catalog-checkpoint-8.47.json). The new recovery matrix checks ordinary Complete/Paused, replacement Complete/Paused, legacy Complete and private Complete/Paused outcomes. It injects actual catalog flush failure, deferred save, noncontaining snapshots, removed-field regression, missing private receipt, lost acknowledgement, later containing publication and later retry after an earlier writer fails. It checks the actual on-disk catalog before a repair save, exact retained bytes, both replacement roles, private absence, conditional rollback and explicit retry. Shutdown during a held flush waits for the owner and accepted journal retirement; new recovery after shutdown is refused. The matrix is run on C: and D:.

Two initial failures remain in the evidence. The first was a fixture mistake: changing a private row's status cannot invalidate a catalog that deliberately omits it. The corrected case inserts a forbidden visible row. The second was the genuine active-child Resume ordering regression described above; the final core and every executable were relinked before repeating the full selected regression set.

The complete native C++ core is rebuilt because the recovery entry signature changed; only external C compression objects are retained. There is no mixed old-native ABI link. Five production programs and fourteen fixture programs are built. A native 4 MiB ordinary-download smoke checks complete length/hash, one origin request, normal app exit, publication journal retirement and an explicit post-exit output flush. This smoke does not compare IDM or establish a performance win.

## Limits and ordered next work

1. Discovery, file opening, hashing, moves, cleanup and receipt-only pruning still hold the model lease. This increment detaches the recovery catalog checkpoint only. Prepare immutable discovery/IO requests and revalidate their results before migrating the remaining IO.
2. Outer UI scopes in `ProgressUi.hpp`, unfinished Properties in `PropertiesUi.hpp`, `PendingFileInfoInbox.hpp` cancellation and `DownloadInfoUi.hpp` cancellation remain atomic and can still hold the lock through an invoked recovery. Properties requires a backend current-header patch operation; cancellation requires an atomic guarded removal. Simply dropping those UI locks would weaken their existing correctness guards.
3. Finish those caller contracts and held verification-IO tests before claiming recovery is fully detached or the desktop remains responsive on every path. Other unrelated setters may still hold their own model lease while saving.
4. Installed/browser and visual acceptance remain separate. The original slower sequential/split finishing cases, full background-work comparisons and full IDM parity are unresolved. No universal latency claim follows from these correctness tests.

Installed desktop remains 8.32. Research-first installation deferral remains in force. No installation, extension deployment/reload, installer, push or release is performed. Personal catalogs, prior qualified references and canonical shared tools/objects are preserved.

## Source preservation and cleanup

The [8.46-to-8.47 source delta](../benchmarks/speed/experiments/recovery-catalog-8.46-to-8.47.patch) reconstructs all twelve changed/added sources in both directions with original hashes and recorded line endings. This is source reconstruction, not bit-identical timestamped executable reproduction. Cleanup removed **1,787 owned files totaling 187,721,662 logical bytes**, plus the separately removed 4 MiB smoke payload. All link groups resolved without exclusions. Only individually checked fixture outputs, temporary payloads, redundant target objects and temporary source copies were retired after their evidence was saved. Current production programs, all newly rebuilt core inputs, prior qualified references, installed files and personal data remain. See the cleanup record linked by the validation receipt for exact totals and retained multi-link/reparse exclusions.
