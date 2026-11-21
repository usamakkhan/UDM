# Owned toolbar settings and prepared skins — 9.57 / extension 0.73.21

Move toolbar size, captions, looks, visibility, skin selection, layout persistence and Reload onto the owned appearance worker. Merge only the changed fields, prepare immutable skin pixels outside the catalog lock, render without reopening skin files, and restore the current layout on failure without a second model read. The held-model toolbar command returns in about 22 ms versus 2.56 seconds on frozen 9.56. Build five native 9.57 products and pass 251 backend plus 1,293 optimized UI assertions. Browser source manifests advance to 0.73.21. Installed 8.68 remains unchanged; installed IDM parity and the prior total-time gate remain open.

## Blocking owner and scope

Toolbar commands previously read preferences, loaded external skins and saved settings on the UI thread. The frozen 9.56 control takes 2556.5061 ms to handle the first Small Buttons command while a deliberate 2.5-second catalog writer is held. The same native command in the current fixture returns in 22.4696 ms before the writer releases. The command is accepted promptly; persistence still waits for the writer.

These single controlled observations establish responsiveness under catalog contention. They are not a timing distribution, a matched whole-transaction comparison, an installed IDM benchmark or a promise of zero delay. Native drawing and other UI work remain. The prior total-time gate and full inside/outside IDM parity stay open.

## Intent and settings ownership

AppearanceSettingsTask now accepts toolbar size, caption mode, built-in look, visibility toggle, external skin selection, layout and read-only Reload intents. Explicit paths and layout vectors are copied into the draft; no Windows control or image-list handle enters the worker. The existing accepted-work guard, weak wake, timer acknowledgement, joined lifetime and deferred Close/Exit protocol remain in use.

The worker merges into the latest settings under the catalog lock. Visibility toggles evaluate the live value. Built-in looks clear the external skin path. Layout saving changes ToolbarLayout and its compatible ToolbarOrder/ToolbarHidden fields together, preserving repeated separators, explicit empty layouts and all unrelated settings. No-op customization compares the native layout with the last actually rendered layout, so merely closing Customize cannot overwrite an unrelated newer setting. Initialization notifications do not save an empty not-yet-rendered toolbar.

## Skin preparation and rendering

An explicitly selected skin is validated and decoded before any settings write, without holding Manager. The result owns an immutable shared ToolbarSkin containing the descriptor and raw BMP pixels. A later removal or edit of those files does not change that operation's prepared result. Other appearance changes prepare the currently saved skin after releasing the catalog lock. A previously saved unavailable skin retains its path and returns an owned fallback diagnosis, matching the existing usable-built-in-icons fallback; a newly selected invalid skin fails before changing preferences.

Acknowledgement installs a scoped prepared-result reader. The renderer uses it only when the requested skin path matches that owned result. It borrows the immutable pixels instead of reopening the descriptor and BMP files, and chooses strips and dimensions using the actual UI DPI at rendering time. The scope restores its previous pointer even on failure, and no pointer survives the result's lifetime. Windows image-list creation stays on the UI thread; worker skin preparation uses byte data and filesystem APIs, not borrowed GDI resources.

The sixteen-scenario fixture includes both initial skin selection and a changed skin already on disk. Its captured-result hook removes the descriptor and BMP before acknowledgement. Exact native icon size and center color still match the prepared pixels while another writer holds Manager. Export also checks that both source files are absent. A later worker operation reloads changed pixels without altering an earlier immutable result.

This does not make every skin-related path asynchronous. Direct legacy/custom-skin appearance repaints retain their existing fresh-file behavior. Directory enumeration and descriptor-label discovery in the toolbar menu remain on the UI thread. Built-in PNG overrides are still read on the UI thread when explicit Reload invalidates their cache. CPU resampling and native image-list construction remain UI work.

## Native customization and rollback

Toolbar button definitions read a nonblocking appearance snapshot. A button rebuild owns one complete settings value through every definition and text-row update, preventing both model waits and mixed settings within the same rebuild. Menu preference reads use an immediate model read when available and otherwise the last rendered appearance; skin-directory scanning is a separate remaining IO concern.

