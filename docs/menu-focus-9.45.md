# Menu keyboard ownership and Exit diagnosis — UDM 9.45

Keep a pending download menu from consuming a child dialog's Escape key. Retire pending menu requests when another command or a valid row activation supersedes them. Build five native 9.45 products; both source extension manifests advance to 0.73.9. All 30 focused backend and 509 current native UI assertions pass. Installed 8.68 remains unchanged.

## Confirmed modal-keyboard bug

The new fixture opens a real modal Form while a context-menu reader is waiting for a separate catalog writer. During the dialog's initialization, the main window is disabled and the key targets the dialog's edit control. The fixture passes Escape through the main window's existing translation hook and, if unhandled, through the dialog's own translation hook. A fallback posted Cancel prevents the failing baseline from hanging; it is distinct from the dialog handling Escape.

Against frozen 9.44 App, Version and QueueMenus sources and the 9.44 core objects, the test fails at its tenth assertion. The main window consumes Escape, so the dialog never receives it. The modal closes through the fallback. The exact same test bytes against 9.45 show that the main window leaves Escape unhandled, the real dialog handles it and cancels, and the pending menu request is retired. The existing main-window Escape cancellation remains covered separately. No physical keyboard input or interactive desktop switch is injected.

[Failing 9.44 baseline](validation/menu-focus-9.45/preflight/baseline.json), [same-test before/after comparison](validation/menu-focus-9.45/modal-comparison.json), [current focus checks](validation/menu-focus-9.45/menu-focus-checks.json).

## Production change

For a pending menu, Escape first retires the request. The main window consumes that key only when it is enabled and the key's target is the main window or one of its native children. An owned modal dialog retains its key. Starting another main-window command or activating a valid table row cancels a pending menu before continuing that operation, preventing a late popup from interrupting the newer action.

The background reader, captured popup advice, stale-selection/queue checks and shutdown ownership from 9.44 are retained. Actual command invocation still performs its live validation. This increment does not change download scheduling, transfer data or final-file publication.

## Current acceptance checks

The 28-check focus component includes the earlier context-menu coverage plus the real modal Escape case, responsiveness while its writer remains held, absence of a late popup after modal close, and immediate supersession by another command. It retains Add/Move wording, synchronization-only completed-file queue destinations, native popup advice updates, late queue/policy rereads, removed-job rejection and direct window destruction with a reader pending.

The full legacy menu workflow passes 458 assertions, including selected Stop, deferred Stop All, scheduler non-restart and three deterministic 1 MiB transfers whose final SHA-256 values match the expected bytes. The main Close component passes 23 assertions across busy tray policy, late policy changes, Show cancellation and actual Exit. The menu-reader and close-policy backend suites pass 16 and 14 assertions respectively. These are fresh 9.45 runs; prior suites are not added to these totals.

[Backend checks](validation/menu-focus-9.45/backend-checks.json), [legacy workflow](validation/menu-focus-9.45/main-menu-checks.json), [main Close](validation/menu-focus-9.45/main-close-policy-checks.json), [current UI totals](validation/menu-focus-9.45/ui-summary.json).

## Separate controlled Exit observation

A diagnostic-only copy of the current App adds stage timestamps and surrounds the existing rememberLayout call with two markers. It uses the current release core objects and the real CMD_EXIT route. A separate writer deliberately owns Manager for 750 ms. No browser integration or actual download is started by this observation.

| Stage | Measured time |
| --- | ---: |
| Layout step | 761.9 ms |
| Manager stop | 55.7 ms |
| Exit command return | 866.4 ms |
| Exit plus fixture Manager/worker teardown | 866.8 ms |
| Additional fixture teardown after window close | 0.5 ms |

The layout function captures native column state, then takes the model lock and merges ListLayout in memory. The controlled contention lands in this step before integration or Manager shutdown begins. This establishes a window-thread lock wait in the fixture. The 750 ms hold is deliberate, not a naturally observed download delay. The diagnostic is excluded from the acceptance assertion totals; its successful execution does not mean that Exit meets a performance target.

These timings include the fixture's Manager/worker teardown. They exclude a kernel writeback trace and do not measure a real browser connector, installed IDM, network capture or a completed download's entire disk lifetime. The remaining Exit wait is diagnosed here, not fixed by the modal-keyboard change.

[Exit result and stage events](validation/menu-focus-9.45/exit-observation/result.json), [phase summary and scope](validation/menu-focus-9.45/exit-observation/phases.json), [instrumentation recipe](validation/menu-focus-9.45/exit-observation/instrumentation.json), [diagnostic-only patch](validation/menu-focus-9.45/exit-observation/instrumentation.patch), [fixture source](validation/menu-focus-9.45/exit-observation/fixture.cpp).

## Build and cleanup

The build freezes 640 repository inputs and generated COM hashes before compilation. Dependency analysis rebuilds Bridge for the version header and verifies reused core objects. All five native products report file and product version 9.45. The source changes against qualified 9.44 are App.cpp, the new focus fixture, Version.hpp and the two extension version fields. Test-only UI presenters and Exit instrumentation are excluded from the normal products.

Removed 13 owned fixture/compiler roots and 107 temporary files (448.9 MiB logical bytes) after exporting sanitized evidence. Current products and required core caches, previous 9.44 products, installed 8.68 / extension files 0.72.32, both personal catalogs and shared media helpers retain their verified hashes. No helper is copied. No installer, installation, commit, push or release is included.

[Build](validation/menu-focus-9.45/build.json), [dependency inventory](validation/menu-focus-9.45/dependency-inventory.json), [source changes](validation/menu-focus-9.45/source-change-audit.json), [binary versions](validation/menu-focus-9.45/product-versions.json), [cleanup](validation/menu-focus-9.45/cleanup.json), [preservation](validation/menu-focus-9.45/final-preservation.json).

## Next implementation

Capture the UI-owned layout once, then move its model acquisition and in-memory merge to an owned Exit preparation stage. Preserve other settings written concurrently and let Manager's existing final checkpoint persist the merged layout. Keep the window responsive while that stage waits; avoid repeating the blocking rememberLayout call when shutdown continues. Retain Manager and notifier lifetimes through success, cancellation and error cleanup.

Test actual CMD_EXIT while a catalog writer is held: the command handler must return before that writer releases, then shutdown must finish exactly once with the latest layout and other settings preserved. Measure the remaining Manager, integration and window teardown independently; moving a wait off the UI thread alone does not prove a lower total time. Installed popup interaction, modal transitions and IDM comparisons including background work remain required. The original inside/out parity goal stays open.
