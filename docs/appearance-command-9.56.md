# Accepted appearance commands — 9.56 / extension 0.73.20

Move dark mode, category visibility, font selection/reset and tray-style saves onto an accepted worker with narrow drafts and fresh-field merges. Render acknowledged values without reading the shared catalog again; preserve live unrelated settings, rollback/retry, guarded controls and deferred Close/Exit. The held-model dark command returns in about 5 ms versus 2.54 seconds on frozen 9.55. Build five native 9.56 products and pass 218 backend plus 1,420 optimized UI assertions. Source browser manifests advance to 0.73.20. Installed 8.68 remains unchanged; installed IDM parity and the prior total-time gate remain open.

## Problem and measured scope

View commands previously copied shared preferences and saved the full settings object on the UI thread. A catalog writer could block the entire command. The frozen 9.55 control takes 2540.5837 ms for the first dark-mode command while a deliberate 2.5-second catalog writer is held. The same native WM_COMMAND in the 9.56 fixture returns in 5.0742 ms, before that writer releases.

This establishes command responsiveness under model contention. It does not remove the accepted save's need to wait for the writer, prove faster whole-transaction completion, establish an installed IDM comparison, or close the earlier total-time gate. These are single controlled observations rather than a benchmark distribution. The new UI keeps displaying the last acknowledged appearance until persistence succeeds.

## Draft and persistence ownership

AppearanceSettingsTask accepts five typed intents: toggle dark mode, toggle category visibility, a complete font choice, font reset, and a tray style. Seven View commands use those intents. Font selection captures face, logical height and weight after the native chooser returns OK; cancellation retains its existing no-save path. The worker never captures a control, font handle or window pointer.

Validate the intent before acquiring the shared decision guard. The UI reserves that guard atomically and starts one accepted save. The worker acquires the catalog directly; no timed polling or artificial pacing is added. It copies the latest settings under the lock, changes only its intent's fields, and calls the canonical settings save. Toggles evaluate the live value at execution, not an earlier rendered value. Reset takes the canonical font defaults. Unrelated fields survive concurrent writers.

Prepare the owned result before persistence can commit. Include Manager's positive-rate remembered-limit normalization so the acknowledged full settings agree with the actual durable settings. The backend fixture deliberately supplies a stale remembered rate and verifies this canonical result. Persistence failure retains the original error, rolls back current settings and leaves the prior catalog intact. No failed save is acknowledged as successful.

The guard stays reserved through result acknowledgement and worker join. Publication occurs after the model lock is released. A weak notification only wakes the UI; the task owns the result. A failed notification does not discard accepted work, and the existing timer can poll readiness. Repeated cleanup is safe, and owner destruction joins accepted work before Manager can go away.

## UI and rendering

While the save is pending, retain the old rendered appearance, keep the main window enabled, disable the toolbar window and conflicting buttons, and gate conflicting command advice. Native font choices disable without reading the catalog. Toolbar and queue popup preparation skips its model-dependent rebuild while this save is pending. Local selection, sorting, painting, timers and a deferred Exit remain available. Toolbar advice and its prior enabled state are restored from owned UI values.

After acknowledgement, full appearance changes call applyAppearanceSnapshot with the owned committed result. They do not reacquire Manager to read preferences. Category hiding returns focus to the download list. A tray-only acknowledgement borrows the owned result for updateTray and advances only the cached TrayIcon field; it must not claim that unrelated font/layout fields have been rendered.

Normal Close waits for acknowledgement and then applies the current Close to Tray policy. Show or independent local activity can cancel that normal deferred close while preserving the accepted save. Explicit Exit stays queued through Show and duplicate commands. A save failure restores command availability, preserves the prior render and cancels deferred Exit for explicit retry. Direct window destruction joins the accepted save and disarms weak notifications. Final Exit retains the accepted setting and newest local list layout through background cleanup.

A Windows rendering error after a successful save is reported explicitly as saved-but-not-displayed and cancels the deferred close. It is not relabeled as a persistence failure. General recovery from arbitrary rendering failures remains a wider open error-handling concern; this increment does not claim to verify every such OS failure path.

## Qualification

All 218 backend assertions across ten suites pass, including 31 new checks for acceptance, duplicate/foreign guard handling, fresh-value toggles, narrow merges, canonical rates, all tray choices, font defaults, invalid drafts, real checkpoint failure/rollback, retry, failed notifications, post-unlock publication and joined destruction.

All 1,420 optimized UI assertions pass: 322 new appearance-command assertions across fourteen scenarios, 469 main-menu checks, 284 toolbar checks, 309 existing limiter lifecycle checks and 36 appearance repaint checks. The fourteen scenarios cover dark, categories, an owned font choice, font reset, three tray styles, failed save/retry, Close to Tray, cancellation of normal Close before and after acknowledgement, explicit Exit, failed Exit/retry and direct window destruction. The new fixture also holds a second writer during acknowledgement to detect a reintroduced live preference read.

Existing menu and toolbar fixtures now wait, with explicit deadlines, for accepted appearance saves before inspecting committed settings. Their actual visual/menu assertions are retained. Generated toolbar fixtures contain their small, large and high-DPI strips. Native UI children attest their own separate desktop before creating windows; tests neither switch the interactive desktop nor inject physical input.

Production and test inputs match one frozen 668-file inventory. Existing core objects are hash-verified and Bridge is rebuilt for the version change. All five native products carry version 9.56. The baseline uses hash-verified frozen 9.55 App, MainFeatures, ToolbarUi and Version inputs, the same fixture with only its App include redirected, and the preceding core set. Its expected failed first assertion is retained as a failed control, not counted among passing checks.

## Delivery and next work

Both browser source manifests advance to 0.73.20 without a capture behavior change. No installer, installation, commit, push or release is created. Installed 8.68 and extension files 0.72.32, both personal download catalogs, shared media helpers, previous products and required core objects remain unchanged. Owned temporary fixtures/compiler outputs are removed after sanitized evidence export; no new media helper copies or network payloads are created.

Next extend the accepted narrow-intent path to toolbar size, style, look, visibility, skin validation/loading and drag customization. Browser integration, drop-basket and double-click preference paths, startup, live browser/IDM comparisons, remaining shutdown/interruption matrices and the earlier total-time gate remain open. Complete inside/outside IDM parity is unproven.

[Validation status](validation/appearance-command-9.56/status.json) links the exact inventories, hashes, assertions, baseline reconstruction, compiler records, cleanup and preservation evidence.
