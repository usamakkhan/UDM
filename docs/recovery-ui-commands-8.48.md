# UDM 8.48: guarded UI recovery commands

UDM 8.48 migrates the five identified outer recovery scopes in progress actions, unfinished Properties, pending File Info cancellation, Download File Info cancellation and browser-receipt cancellation. Native version advances to 8.48; both extension source manifests advance to 0.72.12. Browser JavaScript is unchanged. Installation remains deferred.

## Why releasing the UI locks alone was unsafe

The 8.47 recovery catalog transaction releases its own model lease during serialization, durable save and snapshot acknowledgement. An outer recursive UI lease still prevented other model users from entering that interval. Those outer leases also kept current-header patches and cancellation eligibility atomic. Removing them without a replacement would allow a stale Resume or Cancel to act on newly completed or newly accepted work, or overwrite a refreshed browser session with old Properties credentials.

The existing [recovery transaction design](publication-recovery-transaction-design.md) therefore required backend guarded continuations. This increment implements those continuations instead of dropping the checks.

## Progress Resume and Stop and Save

`Manager::resumeIf` executes an in-process predicate under the model lease before recovery and again after it. The predicate performs no IO. The regular Resume API delegates to the same implementation with no extra predicate, retaining active-child ownership and prefetch behavior. The progress adapter releases its own lease before calling the backend, and rechecks displayed action, current status/activity, relocation, live-recording state and authentication policy under the backend's final lease. A recovered completion wins over an obsolete Resume or recording retry. An authentication prompt can still be returned instead of queueing work.

Stop and Save validates its displayed action under the backend lease, saves the existing stop intent and stops an active recorder. Its idle continuation releases that lease before guarded Resume. The final continuation requires the same retained live-recording intent and rejects Complete or Awaiting confirmation. The initial stop-intent save retains its existing lock and durability behavior; this work does not detach every progress-command save.

## Properties edits merge with current headers

`Manager::configureProperties` prepares recovery before taking the mutation lease. Under its final lease it applies `PropertiesHeaderPatch` to the current job, validates the configured row, swaps it and saves. Untouched session headers survive a concurrent refresh; typed logical changes apply to the refreshed record. A sensitive assignment checks the current URL origin again. Failed persistence swaps back the exact pre-edit current row, preserving independent refreshed credentials and metadata. A recovered completion preserves the existing old-destination warning rather than applying unfinished-download edits to it.

Completed Properties retains its existing atomic `updateCompleted` path; that path does not invoke this recovery transaction.

## Cancellation remains atomic after recovery

`cancelUnconfirmedOffer` distinguishes an Awaiting-only pending offer from a newly opened File Info offer that also permits Paused/Failed removal. The backend's private recovery continuation evaluates eligibility under recovery's own lease before starting, and executes its final predicate/removal under that same final lease after recovery has retired its journals and released its file-publication owner. There is no unlocked gap between owner release and the guarded removal.

This distinction matters when an unconfirmed offer recovers to Paused: cancellation can still remove that unconfirmed job, while a newly completed or user-accepted job survives. Individual Start, current membership, activity and original-response protections are rechecked. A stale new-offer cancellation does not pause work already accepted in another File Info window. The normal Remove API uses the same reconciled mutation helper and keeps its existing validation, persistence rollback and owned cleanup.

Browser-receipt cancellation additionally rechecks protocol, accepted status, job ID and protected request fingerprint before recovery and inside the final continuation. A changed receipt prevents removal. Overlapping cancellation rejects a live publication owner before interpreting its provisional recovered status and again after ending prefetch. It cannot release the presentation claim as though removal succeeded while another recovery is still publishing. A still-busy Awaiting offer is reported for retry rather than acknowledged. These guards cover the extra outer wrapper found during this caller audit.

The private reconciliation body is wrapped so ordinary early returns, including an absent replacements directory, still reach a successful caller continuation. Exceptions do not run it. No callback is exposed through browser messages or persisted settings.

## Qualification and retained failures

All **3,374 focused checks passed across 20 runs**. The [validation receipt](validation/recovery-ui-commands-8.48.json) records the final suite totals, exact inputs/objects/product versions, app smoke and cleanup. The new **206-check** headless native fixture calls the actual progress adapter and backend contracts; it opens no browser or native window, and launches no downloaded files, clipboard or Explorer actions. It holds the real recovery catalog flush in sixteen cases: progress Resume, recording Resume, Stop and Save, recovered Properties completion, current-header merge, changed-origin refusal, failed final Properties save, pending and File Info cancellation of Complete/Paused recovery, newly accepted work, changed cancellation policy, actual browser-receipt Complete/Paused cancellation and a fingerprint changed during recovery.

At each held boundary, a model snapshot remains available while job/destination ownership stays active. Tests verify exact downloaded/staged bytes, the durable catalog's removal or retained row, journal retirement, current metadata, cancellation identity rechecks, and duplicate receipt cancellation preserving its presentation claim. The command matrix is run on C: and D:. Existing publication, catalog, ownership, replacement, File Info, header-draft, presentation and pending-cancellation suites are run against the same final core.

The first command fixture used an unsupported synthetic header and failed Properties validation as it should. Its diagnostic retry identified that error; the corrected fixture uses a supported header. Both initial results remain. The initial source freeze also rejected the final receipt-overlap and fixture additions; the affected core object was explicitly rebuilt and the command fixture recompiled before qualification. No initial link was installed or declared qualified.

An ordinary 4 MiB download matched its independent SHA-256 and size, used one origin request, exited normally, retired its publication journal and passed a separate post-exit output flush. Measured exit and durability tails were 202.221 ms and 224.056 ms. These are a single unpaired lifecycle smoke, not an IDM comparison or a speedup gate; previous measurements and losses remain. The payload was removed immediately.

All native C++ core units are rebuilt against the changed header and live-finish signature. Only the eighteen canonical external C compression inputs are retained. Media helpers are shared through a junction to the installed tools, with no new FFmpeg copy.

## Limits and next work

This closes the five named outer recovery scopes, not every model-lock or latency gap in UDM. Journal discovery, verification/open/hash/move IO, metadata/cleanup and receipt-only pruning remain locked, as do unrelated normal mutation saves and completed Properties. Continue the immutable IO preparation and revalidation phases in the transaction design, audit further callers as they are found, then verify installed visual/browser behavior separately.

The original slower sequential/split finishing cases and IDM performance parity remain unresolved. Native availability tests and the lifecycle smoke do not prove a millisecond timing target, installed behavior or identical IDM internals. No body/intent/catalog durability barrier or pacing policy is weakened. The installed app remains 8.32; no installation, extension deployment/reload, installer, release or push is performed.

## Source preservation and cleanup

The [8.47-to-8.48 source delta](../benchmarks/speed/experiments/recovery-ui-8.47-to-8.48.patch) reconstructs thirteen changed/added sources in both directions with exact hashes and recorded line endings. It does not promise bit-identical timestamped executables. Cleanup is limited to individually verified owned fixture payloads, redundant target objects and temporary source copies, with complete hard-link groups and reparse/parent checks. The current five products, rebuilt core, shared tools, prior qualified references, personal catalogs and installed executable remain. Removed **1,483 owned files totaling 213,764,701 logical bytes**, plus the separately removed 4 MiB smoke payload. All link groups resolved, with no skipped paths or directory deletion. These totals are not physical allocation claims.
