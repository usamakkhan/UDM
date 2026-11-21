# Publication recovery without shared-model IO

Status: staged implementation through UDM 8.53. Properties now invokes completed/unfinished persistence and recovery from an owned worker, with actual private-window message-pump, error/retry and preparation-reentrancy checks. This closes the source-level synchronous caller gap recorded in 8.52; it does not detach normal backend metadata saving or receipt-only pruning. Further caller/namespace, zone metadata, installed acceptance and IDM finishing-speed work remain. [Properties qualification](properties-save-8.53.md).

## Required transaction boundary

The existing `recoverReplacements` owns one recursive mutex lease, but Resume, Remove, Configure, File Info and schedule/project workflows may already own another. A local `unlock()` is insufficient. No implementation may suspend an unknown number of recursive leases: outer callers may retain references into mutable JSON or assume batch isolation.

Introduce an explicit recovery preparation phase before each public command acquires its long-lived mutation lease. Preparation must return a result describing whether a record became Complete, not silently continue into the original Start/Edit operation. The existing recovery method remains the reconciliation authority until every caller has migrated; it must not be replaced by an unused parallel implementation.

Proposed internal concepts:

- A request identifies a scoped job tree or startup-wide discovery and the command reason (Resume, Remove, Configure, Start, schedule edit, project admission, predecessor recovery).
- A prepared operation owns immutable journal bytes/digest, parsed Before/After records, commit ID, current job pointers, destination-policy inputs and normalized target/staging/previous paths.
- A reservation owns the complete affected job tree and paths. It must cover both replacement roles and ephemeral/internal publication records, using the existing publication ownership contract rather than a second unrelated busy flag.
- An IO result contains verified file identities, lengths and digests plus the resulting filesystem branch. It contains no references into live JSON.
- A recovery outcome distinguishes unchanged, recovered paused, recovered complete and rejected/manual recovery. It must preserve the current File Info recovered-destination warning.

These are proposed interfaces, not shipped symbols.

## Ordered implementation

1. Discover candidate journals outside the model lease. For scoped publication journals, retain exact job-tree filtering. Replacement discovery must still find journals whose OriginalId refers to the selected original, including a completed or ephemeral successor. Do not ignore corrupt possibly related journals merely to make a scoped operation pass.
2. Under the model lease, resolve current membership and descendant identity, reject active owners, capture destination inputs and acquire the full reservation. Reject a catalog batch before publishing a reservation or altering state. Snapshot the journal identity and current rows. Release every command-owned model lease before IO.
3. Inspect/pin files and compute hashes without the model lease. Preserve the existing reparse, sharing, file-size, digest, target-nonoverwrite and replacement-original checks. A replacement rollback may move the previous file only in the existing verified branch. Retain journal and all bytes on uncertain IO.
4. Reacquire the model lease. Revalidate journal identity, job membership/pointers, reservation ownership, destination inputs and current commit IDs. Do not reuse cached JSON iterators or maps across the unlocked interval. Preserve independent metadata using existing PublicationCommit/ReplacementCommit application semantics; legacy replacement behavior needs an explicit compatibility branch.
5. Install the recovered row or pair and create a snapshot-bound PublicationCatalogProof lease before releasing the model lease for serialization and durable save. New legacy repair needs a valid fresh commit identity if it uses this proof. A sequence number, normal return from `save`, or an unrelated newer checkpoint is not acceptance evidence.
6. Resolve the proof without holding the model lease. On failure, reacquire it and roll back only still-owned applied fields; never overwrite independent newer edits. Preserve the journal and bytes. A possibly published checkpoint must be resolved before reverting or granting another writer.
7. Only after accepted catalog proof may the exact unchanged journal be removed. Retained media cleanup, zone metadata and receipt retirement must preserve current ordering and failure behavior. Ephemeral receipt cleanup must not create a visible row.
8. Release reservation and return an explicit outcome. Public command continuation rechecks membership, active state, batch state and relevant draft inputs; it does not assume its original selection is still current.

