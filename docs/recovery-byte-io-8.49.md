# UDM 8.49: detached recovery byte IO

UDM 8.49 moves ordinary publication, paired replacement and legacy-completion byte inspection outside recovery's own shared-model lease. Reservations, immutable journal ownership and verified file handles remain live until snapshot-bound catalog acceptance. Native version is 8.49 and both extension source manifests are 0.72.13; browser JavaScript is unchanged. Installed desktop remains 8.32, with installation deferred.

## Reproduced read-loop blocking and redundant work

A separate 8.48 control links qualified prior core objects with an instrumented copy of Duplicates. The only control changes add held SHA-256 read-loop observation callbacks. There is no unlock, mutation, hash, sharing or move-policy change. All ten valid recovery variants reach that boundary with the model unavailable. The 90-check control verifies successful recovery and exact bytes after release. This is an availability and read-count control, not timing evidence. [Exact inputs and control results](validation/recovery-byte-io-8.49/baseline-proof.json).

The intact-old-original replacement branch previously hashed its target against the new digest, discarded the mismatch, then reopened and hashed that same target against the old digest. Its small real-file control reaches four read checkpoints: a data read and EOF check for each of two hash passes. The new implementation measures one held target, classifies its digest against both identities, and retains that same file for acceptance. It reaches two read checkpoints and one completed hash. This removes a complete redundant read/hash pass; no universal elapsed-time improvement is inferred from the count.

The [control instrumentation delta](../benchmarks/speed/experiments/recovery-byte-io-8.48-control.patch) reconstructs the exact observed 8.48 source independently of the production delta.

## Immutable preparation and final revalidation

Under the model lease, recovery resolves actual catalog membership, captures root/descendant pointers, current video/audio relationships, registration, core verification inputs and destination policy. It reserves the affected job graph and target/staging/previous paths through the existing publication ownership registry before releasing the lease. The requested scoped parent is also checked when the byte operation belongs to a child.

The unlocked IO consumes copied journal operation data, copied job inputs and local paths. It does not read live JSON, retain JSON iterators or use stale catalog indexes. Actual file opening, length/digest inspection, required early-intent writeback, existing folder checks and recovery moves execute in that interval. Existing reparse/type/hash checks, sharing flags, nonoverwrite behavior, write-through moves and final-name identity checks remain.

The read loop takes a brief model lease to check shutdown before IO checkpoints; it holds no model lease during the held read boundary or actual read. Before a recovery move it additionally revalidates the captured model inputs. After IO, recovery reacquires the model, clears cached indexes and checks closing/batch state, ownership, actual pointer/membership, top-level registration, destination inputs, child relationships and publication/response/byte-verification fields. Changed input is rejected before applying history. Exceptions reacquire the lease and invalidate indexes before unwinding. Uncertain bytes and journals remain available for recovery.

Current and legacy journal application merge only recovery-owned fields with the freshly revalidated row. Independently saved descriptions, current request headers and unrelated settings survive. Replacement rollback uses conditional row changes rather than replacing current rows wholesale. Legacy completion now measures and hashes its held file directly, reuses its measured length and avoids the extra pathname-based hash open and post-verification size query. Invalid, busy or mismatching legacy evidence stays incomplete; a read failure is reported with history preserved.

The existing catalog proof phase still releases its lease for serialization, durable save and actual containing-snapshot acknowledgement. Byte and journal leases remain until that acceptance. No acceptance shortcut, extra network request, weakened flush or pacing rule is introduced.

## Qualification

All **5,706 checks across 22 production-code runs pass**, plus the separate 90-check 8.48 control. The new 1,166-check byte-IO matrix passes independently on C: and D:. It covers ordinary staged/published files, saved early intents, legacy journals, private and child publication, legacy completion, paired completed publication, retained original and original restoration.

At the actual held open-file SHA-256 read-loop boundary, tests obtain a model snapshot, observe active job/path ownership, and refuse a concurrent source writer and immutable-journal writer. An unrelated durable save completes while the read loop remains held. Independent metadata for both replacement roles survives application. Fault cases change destination, membership, private absence, publication identity, child relationships and batch state; inject a read-checkpoint exception; create an occupied target; and shut down while verification is held. Rejected operations preserve their records/bytes and release runtime ownership; explicit retry or reopening reconciles the retained state. Early-intent acceptance still observes its required body flush. These are owned native fixtures with small synthetic files, not live-browser or installed visual checks.

Existing journal, ownership, publication/replacement commit and recovery, early-intent, checkpoint/proof, synchronization, File Info decision, request-header draft, capture-presentation and pending-cancellation regressions pass against the same core. The 206-check actual headless UI-command matrix also passes on both drives. [Suite results](validation/recovery-byte-io-8.49/suite-results.json).

Five production programs and eighteen fixture programs are built. Duplicates and the version-reporting Bridge are recompiled; twenty-two native units and eighteen external C inputs retain their qualified objects. The repository-owned quoted-include closure of each retained native unit is checked against the qualified 8.48 hashes; selected object hashes are verified. Resources and all twenty-three production/fixture executable versions are 8.49. The temporary control executable uses its separate 8.48 resources. [Complete qualification](validation/recovery-byte-io-8.49.json).

A complete 4 MiB ordinary app download used one origin request, matched its independent SHA-256/length, retired its publication intent, exited normally and passed an explicit post-exit output flush. Its exit tail was 189.343 ms and its durability tail 301.716 ms. This is one unpaired lifecycle smoke, not an IDM comparison or speedup claim; prior measurements and losses remain. The payload was removed immediately. [App smoke](validation/recovery-byte-io-8.49/app-4m-smoke.json).

## Remaining boundaries

Journal discovery/enumeration, opening/reading the journals themselves, receipt-only pruning and post-acceptance zone/retained-output/journal/handle-release cleanup still hold the model lease. Failure-unwind handle release also needs a separate IO-boundary audit. Normal mutation saves, completed Properties and any further outer callers found by audit remain separate work. This is not a claim that every recovery IO or every app operation is detached.

Continue immutable journal-discovery preparation and identity revalidation, then detach post-acceptance cleanup while preserving exact-journal retirement and receipt ownership. Keep adversarial namespace/alias/parent mutation and refusal/fallback cases in the remaining transaction audit; the existing staging/restoration rename protocol is preserved rather than claimed to be a globally atomic filesystem transaction. Recheck all relevant caller contracts before asserting desktop-wide availability.

Installed/browser and rendered UI acceptance remain separate. Original sequential/split finishing losses, complete background-work comparisons and IDM parity are unresolved. No installation, extension deployment/reload, installer, push or release is performed. The two personal catalogs, installed executable, canonical tools and prior qualified reference inputs retain their hashes.

## Source preservation and cleanup

The [8.48-to-8.49 production source delta](../benchmarks/speed/experiments/recovery-byte-io-8.48-to-8.49.patch) reconstructs six changed/added sources forward and backward with exact hashes and recorded line endings. The control instrumentation is independently reconstructed. These proofs concern source bytes, not timestamped executable reproduction.

Cleanup individually verifies owned paths, parents, file identities and complete hard-link groups before removing fixture payloads, temporary source copies, redundant target objects and control artifacts. Current products, two rebuilt private core inputs, forty retained inputs, shared tools junctions, personal data and prior qualified references remain. Removed **2,377 owned files totaling 240,384,792 logical bytes**, plus the separately deleted 4 MiB smoke payload. No unresolved link groups, skipped paths or directory deletions occurred. These are logical totals, not physical allocation claims.
