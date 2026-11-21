# Main font ownership and reuse — 9.55 / extension 0.73.19

Reuse the main font and skip redundant native font-change messages when its face, pixel height and weight are unchanged. Repair detached control bindings, prepare replacements before releasing the old font, and retain the previous valid font on creation failure. Build all five native 9.55 products; pass 187 backend and 910 optimized UI assertions. Source browser manifests advance to 0.73.19. Installed 8.68 and extension files 0.72.32 remain unchanged; installed IDM parity and the prior total-time gate remain open.

## Rendering change

The previous full appearance path deleted and recreated the main font on every apply, then unconditionally assigned it to eleven controls and reassigned the toolbar font during rebuilding. A native list control also forwards its font message to its header, so the frozen 9.54 fixture observes thirteen font-change messages on an unchanged apply.

MainWindow now owns a key containing the exact effective face name, pixel height and weight. The fixed CreateFont attributes remain unchanged. The face is normalized to the native LF_FACESIZE limit. An unchanged key and valid native font reuse the current object. There is no multi-font cache or retained historical font set.

Each of twelve controls, including the header and toolbar, is inspected through WM_GETFONT. Only mismatched bindings receive WM_SETFONT. This repairs lost bindings even when the font itself is reused. The toolbar rebuild makes the same binding check instead of assigning the font again unconditionally. Layout, palette, captions and toolbar rebuilding otherwise retain their existing behavior.

For a changed key or missing owned handle, construct the replacement first and verify that Windows returned a font. Only then attach it as the current font and bind every control. Keep the preceding font alive until those bindings finish. A failed creation leaves the old owned font, key and control bindings intact. The appearance snapshot scope is still restored on failure; the last successfully rendered settings are not replaced. This is a font-lifetime guarantee, not a claim that all preceding theme work is rolled back atomically.

## Qualification and limits

Eighteen new optimized assertions observe actual Windows messages and handles. They cover unchanged appearance, palette/toolbar/pane changes, all twelve missing-binding repairs, requested face/weight/height changes, DPI changes and same-DPI handling, DPI roundtrip, an injected null native font factory result, retry, hot applies without factory invocation, repeated replacement GDI counts, missing owned handle recreation, and unchanged catalog data. Native handle values may be recycled, so identity checks alone do not establish the skipped work; the message observations and factory observers supply the additional evidence.

The frozen 9.54 build fails the same first assertion, recording thirteen redundant font messages. All new test work runs on a child-attested separate desktop without switching the interactive desktop or injecting physical input. The baseline fixture's common measured prefix is identical; its current-only macro-guarded assertions differ after a type-qualification correction, removal of a fragile deleted-handle assertion and the explicit steady-state resource check. The initial wrapper stopped after successful compilation because that canonical fixture changed; this is retained as a wrapper failure, not a product failure. The frozen compiled fixture was then hash-checked and executed normally.

All 187 backend checks pass across nine suites. All 910 optimized UI checks pass: 18 main-font checks, 36 appearance checks, 25 toolbar-cache checks and 831 modal workflow checks. Five native products have file version 9.55. Final UI inputs match the 664-file qualification inventory. It differs from the original build inventory only in MainFontUiTests.cpp: the resource test now warms the same metrics and then requires no growth across two more batches. Production inputs are unchanged, and the original failed six-object-growth diagnostic is retained separately. Existing core objects are hash-verified, with Bridge rebuilt for the version change.

The single same-appearance observation is 21.5404 ms on frozen 9.54 and 9.3660 ms on 9.55. These are noisy fixture observations, not a benchmark distribution or an installed IDM comparison. The established result is eliminating thirteen redundant font messages and avoiding creation on the covered unchanged path.

## Remaining work

Synchronous preference mutations for fonts, theme, tray and toolbar commands still need shared-model ownership work. This increment addresses rendering resource churn, not those waits. Next, introduce an accepted appearance-command task that owns a narrow field draft, merges into current settings under the model lock on a worker, and returns an owned acknowledged result for rendering without a second model read. Toggles must use the current committed value at execution. Keep conflicting commands gated through acknowledgement, defer Close/Exit until accepted saves finish, and preserve explicit failure/retry and shutdown joins. Startup, category/tray image work, installed browser capture behavior, live-network limiter accuracy, completion/background-work comparisons and full IDM inside/outside parity remain open. The earlier total-time gate is still unproven.

The source extension versions advance to 0.73.19 without a capture behavior change. No installer, installation, commit, push or release was performed. Installed 8.68 and extension files 0.72.32, both personal download catalogs and shared media helpers are preserved. Temporary test/compiler outputs are removed after sanitized evidence export, preserving current and preceding products and required core objects.

[Validation status](validation/font-reuse-9.55/status.json) includes source and binary hashes, native assertions, baseline reconstruction, compiler logs, cleanup and preservation records.