## Caller migration requirements

| Entry | Current outer lease / behavior | Required continuation |
| --- | --- | --- |
| Direct scoped recovery | Recovery method itself holds the model mutex | Common reserved IO transaction, with no implicit outer lease required |
| Startup | Manager loading is not yet a usable desktop model | Same identity/acceptance rules; startup errors and receipt pruning remain visible |
| Resume | Core.cpp acquires the mutex before recovery | Prepare before mutation lease; reacquire and recheck active/selected job, original response and authentication states |
| Remove | Recovers file operations and publication tree before deleting history | Preserve both recovery prerequisites; recheck expected completion and replacement dependants before deleting anything |
| Configure | Calls recoverBeforeConfigure before applying edits | Preserve the beforeStart/journal policy; a recovered Complete result must retain the old-destination warning |
| File Info Start / Later | FileWorkflows.cpp owns the mutation lease; Start requests beforeStart recovery | Distinguish Start from Later; preserve destination/login/queue drafts and exact recovered warning; do not resubmit a recovered response |
| Create/revise schedule | Queue.cpp calls recoverBeforeConfigure before checking draft consistency | Recover before the mutation lease, then repeat queue and original-download draft comparisons before any swap/save |
| Site Grabber addProject | Recovery runs before catalogTransaction defers saves | Prepare every selected existing job before opening the batch; rebuild/revalidate matched-job mapping afterward |
| Move/Rename (`relocate`) | Holds a unique recursive model lease, after file-move recovery, before replacement recovery | Prepare recovery before retaining source/destination JSON or installing relocation ownership; reacquire and repeat membership, Complete/inactive state, pending-replacement and destination checks |
| Recycle completed file (`recycleCompleted`) | Holds a model lease across file-operation and replacement recovery | Recover before preparing recycle action; recheck identity, Complete/inactive state, dependent replacements and missing-file policy before invoking shell recycle |
| Replacement publication | Predecessor recovery occurs inside initial model scope | Reconcile both roles before acquiring new swap ownership; revalidate original pointer, path, digest and completion state |

This table records the original caller baseline and required contracts; the 8.48 and 8.49 sections below record implemented stages. Every additional production caller found during migration must be added here. No caller may hold an outer model lease merely because its invoked method also locks internally.

## Verification gates

The current six-case fixture covers legacy and interrupted-journal recovery through direct, Resume and File Info Start paths. Each holds the actual flushed catalog boundary, finds the model unavailable, then verifies exact bytes. After implementation these cases must assert availability while retaining their existing successful recovery and File Info warning expectations; retain the baseline evidence.

Add held verification-IO cases, not only held completion callbacks. Exercise an independent model save and UI-facing snapshot during that hold. Verify that competing Resume, Remove, configuration, path changes and replacement publication cannot take over reserved jobs or paths, while unrelated jobs/settings remain usable.

Fault coverage must include changed journal bytes/commit ID; changed destination or project root; removed/replaced job; alias paths; ambiguous child identifiers; original/previous/staging file replacement; locked source/backup; failed temporary flush or catalog replacement; a captured later writer; cancellation; shutdown during hashing or checkpoint resolution; expired owner; and ephemeral receipt recovery. Verify rollback and retained bytes by reopening the isolated catalog.

Run all publication preparation, ownership, commit, recovery, catalog checkpoint/proof and completed-checkpoint fixtures on the exact rebuilt code. Then exercise File Info and scheduler/project public entry points. Record actual installed/browser acceptance separately from native fixtures. Native and both extension source versions advance for functional changes; installation, installer creation and publishing remain deferred.

## Performance and cleanup

A green correctness fixture does not prove IDM speed parity. Compare uninstrumented complete lifetimes on the original cases, retaining losses and final background work. The existing user-mode observations are attribution evidence only; they do not establish kernel/disk/filter causes. Keep durability barriers until an alternative recovery protocol supplies equivalent acceptance and restart proof.

