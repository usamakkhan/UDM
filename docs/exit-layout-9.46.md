# Responsive Exit layout preparation — UDM 9.46

Move a contended Exit layout merge off the window thread, preserve the fast path when Manager is available, and continue the existing shutdown once the merge is accepted. Build five native 9.46 products; both source extension manifests advance to 0.73.10. All 49 focused backend and 1,360 current native UI assertions pass. The matched total-time performance gate remains open: three controlled pairs show a 21.1 ms median regression despite much faster command return. Installed 8.68 remains unchanged.

## Confirmed window-thread wait

The same new Exit fixture against frozen 9.45 App, Version, MainFeatures and QueueMenus copies fails at its first assertion. A separate writer holds Manager with a two-second watchdog. The actual CMD_EXIT call returns after 2,094.5 ms, after the writer has expired and the window has been destroyed. It cannot process a heartbeat during that wait.

With 9.46, the busy Exit case returns after 11.9 ms while the writer is still held. The window remains alive, processes duplicate Exit/Close/Show, timer and paint messages, and waits for its one owned layout stage. The same test source runs before and after; later assertions cover the new asynchronous lifecycle.

[Failing baseline](validation/exit-layout-9.46/preflight/baseline.json), [current real Exit checks and timings](validation/exit-layout-9.46/exit-layout-checks.json).

## Production behavior

The UI captures its native column layout before attempting the model lock. If Manager is immediately available and no accepted File Info decision owns it, the existing in-memory layout merge proceeds without creating a worker. When acquisition would block, MainExitLayoutTask owns the captured value and performs cancellable acquisition on its worker. It merges only Settings.ListLayout, preserving other settings written concurrently. It adds no intermediate catalog save; Manager's existing final checkpoint remains responsible for persistence.

While that stage waits, the main window disables new input, coalesces repeated Exit/Close, ignores Show requests, and services the stage through the existing task signal and timer. A successful result continues shutdown once and skips the former blocking rememberLayout call. A pre-merge failure restores input and leaves the prior layout and disk catalog available for retry. Retry captures the newest native layout. The worker distinguishes an unaccepted cancellation from an already applied merge.

Direct destruction cancels and joins a waiting stage before its dependencies die. A normal close-to-tray retains its previous policy behavior and does not begin this Exit stage. The rest of shutdown, including integration stops and Manager teardown, is retained; this change does not claim that all Exit work is asynchronous.

## Acceptance and fixture ownership

The new backend suite passes 19 assertions covering acquisition under a held writer, duplicate starts, unaccepted cancellation, joining without acquiring the writer's lock, accepted File Info ownership, failure/retry, notification after unlocking, cancellation after acceptance, invalid input, unchanged disk contents during preparation and persistence through the canonical save. Existing close-policy and menu-reader suites pass 14 and 16 assertions, for 49 current backend assertions.

The new native Exit suite passes 20 assertions across busy Exit, failure/retry and direct destruction. It checks responsiveness while the writer is held, one shutdown owner, no early save, exact-once integration stop, the captured layout and concurrent setting in the final catalog, and preservation after an unaccepted cancellation. Existing native components also pass: 23 main Close checks, 458 menu-workflow checks, 28 menu/focus checks and 831 Main/Options/modal checks, for 1,360 current UI assertions. The legacy menu workflow retains three deterministic 1 MiB downloads with exact expected final SHA-256 values.

InitInstance-based tests must now retain their Manager until asynchronous Exit has finished. A test-only closeOwnedMainWindow helper dispatches the normal Exit and pumps owned window messages until destruction. Eighteen existing fixture edits change only cleanup calls and necessary text encoding/newline normalization. Reversing those calls and restoring each original encoding reproduces every qualified 9.45 source hash; no assertion was removed or weakened. The helper is excluded from normal products. Owned desktops close without switching the interactive desktop or sending physical input.

[Backend checks](validation/exit-layout-9.46/backend-checks.json), [Exit lifecycle](validation/exit-layout-9.46/exit-layout-checks.json), [Close policy](validation/exit-layout-9.46/main-close-policy-checks.json), [legacy menus](validation/exit-layout-9.46/main-menu-checks.json), [focus](validation/exit-layout-9.46/menu-focus-checks.json), [Main/Options](validation/exit-layout-9.46/modal-checks.json), [UI totals](validation/exit-layout-9.46/ui-summary.json), [cleanup-only fixture audit](validation/exit-layout-9.46/fixture-cleanup-audit.json).

## Matched total-time gate is not passed

