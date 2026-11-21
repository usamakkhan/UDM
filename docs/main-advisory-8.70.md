# Main-window advisory reads, 8.70 / browser 0.72.34

List tooltip notifications previously read a live job filename outside the Manager lock, then obtained a blocking lock to describe its queue. Toolbar and selection-file advisory refresh also waited for the model. A catalog writer could therefore delay these UI reads, and the tooltip filename read could race with an edit.

## Implementation

`DownloadListTooltip.hpp` formats only immutable `DownloadListRow` and UI-owned `NavigationTreeSnapshot` values. Navigation snapshots include friendly queue descriptions, updated even when the tree structure is unchanged. The tooltip handler reads no live Job, Manager or filesystem data. Empty, missing and out-of-range rows are guarded; membership and missing-queue wording are preserved. Notifications during a row rebuild return empty text rather than a mixture of snapshots.

`commandStates` has an explicit advisory busy result. Toolbar and selection-file refresh opt into a try-lock; if the catalog is busy, toolbar buttons retain their existing states. Later regular refresh reads current state. Actual command validation retains its existing current-model lock and fresh-file policy: a skipped advisory read does not authorize a stale action or discard a command.

The 8.69 nested modal-size fix remains included. Transfer and all core objects except Bridge retain their 8.69 hashes. This increment does not change the installed 8.68 transfer implementation or make a new whole-download timing claim.

## Build and evidence

All five production binaries have version **8.70**; Chromium and Firefox source manifests are **0.72.34**. [Build identities](validation/main-advisory-8.70/build.json).

Eleven backend suites on C: and D: pass **1,624 assertions**. New tooltip cases cover immutable old snapshots, later filename/queue updates, unchanged tree structure, explicit and legacy membership, missing queues and null rows. Existing modal-size, reception-cache, writeback, checkpoint, publication and ownership checks also pass. [Backend assertions](validation/main-advisory-8.70/backend-checks.json).

The actual MFC main-window fixture passes **20 additional assertions** on its own separate Windows test desktop. It holds Manager on another thread and invokes the production tooltip notification, toolbar refresh and selection-file handler. They return before that thread releases the model; displayed tooltip/button state survives. After release, filename and stopped-queue descriptions update, advisory state becomes available, and an ordinary Resume decision rejects the current completed job. Existing modal-size/control/selection assertions also pass. The owned frame exits normally and the test desktop closes, with no desktop switching or user input. This is component qualification, not operation of the user's browser or IDM. [UI assertions](validation/main-advisory-8.70/ui-checks.json).

The first independent UI fixture compile failed with C3861 because two assertions omitted the MainWindow qualification on `toolbarCommand`. The fixture references were corrected and rebuilt; this failure is retained and excluded from passing validation. Production/backend compiler inputs were unchanged by that fixture-only correction. [Failure record](validation/main-advisory-8.70/initial-fixture-build-failure.json).

## Delivery and remaining work

**8.70 is a built, qualified development candidate. Installed UDM remains 8.68 / extension manifest files 0.72.32**, with its installed hash and both personal catalog hashes preserved. No new installer, installation request or browser activation is part of this increment.

This closes specific advisory tooltip/toolbar waits. Normal full-history refresh, navigation snapshot acquisition, pending browser presentations, authentication/review discovery and authoritative commands still have separate synchronous model dependencies. Next, assemble a coherent main-window read snapshot that can skip a busy model without partially changing filters, rows or selection; apply it only after releasing the lock. Fresh live File Info/browser/IDM timing remains a separate gate.

All **23 owned fixture roots** and **24 temporary test executable/object files** were removed after preserving receipts. Qualified products, shared core objects, source, rollback, installed app and personal catalogs remain. No download payload was generated for these UI checks. [Cleanup](validation/main-advisory-8.70/cleanup.json), [preservation](validation/main-advisory-8.70/final-preservation.json).