Use isolated catalogs and canonical shared media tools. Retain source/results/rollback/reference artifacts. Individually remove owned payloads and redundant compiler objects after hashes, ownership and link checks; never recursively delete through tool junctions or personal data.

## Replacement identity prerequisite

A follow-up source audit found replacement recovery hashing the new target and retained previous version through temporary pathname-based reads. Both handles closed before recovered-pair catalog persistence. The 8.42 regression reproduces a concurrent writer at that actual save boundary against the qualified 8.41 core. This is a separate ownership gap from ordinary publication recovery. Paired replacement completion and original-file restoration must retain every verified file through acceptance before their IO can safely be detached from the model lease. Version 8.42 passes 1,249 focused checks, including paired/restore write and rename exclusion; [qualification](replacement-recovery-pinning-8.42.md). The caller migration above is still pending.

## 8.46 journal ownership prerequisite

A real baseline probe showed both ordinary and replacement recovery consuming a journal changed by rename, direct write or a hard-link alias during actual catalog persistence. Recovery now parses one bounded held-file snapshot, retains read/delete ownership through its existing save, and retires that exact file by handle. Refused ownership and failed persistence preserve the journal; scoped discovery does not claim unrelated locked journals. All 1,661 focused checks pass, including the expanded journal fixture on C: and D:. [Implementation and evidence](recovery-journal-ownership-8.46.md).

This does not detach recovery IO, reserve its live job tree/paths, add snapshot-bound recovery catalog proof, migrate outer-lock callers or change the six-case locked-model baseline. The same held journal object can be carried through the detached protocol above; every other reservation/revalidation/acceptance requirement remains. The 8.45 metadata timing experiment is retired rather than retained as an unproven finishing optimization.

## 8.47 catalog-checkpoint phase and remaining boundaries

The [8.47 implementation](recovery-catalog-checkpoint-8.47.md) completes snapshot-bound recovery checkpoint acceptance and reservation of affected descendant jobs and target/staging/previous paths. Direct recovery and migrated backend preparation scopes release their own model lease for serialization, persistence and captured-snapshot resolution. Nine public-entry holds observe available model snapshots at the real flushed catalog boundary; a freshly linked six-case 8.46 control retains its unavailable-lock baseline. Complete/Paused, paired, legacy, private, failed/deferred/unknown and actual later-writer cases pass, including shutdown owner draining. All 2,829 focused checks pass across 16 native runs.

The caller table above describes the original state and required final contracts. Backend preparation is migrated for Resume, Remove, Configure, stopped File Info, schedules, project admission, relocation/recycle and replacement predecessor recovery; continuations recheck their current inputs. Project batches use an already-reconciled resume continuation. The next UI phase still requires backend atomic operations for Properties current-header patches and guarded cancellation removal. ProgressUi, PropertiesUi, PendingFileInfoInbox and DownloadInfoUi retain outer atomic model scopes. A naive UI unlock would weaken those guards.

Verification/discovery/open/hash/move/cleanup IO and receipt-only pruning remain inside the model lease. Finish immutable discovery/IO preparation and result revalidation, then migrate every remaining outer caller and run held verification-IO and installed UI/browser checks. Do not label the full transaction or desktop responsiveness complete from the checkpoint-only tests. Existing sequential/split finishing losses, actual IDM comparisons and installation deferral remain unchanged.

## 8.48 guarded UI continuation phase

[Version 8.48](recovery-ui-commands-8.48.md) migrates the four previously named outer UI scopes plus an additional browser-receipt cancellation wrapper found during the caller audit. Progress actions release their lease and use a predicate checked before and after backend recovery. Unfinished Properties applies its logical header patch to the current recovered session under the mutation lease, including current-origin checks and exact rollback. Cancellation uses private before/after reconciliation callbacks under recovery's own final lease, so an unconfirmed offer recovered to Paused can be removed without an unlocked acceptance/removal gap. Browser cancellation rechecks receipt identity and rejects a provisional publication owner rather than releasing its presentation prematurely.

