# Owned download-address recovery â€” UDM 9.1

UDM 9.1 / source extension manifests 0.72.65 opens address and media recovery before waiting for the download model. One owned worker prepares the saved request, reads browser offers, saves the reviewed replacement and acknowledges displayed receipts. The window remains responsive during those operations. This is an architectural responsiveness change; live IDM latency and visual parity remain unverified.

## Session and request ownership

The session owns one lazy persistent worker, a single queued command and one UI-consumed future. Completion publishes the result before posting a weak Form task identity. Periodic advisory reads reuse that worker and start only from the ordinary cadence, preventing a completion-message/read loop. Cancellation interrupts advisory model acquisition, while an accepted Save and its cleanup retain ownership until they settle. Closing a session that never began does not require acquiring Manager.

Begin saves the existing paused recovery state and captures the original private request selection. Later commands verify membership and that selection under the same model lock. File Info recovery cannot submit Save and resume. Save compares both the current decoded browser candidate and pending receipt cohort against the immutable view the user actually saw. A changed offer returns for explicit review without applying it. A successful Save cannot replay within the session; storage failures retain an explicit retry path.

Decoded media plans are shared immutably and reused while the encrypted candidate is unchanged and current. The UI compares an owned offer revision rather than repeatedly parsing or cloning those plans. Diagnostics must never serialize private candidate, header or selection values.

## Window behavior

A small bootstrap window paints immediately with Cancel. Worker completion populates that same HWND, preserving the existing ordinary/FTP and media workflows. Controls are replaced only after their current pulse returns, with retired control IDs. Save stays unavailable until details are ready; media requires a matching capture. Media page markers are prepared on the worker. Browser-owned presentation is requested once at initialization.

Cancel during an accepted Save leaves the window present, disables repeated Cancel, waits for the save and then settles cleanup. Close acknowledges only the immutable receipt cohort that was displayed; later captures remain pending. A cleanup-save failure shows Retry close without replaying an accepted address change. Parent File Info polling pauses while the child owns recovery, and a FileInfoDialogOwner lifetime guard prevents main-window shutdown from destroying its Manager.

## Qualification

Build all five native products as **9.1**, with both source manifests at **0.72.65**. Freeze 593 repository inputs plus generated COM inputs, compile affected core units and reuse only verified unchanged objects. [Build](validation/download-refresh-9.1/build.json), [dependency inventory](validation/download-refresh-9.1/dependency-inventory.json).

Pass **3,270 backend assertions across 72 C:/D: runs**. The new suite contributes 33 checks per drive: ordinary/FTP recovery, real adaptive replacement, immutable offer reuse, request-route invalidation, stale Save review, replay rejection, real save/receipt failures with explicit retry, exact displayed-token cleanup, cold cancellation while a writer holds Manager, and persistent worker ownership. The separate 33-check preflight is excluded from these counts. [Backend evidence](validation/download-refresh-9.1/backend-checks.json), [preflight](validation/download-refresh-9.1/preflight.json).

Pass **678 actual UI assertions**: 333 Main/File Info, 257 pending admission and 88 progress menu. New scenes hold a real Manager writer before opening recovery, verify bootstrap painting and protected shutdown, exercise stale browser review followed by an accepted Save while Cancel is pressed, and hold media polling/close behind the model. Each uses owned isolated Win32 desktops without switching the personal desktop or injecting personal input. [Main UI](validation/download-refresh-9.1/ui-checks.json), [pending UI](validation/download-refresh-9.1/pending-ui-checks.json), [menu UI](validation/download-refresh-9.1/progress-menu-ui-checks.json).

The separate legacy CapturePresentationUiTests driver assumes synchronous recovery Save/review and has not been migrated or qualified in this increment. These selected checks do not establish a live browser workflow, IDM visual/timing equality or complete project qualification.

Remove **78 owned fixture directories and 119 temporary compiler/test files** after exporting compact evidence. Preserve current products/required core cache, previous 8.99 native products, installed 8.68, both personal catalogs and media helpers. Logical bytes removed: 685,856,732; this is not a measurement of physical free space or drive wear. [Cleanup](validation/download-refresh-9.1/cleanup.json), [preservation](validation/download-refresh-9.1/final-preservation.json).

## Remaining work and delivery boundaries

The parent File Info follow-up still calls endPrefetch and reads refreshed login/request settings synchronously after the child returns. Final teardown, caller restoration/admission, category and scheduling paths require further ownership work. Dialog execution and persistence can still wait on Windows scheduling, disk activity and other model owners; no zero-delay claim is made.

Installed UDM remains 8.68 / extension files 0.72.32. Browser JavaScript is unchanged; only the source manifests advance for this increment. No installer, installation, personal-desktop automation, commit, push or release is included. Full inside/out IDM parity remains unproven.

## Superseded draft

The 9.0 / 0.72.64 focused preflight passes 33 checks beside the existing verified media helpers, after an initial helper-location failure. The UI compilation fails on protected Form caption access. Replace that access with SetWindowText, retain the failed evidence, stop only the exact owned build runner and its verified descendants, and rebuild fresh as 9.1 / 0.72.65. The draft is not qualified.
