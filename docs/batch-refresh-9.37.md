# Batch metadata refresh without window-thread request preparation — UDM 9.37

UDM 9.37 / source extension manifests 0.73.1 continues the batch responsiveness work in 9.36. Initial rows were prepared on a worker, but accepting batch credentials still captured fresh policy and then prepared every request on the window thread. Per-file browser request edits also rebuilt their metadata request synchronously. The old path parsed URLs, scanned saved-login policy, combined headers and browser context, allocated request arrays and refreshed native cells in that callback.

## Ownership and behavior

BatchRefreshInput captures only editable source identities, addresses, names and protected override blobs into an owned snapshot. It does not copy row metadata, descriptions, destinations or checkbox state, and it does not borrow the mutable window rows. The immutable browser source cohort retains the existing joined-reader lifetime contract.

For a bulk refresh, BatchInitialTask reads current settings under Manager and releases the lock before projection. BatchRefreshProjection prepares origin/fallback names, request-scoped observed-response hints and browser names/extensions, matching headers/proxy/session, saved-login invalidation and bounded preview work on the owned worker. Added rows are omitted. The result changes only advisory metadata and name caches in the live rows; destination/description edits and explicit check choices remain there.

A single browser-row refresh captures that row alone and uses the existing preview owner's policy snapshot, preserving the previous behavior. It does not reacquire Manager or replace the whole preview owner. The worker cancels/replaces only that source slot. Other in-flight rows retain their slots and policy; editing one Referer does not reprobe the other links.

Old size/type evidence is invalidated before starting the refresh. Suppressed source IDs prevent an old completion from repopulating an invalidated row after a preparation or thread-start failure. Successful publication removes suppression for the prepared IDs. Bulk failure retains the edited draft with inline Retry; closing cancels and joins the preparation reader without accepting a download or clearing another command's ownership.

BatchVirtualList.invalidateCells clears its callback cache and invalidates the viewport once. Refresh no longer sends native cell updates for every row. Deferred batch completion callbacks are mutable so prepared vectors and preview ownership move into the live view rather than being copied from const captures. Necessary input snapshots and advisory-state invalidation still run in the command callback; this is not a zero-work window-thread claim.

## Current-build validation

All five native products report file/product version 9.37. Chromium and Firefox source manifests are 0.73.1. The serialized build freezes 625 repository inputs, generated COM inputs and linked core hashes before compiling; Bridge recompiles for the version dependency.

All 796 targeted backend assertions pass across 32 C:/D: runs and 16 suites. The new refresh projection suite contributes 22 assertions per drive. It covers owned request snapshots, matching/mismatched browser context, observed-hint reuse and invalidation, protected overrides, saved logins and proxy policy, omitted added rows, single-slot identity, invalid source sets, cancellation, projection outside Manager, a single-row refresh completing under a held Manager, and unchanged personal-independent fixture catalogs.

The 1,394 current UI assertions comprise 53 large-list, 261 layout/transition, 141 opening/refresh/late-metadata, 108 admission and 831 Main/Options checks. Owned Win32 desktops are created and closed without switching the interactive desktop or sending physical user input.

Two new bulk cases hold the actual projection worker at a controlled boundary. The real dialog has editing frozen, old size evidence invalidated and Cancel enabled; its pulse and paint return while projection remains held, and a separate lock check confirms that worker owns no Manager lock. One case releases and completes, while the other cancels there. Existing held-policy-writer, error/Retry and late-metadata checks still pass.

The per-file case opens the actual browser File Properties dialog, changes its Referer, saves it and holds Manager. It checks immediate properties evidence invalidation, only the edited row becoming Unknown, pulse/paint returning while the worker is held, and the refreshed size appearing before Manager releases. The local origin records exactly one authenticated metadata request with no challenge. The other row keeps its observed size and explicit check choices remain intact. This tests a real native batch properties workflow using owned source data; it does not control an installed browser extension.