Native customization keeps its existing add/remove/reorder/Reset workflow and commits when customization closes. Shift-style layout notifications retain their existing save boundary. The task owns the requested layout. Before attempting a write, the worker captures the current layout for failure rollback. The UI also retains its last rendered layout for failures before that model snapshot can be captured.

On failure, restore the worker-owned current layout using an owned copy of the last rendered appearance. Advance only its layout fields; do not pretend unrelated font/style settings have been rendered. Restore native button enablement and the toolbar's previous enabled state without reacquiring Manager. The fixture verifies rollback while another writer holds the catalog, cancels deferred Exit and then retries successfully. Editing notifications are suppressed during an accepted save, while drawing remains available.

## Reload and error semantics

Reload captures current live settings, prepares the current skin and returns refreshed=true with saved=false. It invokes no persistence checkpoint and does not apply save-time remembered-rate normalization to a read-only result. The UI invalidates built-in icon generations only when that result is acknowledged. A display failure after Reload is reported as a redraw failure, rather than falsely saying settings were saved. Ordinary save failures still preserve the previous catalog and cancel deferred Exit for explicit retry.

Accepted preparation remains joined work. Long or stalled filesystem reads are not made cancellable by this increment and may defer an accepted Exit. This and broader interruption/error matrices remain open. The implementation does not remove persistence flushing or falsely acknowledge a save to improve a timing number.

## Qualification and fixture maintenance

All 251 backend assertions across eleven suites pass. The 33 new toolbar checks cover sizes, caption modes, built-in looks, visibility, narrow merges, compatible layout fields, repeated separators and empty layouts, lock-free skin decoding, immutable/fresh pixels, invalid drafts, missing skin rejection, real checkpoint failure with owned rollback, explicit retry, unavailable saved-skin fallback and genuinely read-only Reload. The existing 31 appearance-task checks continue to pass.

All 1,293 optimized UI assertions pass: 157 new toolbar checks across sixteen scenarios, 322 appearance-command lifecycle checks, 284 full toolbar checks, 25 image-cache checks, 469 menu checks and 36 appearance repaint checks. All children attest separate owned desktops, without switching the interactive desktop or injecting physical input.

The full toolbar fixture initially inspected later operations before accepted saves were acknowledged. One run recorded 89 checks and failed Reset persistence because the next Customize dialog had not opened while the toolbar remained gated. After adding the bounded post-Customize wait, a second run recorded 106 checks and found another premature inspection after indexed skin selection; 3D Style also needed its bounded wait. Both failed runs and their binary/input proofs are retained as diagnostics and excluded from passing counts. No assertion was removed or relaxed. The final ordinary fail-fast optimized run executes all 284 assertions.

The original 670-file build inventory differs from the final qualification inventory only in native/ToolbarUiTests.cpp. Production inputs are unchanged. The new toolbar and existing appearance-command runs use the initial inventory; the final toolbar, cache, menu and repaint runs use qualification epoch 3. Both intermediate and final inventories are retained explicitly. All five native products have file version 9.57. The frozen 9.56 control verifies App, MainFeatures, ToolbarUi, AppearanceSettingsTask, AppearanceCommandUi, Version, resource and core inputs; its fixture differs only by redirecting the App include.

## Delivery and next work

Both browser source manifests are 0.73.21, with no capture behavior change. No installer, installation, commit, push or release is created. Installed 8.68 and extension files 0.72.32, both personal catalogs, shared media helpers, prior products and required core objects are preserved. Owned test/compiler outputs are removed after sanitized evidence export; no media-helper copies or network download payloads are added.

Next move toolbar directory/descriptor discovery and remaining passive custom-skin IO to cancellable owned preparation with coherent menu selection IDs. Then address browser-integration, drop-basket and double-click preference paths, startup, standard standalone GUI build profiles, long-read cancellation and the wider modal/shutdown matrices. A fresh installed/browser IDM comparison and the earlier whole-background-work performance gate are still required.

[Validation status](validation/toolbar-settings-9.57/status.json) contains inventories, hashes, native assertions, source-file deletion proof, baseline reconstruction, compiler records, failed fixture diagnostics, cleanup and preservation records.