All 3,374 focused checks pass across 20 runs. The 206-check actual headless command-adapter fixture passes on C: and D: and observes model availability at real held recovery catalog flushes while retaining ownership and exact bytes. Existing header-draft, File Info, pending cancellation, presentation, catalog, publication and recovery fixtures pass against the same 24-unit rebuilt native core. Five products and seventeen fixtures built; a complete real 4 MiB app lifecycle/hash/normal-exit/post-exit-flush smoke passes. This is native correctness evidence, not installed visual/browser or IDM latency parity.

The five identified scopes are closed; the full transaction is not. Discovery/verification/hash/move/cleanup IO and receipt-only pruning still remain inside the model lease, and normal mutation saves may remain locked. Continue immutable IO request preparation and result revalidation, audit additional callers rather than assuming this list exhaustive, then run installed acceptance when installation is authorized again. Keep the original slower sequential/split finishing losses and background-work comparison gates open. Installation remains deferred at 8.32.

## 8.49 immutable byte-IO phase

The [8.49 qualification](recovery-byte-io-8.49.md) detaches ordinary, paired and legacy file opening/length/hash checks, required early-intent body writeback, existing folder validation and recovery moves. Captured data includes actual root/descendant pointers, membership, registration, destination inputs, child bindings and byte/publication/response fields. IO uses copied JSON and paths. Brief checkpoint leases observe shutdown and, before moves, captured-state consistency. After IO, recovery clears stale indexes and revalidates everything before merging current history. File/journal handles and complete job/path ownership survive through the separate captured-snapshot catalog proof.

An independent durable save and UI snapshot complete during real held SHA-256 read loops. Changed destination, membership/private absence, commit ID, graph or batch state is rejected; read exceptions and shutdown retain recoverable records/bytes. Legacy/current application preserves unrelated saved metadata. The intact-original replacement path now classifies one measured target instead of hashing it twice. A separate instrumented 8.48 control confirms blocked-model behavior in ten variants and two old-target hash passes versus one. All 5,706 production-code checks pass across 22 runs, including 1,166 byte-IO checks per drive; the control separately passes 90 checks. Native products and one complete 4 MiB lifecycle/hash/post-exit-flush smoke qualify without installation or an IDM timing claim.

### Integrity preflight completed in 8.50; further gates remain

The [8.50 preflight](replacement-metadata-8.50.md) reproduces four false completions against 8.49: altered After/OriginalAfter counts and retained/restored OriginalBefore counts could persist Size 100 for actual 14/9-byte files and consume the journal. Completed-pair recovery now validates measured counts, digest and destination before applying either role. Original repair takes accounting/digest/location from its held verified file rather than stale Before metadata. Shared normal/recovery proof schema rejects malformed completed counters; zero-byte files remain valid. Private, legacy, actual-catalog, rollback and retry cases pass, including 2,490 metadata checks per drive. All 10,686 selected checks pass across 24 runs.

Keep exceptional handle-release boundaries separate from successful post-acceptance cleanup. Continue remaining namespace/alias and caller audits; the integrity preflight does not claim the full transaction or all possible journal-schema corruption cases complete.

### Next: immutable journal discovery and ownership handoff

1. Capture the requested scope/known active owners briefly under the model. Discover journal names and relevant untrusted bytes without retaining model JSON references. Scoped replacement discovery must still consider OriginalId and retain corrupt possibly related evidence.
2. Reacquire the model, rebuild identity/membership and reject a now-active owner or catalog batch before committing a reservation. An optimistic discovery read does not authorize journal mutation or retirement.
3. Acquire and parse the immutable held journal snapshot outside the model lease, then revalidate relevance, exact operation identity, captured job relationships and paths before byte IO. Handle a journal that disappeared or changed during discovery according to actual owner/outcome evidence; do not silently consume or delete a successor's file.
4. Preserve unrelated locked-journal behavior and direct/public caller warnings. Add held discovery/open/read cases, changed journal/commit/name and scoped-original matches, actual later publisher handoffs, malformed/oversized input, cancellation and shutdown. Keep the old discovery-lock baseline separate from byte/catalog controls.