The 10,000-row browser fixture also invokes the actual refresh command. In this individual local run the command returned in **44.3 ms**, and the full refresh became available in **230.8 ms**. It checks real last-row metadata and identity, unchecked browser defaults, available Cancel, and publication while a later catalog writer holds Manager. Observed-response hints avoid network probes in this fixture. These times are not repeated medians, installed-app/IDM comparisons, total transfer times or background-flush measurements.

Admission still completes all ten selected authenticated files with exact payloads and exactly one transfer request each. This targeted qualification does not prove the entire backend registry, every editor validation path or full IDM parity.

[Build](validation/batch-refresh-9.37/build.json), [input inventory](validation/batch-refresh-9.37/dependency-inventory.json), [changes from 9.36](validation/batch-refresh-9.37/source-change-audit.json), [versions](validation/batch-refresh-9.37/product-versions.json), [backend](validation/batch-refresh-9.37/backend-checks.json), [large refresh](validation/batch-refresh-9.37/batch-large-checks.json), [refresh measurements](validation/batch-refresh-9.37/refresh-measurements.json), [opening and real properties workflow](validation/batch-refresh-9.37/batch-opening-checks.json), [transitions](validation/batch-refresh-9.37/batch-transition-checks.json), [admission](validation/batch-refresh-9.37/batch-admission-checks.json), [Main/Options](validation/batch-refresh-9.37/modal-checks.json).

## Preliminary boundaries and retained failures

Preliminary backend checks pass 22 assertions, the corrected opening/refresh fixture passes 141 and the extended large-list fixture passes 53. These developing 9.37 sources link qualified 9.36 cores, so they are excluded from current-build totals.

The first bulk projection opening fixture passed 124 assertions before adding the per-file workflow. An intermediate build was rejected by its source hash guard because a size assertion was strengthened during compilation; no UI fixture was started for that binary.

The following UI attempt passed the per-file case, then timed out in the older late-selector case because the new File Properties handler reused phase 8 without restricting itself to browser-row. The corrected handler is scoped to its mode. Cleanup diagnostics are also saved before the final close assertion, retaining original failure information. Reversing exactly those two harness changes reconstructs the failed test source hash; no production source changed for that correction. The failed record is retained, rather than counted as a passing run.

[Boundary summary](validation/batch-refresh-9.37/preliminary-boundaries.json), [aborted build](validation/batch-refresh-9.37/excluded-preflight/opening-row-aborted.json), [failed UI attempt](validation/batch-refresh-9.37/excluded-preflight/opening-row-phase-collision-failure.json), [exact harness correction](validation/batch-refresh-9.37/excluded-preflight/harness-correction.json), [corrected preliminary UI](validation/batch-refresh-9.37/excluded-preflight/batch-opening-ui-result.json). Preliminary inventories, backend results and logs are retained alongside them.

## Cleanup and remaining work

Removed 41 owned fixture/compiler roots and 43 temporary build files (451.5 MiB logical bytes) after exporting sanitized evidence. Current products/required cores, previous qualified 9.36 products, installed 8.68 / extension files 0.72.32, both personal catalogs and shared media tools are preserved. No media helper was copied. [Cleanup](validation/batch-refresh-9.37/cleanup.json), [preservation](validation/batch-refresh-9.37/final-preservation.json).

No installer, installation, commit, push or release is included. Form::OnCommand still dispatches a queued action ID without checking whether its control has since been disabled. Next gate obsolete edit/selection commands while refresh or admission owns the draft, and reject a projection whose edited request identity has been superseded. The present tests verify ordinary input freezing, not queued-command races. Per-file validation/protected-credential editing, very large sorting/filtering work, exceptional teardown and remaining Main/modal/shutdown waits also remain to audit. Actual owner/minimize/multi-monitor transitions, installed extension capture, complete media output and live IDM comparisons still need their separate validation. The original inside/out parity goal remains open.
