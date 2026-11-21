# File Info continuation after address recovery — UDM 9.2

UDM 9.2 / source extension manifests 0.72.66 removes synchronous prefetch stopping, saved-request reads and credential resolution from the parent File Info callback after address recovery closes. One owned continuation prepares those values before the parent resumes its background admission workflow. This is a responsiveness and request-identity change; it does not establish live IDM timing equality.

## Accepted request handoff

The recovery worker returns the private request selection captured immediately after a successful Save. Its UI wrapper returns that owned value only when recovery returns an accepted outcome. Failed, cancelled or stale-review replies cannot authorize the parent continuation. Existing callers remain compatible through an optional output parameter; private selections are never exported to evidence.

FileInfoRefreshTask checks membership and the saved selection before ordinary prefetch stopping, passes that selection to Manager's guarded stop, and checks it again afterward. It never holds an outer recursive model lock while waiting for the transfer to stop. A route or target change produces an error without returning usable launch state. Metadata changes outside request identity remain compatible.

Ordinary recovery prepares the current URL, protected headers, parent page, remembered-site settings and Basic login values on its worker. Media keeps its previous no-extra-stop behavior and does not load ordinary site credentials. The resulting selection guards the following prefetch admission, including capacity retries. The continuation neither changes the user's uncommitted destination/description/category/queue nor silently confirms the download.

## Parent window and cancellation

The original File Info remains visible while the worker waits. Draft controls are disabled, Cancel stays available, and pulses return without acquiring Manager. Successful completion updates the address/login controls and starts the existing owned admission task. Errors retain the draft and expose Refresh again, with Start/Later blocked until the saved request has been prepared successfully.

Cancel owns a joined decision command: it cancels advisory model waiting, drains any accepted stop, then performs the established File Info cancellation. The window stays alive until that work finishes. A failed cancellation restores the retained controls and requires recovery before another Start/Later attempt. Exceptional modal teardown still joins outstanding owners before releasing Manager.

Workers publish their result before posting the weak Form task identity. Throwing notification callbacks are contained. Test-only page interception prevents the isolated window fixtures from opening a personal browser; production page launching is unchanged.

## Qualification

Build all five native products as **9.2**, with both source extension manifests at **0.72.66**. Freeze 595 repository inputs and generated COM inputs, and verify the reused core objects. [Build](validation/file-info-refresh-9.2/build.json), [dependency inventory](validation/file-info-refresh-9.2/dependency-inventory.json), [product versions](validation/file-info-refresh-9.2/product-versions.json).

Pass **3,334 backend assertions across 74 C:/D: runs**. The new continuation suite contributes 31 checks per drive, including real stop checkpoint failure/rollback, selection changes before and after stopping, remembered credentials, held-model cancellation, accepted-stop draining and publication before throwing wake-ups. Recovery adds one check per drive for its exact private saved-selection handoff. A separate 31-check preflight is excluded from these counts. [Backend evidence](validation/file-info-refresh-9.2/backend-checks.json), [preflight](validation/file-info-refresh-9.2/preflight.json).

Pass **698 actual UI assertions**: 353 Main/File Info, 257 pending admission and 88 progress menu. Two new scenes save a real staged replacement through the recovery child, then hold Manager before parent preparation can read it. The actual parent pulses and paints while preserving the frozen destination draft and available Cancel. The normal path displays the fresh URL/login controls; the cancellation path owns cleanup until the writer is released. Test page launching is intercepted. The fixtures use separate owned Win32 desktops, never switch the personal desktop and inject no personal input. [Main UI](validation/file-info-refresh-9.2/ui-checks.json), [pending UI](validation/file-info-refresh-9.2/pending-ui-checks.json), [menu UI](validation/file-info-refresh-9.2/progress-menu-ui-checks.json).

Remove **78 owned fixture directories and 87 temporary compiler/test files**, including displaced prior core cache, after exporting compact evidence. Logical bytes removed: 559,189,456; this is not measured physical free space or drive-wear savings. Preserve current products/required core objects, previous 9.1 products, installed files, both personal catalogs and installed helpers. [Cleanup](validation/file-info-refresh-9.2/cleanup.json), [preservation](validation/file-info-refresh-9.2/final-preservation.json).

## Remaining work and delivery boundaries

The older CapturePresentationUiTests driver still assumes synchronous address/media Save and immediate browser-presentation acknowledgement. Its migration and full qualification remain pending; the new owned-parent fixtures do not substitute for that whole suite. Caller restoration/admission, category/scheduling and final teardown also retain other synchronous work. Live browser capture, pre-Start size and complete download/IDM comparisons still need validation of a subsequently installed build.

Installed UDM remains 8.68 / extension files 0.72.32. Browser JavaScript is unchanged. No installer, installation, personal-desktop control, commit, push or release is included. Full inside/out IDM parity remains unproven.