### Then: post-acceptance cleanup and receipts

Retain reservations while zone/retained-output cleanup and exact held-journal retirement run outside the model. Reacquire the model for matching-receipt retirement and current-state validation. Do not delete through a freshly resolved journal path or erase an unrelated successor receipt. Receipt-only pruning needs an immutable IO inventory plus model revalidation; it cannot rely on an unlocked JSON iterator. Cover failed cleanup, absent/mutated paths, aliases, unsupported/read-only retirement, ordinary/private/paired receipts, stopped owners and a containing later writer.

These remain implementation and qualification requirements. The external staging/restoration namespace protocol and every further outer caller still require adversarial audit; no global filesystem atomicity, desktop-wide responsiveness or full IDM parity is asserted. Original sequential/split finishing losses and final background-work comparison gates remain open. Installation stays deferred at 8.32.

## 8.50 replacement integrity qualification

The 228-case metadata matrix spans completed pairs, intact originals and restored originals; current/legacy visible journals, current private journals and empty/nonempty files. Faults cover counts, representation, digest/path disagreement, missing/negative/overflow values, stale actual catalog metadata and real catalog-flush failures. Refused completion retains both records/byte versions/journal; healthy-intent retry succeeds. Repaired originals and private receipts are checked in the actual saved catalog before another save. Descriptions and original finish history remain.

Normal and recovery proof preparation now use the same bounded nonnegative integer Size/Received equality rule. Core and Duplicates are rebuilt for the shared header; Bridge is rebuilt for version 8.50. Twenty-one native units remain after recursive owned-include audit, with eighteen pinned external C inputs. Five products and nineteen fixtures qualify; an actual one-request 4 MiB normal-exit/hash/post-exit-flush smoke passes. Both source extension versions are 0.72.14. No installer/deployment/push/release occurs and installed 8.32 hashes remain unchanged. The full discovery/cleanup, installed acceptance and IDM finishing comparison requirements above retain their original scope.


## 8.51 discovery boundary

Journal enumeration, bounded content reading/parsing and same-file claim now release recovery's own model lease. Scope/input revalidation and shutdown enrollment retain recovery authority; active synchronization workers can discover predecessors without reserving them out from under themselves. Three 8.50 controls reproduced the model lock during actual reads. All 16,276 selected checks pass across 26 runs, including 2,795 discovery checks and 238 workflow cases per drive. Five products and twenty fixtures built from 506 frozen inputs; five core units rebuilt and nineteen native units retained by include-closure audit. Initial fixture and synchronization-worker failures are retained. A one-request 4 MiB lifecycle/hash/normal-exit/post-exit-flush smoke passes. Removed 9,495 owned files / 273,029,223 logical bytes plus the smoke payload. Installation stays deferred at 8.32. Cleanup/pruning, further outer-scope audit, installed acceptance and IDM finishing-speed parity remain open.

A related active replacement is a legitimate predecessor-discovery caller. This case captures/revalidates read-only scope inputs without temporarily reserving the original; actual reconciliation still rejects active ownership and reserves the complete resolved operation before claiming its journal. The remaining cleanup/pruning stages retain their original requirements.


## 8.52 accepted cleanup boundary

Accepted recovery cleanup now releases its own model lease while closing verified files, writing zone metadata, removing retained output and retiring the exact held journal. Complete job/path ownership includes a separate retained-source path. Accepted current inputs and the exact private receipt are revalidated before/after cleanup checkpoints; a cleanup failure never rolls back durable completion. Three 8.51 controls reproduce blocked model access at journal retirement. All 25,218 selected checks pass across 28 runs, including 4,471 cleanup checks and 306 cases per drive. Real read-only retirement failure, explicit retry, retained-source deletion refusal and hard-link cleanup pass. Five products and twenty-one fixtures built from 507 frozen inputs; two native core units rebuilt and twenty-two retained by include-closure audit. A one-request 4 MiB lifecycle/hash/normal-exit/post-exit-flush smoke passes. Removed 13,971 owned files / 293,728,138 logical bytes plus the smoke payload. Installed desktop stays 8.32. Receipt-only pruning, external namespace/caller audits and IDM finishing-speed acceptance remain open.