The first current-version observer reports 922.4 ms through fixture Manager/worker teardown, compared with the historical 9.45 observation's 866.8 ms. Inspection finds Sleep(1) after every dispatch, including after window destruction. That observer adds waiting outside the app. Its source, raw result and scope are retained, with no sample relabeled or discarded.

Two observers were then built from frozen 9.45 and current 9.46 App/core inputs using identical observer-source bytes. Both use an event-driven message wait and the same deliberate 750 ms catalog hold. Three alternating baseline/current pairs retain every run and whole-process wall time. Their medians are:

| Measurement | 9.45 | 9.46 |
| --- | ---: | ---: |
| Exit command return | 842.8 ms | 14.2 ms |
| Window close | 842.8 ms | 863.8 ms |
| Fixture Manager/worker teardown complete | 843.2 ms | 864.3 ms |

Command responsiveness improves, but the matched median total is 21.1 ms slower. Current total samples span 846.3–866.2 ms; baseline samples span 835.2–843.9 ms. The local no-slower total-time gate therefore remains false. Functional tests passing does not override it.

Most of the difference occurs before layout readiness: baseline readiness is 753.3–758.3 ms from entry, while current readiness is 774.0–781.5 ms. Later teardown durations are similar and vary between runs. These traces do not yet record the precise writer-unlock boundary, so they locate the delay but do not prove whether retry wakeup, scheduling or writer timing accounts for all of it. Timer or priority tuning is not justified by this evidence.

This is a small controlled local comparison, not installed IDM timing or naturally occurring transfer finishing. The observer includes fixture Manager/worker teardown and records whole-process wall time separately. Browser/COM integration is not started, and kernel disk writeback is not traced. The build is not qualified as a universal speed/parity result.

[Performance gates](validation/exit-layout-9.46/performance-gates.json), [all six matched runs](validation/exit-layout-9.46/matched-comparison/runs.json), [summary and ranges](validation/exit-layout-9.46/matched-comparison/summary.json), [phase analysis](validation/exit-layout-9.46/matched-comparison/phase-analysis.json), [identical observer source](validation/exit-layout-9.46/matched-comparison/fixture.cpp), [first observer retained](validation/exit-layout-9.46/initial-observer/result.json), [first observer scope](validation/exit-layout-9.46/initial-observer/scope.json).

## Build and preservation

The build freezes 643 repository inputs and generated COM hashes before compilation. Dependency analysis rebuilds Bridge for the version header and verifies reused core objects. All five native products report file and product version 9.46. The new task suite is registered in native/backend-tests.json. Test-only observers and presentation hooks are excluded from normal products.

Removed 21 owned fixture/compiler roots and 204 temporary files (756.8 MiB logical bytes) after exporting sanitized evidence. Current products and required core caches, previous 9.45 products, installed 8.68 / extension files 0.72.32, both personal catalogs and shared media helpers retain their verified hashes. No helper is copied. No installer, installation, commit, push or release is included.

[Build](validation/exit-layout-9.46/build.json), [dependency inventory](validation/exit-layout-9.46/dependency-inventory.json), [source changes](validation/exit-layout-9.46/source-change-audit.json), [binary versions](validation/exit-layout-9.46/product-versions.json), [cleanup](validation/exit-layout-9.46/cleanup.json), [preservation](validation/exit-layout-9.46/final-preservation.json).

## Next implementation and verification

1. Record the actual writer-release boundary, reader acquisition attempts, wake and merge, using a thread-safe observer with no dispatch sleeps. Separate writer hold duration from the additional delay before acceptance. Preserve every sample and compare phases under the same stimulus.
2. Inspect existing Manager availability notifications before adding another mechanism. If release-driven wakeup is needed, make subscription and cancellation lifetime-safe, cover all relevant writer/recursive-lock paths, and prevent lost wakeups around registration and unlock. Accepted File Info ownership must also wake a deferred reader when it retires. Avoid moving the wait into a destructor or adding overhead to every ordinary transfer lock without evidence.
3. Cover release before/after subscription, cancellation before/after acceptance, direct destruction, concurrent preferences, duplicate requests and failed final shutdown/retry. Retain the canonical checkpoint and its latest download state; do not add an early layout-only save.
4. Repeat the matched event-driven comparison after the fix, including window close, Manager teardown and remaining background ownership. Keep the total-time gate open unless the results support closing it. Installed browser, popup and IDM comparisons still remain separate requirements.

The original inside/out parity goal remains active. This increment fixes the measured window-thread layout wait; it does not finish the remaining total-time work.
