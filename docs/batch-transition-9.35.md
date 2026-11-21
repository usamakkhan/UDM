# Stable batch layout, theme and focus transitions — UDM 9.35

UDM 9.35 / source extension manifests 0.72.99 closes the loading-transition presentation gap documented in 9.34. Its initial loading controls did not have the browser dialog's resize anchors, and replacing them with the full view did not run the usual completed layout/theme pass or deliberately route focus to a new control. Resizing while the catalog was busy could leave Cancel behind and the new list/buttons at baseline coordinates until another resize.

## Reproduced baseline

An isolated source reproduction uses every frozen 9.34 repository input and linked core hash, plus the new transition test component. It opens the real batch dialog while a real writer holds Manager, changes DPI and enlarges the loading window. Its first case fails Loading Cancel stays at its intended position through resize and DPI changes. The final close assertion also fails because it retains that assertion error; the process still exits and its owned desktop closes. This is a source/Win32 reproduction, not a new installed UDM or IDM timing comparison.

[Baseline result](validation/batch-transition-9.35/baseline-9.34-transition.json), [matching source/core scope](validation/batch-transition-9.35/baseline-scope.json) and [baseline input inventory](validation/batch-transition-9.35/baseline-source-inputs.json).

## Implementation

Form.completeOwnedRebuild runs the existing anchor layout against the current client, refreshes the new controls' palette/scrollbar theme, and optionally sends the normal WM_NEXTDLGCTL dialog-management message to a valid enabled child. It does not resize or reposition the window, show another window, or call foreground activation APIs. It skips focus routing while the form is iconic.

Batch loading status stretches with the browser dialog; Retry and Cancel follow its bottom-right corner. The initial and fallback loading renderers apply the completed layout/theme pass. After initial data builds the full view, the same pass applies the user's current client geometry and gives focus to the actual file list. Palette application includes list background, text background and foreground; dark mode does not leave a default light list after asynchronous construction.

The existing monotonic control-ID rule remains intact. Queued commands for a removed loading control cannot target its replacement. Existing DPI handling continues retaining logical list-column widths, including an edited width through repeated DPI round trips. Import dialog dimensions, accepted command ownership, request context and checkbox policy remain unchanged.

## Current-build validation

Five native products build with file/product version 9.35; both source extension manifests are 0.72.99. The serialized build freezes 619 repository inputs, generated COM inputs and declared core hashes. Bridge recompiles for its version input. All 714 targeted backend assertions pass across 28 C:/D: runs.

261 actual transition assertions pass across seven workflows: resizable browser views at 96 and 144 DPI, a dark 192-DPI browser view, dark browser DPI cycling, import views at 96 and dark 192 DPI, and a fast browser read. Loading cases resize or change DPI while the first writer remains held. A second writer acquires Manager after capture and before publication; the rebuilt controls are inspected while it is still held.

Assertions check loading Cancel's placement, preservation of the user's outer window geometry, full-list/client anchoring without a further resize, immediate Cancel anchoring, actual list focus, light/dark list palette, forward/backward dialog navigation, stale loading command IDs, custom column-width preservation and all visible controls staying inside the client. Layout, theme, focus, DPI, navigation and paint return before the held writer releases. These navigation checks use native dialog messages within the fixture; they do not send physical keyboard input or switch desktops.

The opening/late-metadata suite passes 90 assertions, preserving its real delayed-HTTP checkbox behavior and held-lock cancel/retry checks. Batch admission passes 108 assertions; all ten selected authenticated files match their complete payload and make exactly one transfer request each. Main/Options passes 831 assertions, for 1290 current UI assertions total. All owned desktops close; personal browser/desktop control is not used. These targeted checks do not prove the entire backend suite, installed-browser behavior, real multi-monitor hardware transitions, absolute IDM timings or full IDM parity.

[Build](validation/batch-transition-9.35/build.json), [inputs](validation/batch-transition-9.35/dependency-inventory.json), [source changes](validation/batch-transition-9.35/source-change-audit.json), [versions](validation/batch-transition-9.35/product-versions.json), [backend](validation/batch-transition-9.35/backend-checks.json), [transitions](validation/batch-transition-9.35/batch-transition-checks.json), [opening and late metadata](validation/batch-transition-9.35/batch-opening-checks.json), [batch admission and complete files](validation/batch-transition-9.35/batch-admission-checks.json), and [Main/Options](validation/batch-transition-9.35/modal-checks.json).

## Preliminary check boundaries

The corrected preliminary transition run passes 261 assertions using 9.35 sources with 9.34 linked cores; it is excluded from current-build totals. Between the baseline test and corrected test, the harness normalizes the fast case to its requested DPI before the later column-width round trip, and its line endings change from LF to CRLF. Reversing that exact insertion and line-ending normalization reconstructs the recorded baseline test hash. The failing loading-layout assertion is unchanged, so that later fast-case setup change does not explain away the reproduced baseline failure.

[Corrected preliminary result](validation/batch-transition-9.35/excluded-corrected-transition.json), [test delta and reconstructed baseline hash](validation/batch-transition-9.35/test-harness-delta.json) and [linked preliminary cores](validation/batch-transition-9.35/excluded-preflight-linked-cores.json).

## Cleanup and next work

Removed 36 owned fixture/compiler roots and 38 temporary build files (370.1 MiB logical bytes) after retaining results and hashes. Current products/required cores, qualified 9.34 products, installed 8.68 / extension files 0.72.32, both personal catalogs and shared media tools remain intact; no media helper is copied. [Cleanup](validation/batch-transition-9.35/cleanup.json) and [preservation](validation/batch-transition-9.35/final-preservation.json).

No installer, installation, commit, push or release is included. Large-batch projection and list materialization, request/header/credential editing work, actual multi-monitor/minimize/owner-activation behavior and the wider Main/modal/shutdown backlog remain to qualify. Next address substantial row/request preparation and large-list first paint while preserving selected identities, row edits and browser context. Then continue installed browser capture, complete media files and live IDM comparisons. Full parity remains unproven. Future extension increments must roll 0.72.99 to 0.73.0 to keep every version part within two digits.