The accepted catalog is not provisional rollback state. Cleanup failures before retirement preserve the owned journal for retry; failures after retirement preserve durable history and any private receipt until later reconciliation. Metadata and retained-source deletion retain existing best-effort behavior. Receipt-only pruning still performs filesystem absence checks and saving inside Model, and is the next required implementation phase. External namespace replacement and unknown recursive caller scopes remain separate gates; the staged history above is not a claim of full completion.


## 8.53 Properties caller dispatch

File Properties now dispatches completed and unfinished saves through an owned worker command. The actual dialog keeps pumping messages during a held catalog flush, freezes the submitted fields, rejects reentrant/duplicate acceptance and close, and restores the draft for failed-save retry. Completed saves atomically recheck membership/closing/batch/current headers; unfinished saves retain existing guarded recovery. All 2,896 selected checks pass across 17 final-binary-matched runs: 55 new backend checks per drive, four actual-window message-pump probes (62 checks), and the existing 522-check Properties dialog suite. The valid paused 8.52 control blocked its UI thread through an injected 500 ms flush hold; that is a causal probe, not natural latency. Five products, nine backend and two UI fixtures built from 509 source inputs with all 24 native core units rebuilt. A real one-request 4 MiB lifecycle/hash/normal-exit/post-exit-flush smoke passes. Removed 1,071 owned files / 321,629,212 logical bytes plus the payload. Installed desktop remains 8.32. Backend persistence/pruning locks, other callers, zone metadata and original IDM finishing-speed gaps remain open.

Model availability and UI-thread availability are separate gates. The native Properties window now returns from OK before its held flush is released, receives timer heartbeats during that hold and reports failed persistence without discarding the draft. Progress already uses the decision worker; Properties now shares its owned lifecycle. Continue receipt-only pruning and remaining synchronous/persistence callers rather than treating this one dialog as desktop-wide acceptance.

The [8.53 pruning baseline](finishing-research-8.53.md#receipt-only-pruning-verified-responsiveness-baseline) now verifies the remaining receipt-only lock with actual catalog-flush holds, for one and sixteen private receipts on C: and D:. All four cases block a second model lease and then durably prune without altering completed bytes. This is implementation input, not a completed pruning migration. Next preserve immutable inventory, conflicting-publication exclusion, exact conditional retirement, snapshot-bound acceptance and successor-safe rollback while detaching IO; include new/changed receipts and journals, active owners, failed/ambiguous saves, cancellation and shutdown. Do not reuse row-completion proof as receipt-absence proof without extending and testing that contract.


## 8.54 receipt-retirement transaction

Receipt-only recovery cleanup now checks journal absence and persists its catalog outside recovery's own model lease. An owned destination/journal reservation excludes conflicting in-process publishers through acceptance; real publishers now honor both journal names even when a reused identifier points elsewhere. A distinct snapshot-bound absence proof accepts only catalog snapshots without the exact selected keys, including a newer containing writer. Failed/deferred/unknown outcomes restore only missing original keys and preserve later receipts. All 22,769 selected checks pass across 21 final-binary-matched runs, including 267 pruning checks per drive. Five products and sixteen fixtures built from 510 inputs; five native core units rebuilt and nineteen native units retained by include-closure audit. A real one-request 4 MiB lifecycle/hash/normal-exit/post-exit-flush smoke passes. Removed 14,422 generated files / 243,500,410 logical bytes plus the smoke payload, and 15,147 empty owned test directories. Installed desktop remains 8.32. External namespace/caller audits, further persistence paths, installed acceptance, zone policy and IDM finishing-speed parity remain open. [Implementation, proof contract and remaining gates](recovery-pruning-8.54.md).
