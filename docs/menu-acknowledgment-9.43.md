# Menu workflow acceptance — UDM 9.43

Correct the synchronous legacy menu fixture to acknowledge its completed Resume through the production window poll before rendering a new batch. Require the intended active/completed native selection before Stop. The full menu workflow passes 458 checks, including selected Stop, deferred Stop All, scheduler non-restart and three byte-exact resumed files. Five native products are built as 9.43; both source extension manifests advance to 0.73.7. This is a validation/maintenance increment. Download-engine behavior is unchanged, and installed 8.68 remains unchanged.

## What the failed check actually measured

The retained 9.42 menu run failed after 435 assertions at “Selected Stop pauses transfer and closes only its progress window.” A generated diagnostic recorded a completed Resume task that was active, ready and still holding the file-info decision flag. Its subsequent refresh returned before rendering the newly created batch. The selected row was the previously completed download; the active batch item was absent from both the displayed and selected IDs. Stop correctly left that unrelated active item running.

This fixture executes synchronously inside InitInstance, before the normal Windows message loop starts. Waiting for transfer bytes or calling Manager::tick does not acknowledge the window's asynchronous Resume result. In production, OnTimer and OnModelMaintenanceReady call pollResumeCommands to take that result and release its ownership. The test had omitted that step.

A generated follow-up using the same 9.42 App source, qualified repository inputs and core objects passed all 457 checks after adding a bounded call to the real window poll. Production Stop, Resume and progress-window behavior were unchanged. The original failure remains a failed diagnostic and is excluded from passing totals.

[Failure](validation/menu-acknowledgment-9.43/preflight/unacknowledged.json), [captured state](validation/menu-acknowledgment-9.43/preflight/batch-stop-state.json), [corrected-sequence diagnostic](validation/menu-acknowledgment-9.43/preflight/acknowledged-diagnostic.json), [same-product scope](validation/menu-acknowledgment-9.43/preflight/baseline-scope.json).

## Permanent fixture and current result

The permanent test waits up to five seconds while invoking MainWindow::pollResumeCommands, then requires both the Resume task and the decision flag to be released. It records their pre-acknowledgment state. Before sending Stop through WM_COMMAND it additionally requires exactly the intended active and completed jobs to be selected in the rendered native table. The original stopping, window, scheduler and final-file assertions remain.

All 458 assertions pass on an isolated owned Windows desktop. Selected Stop pauses only its chosen active job and closes only that progress window. The other receiving transfer and unrelated queued transfer retain their windows. The selected completed record remains unchanged. Stop All defers closing a disabled progress window until its child releases input, closes the other windows, leaves all three jobs paused, and scheduler ticks do not restart them. Explicitly resuming all three completes each deterministic 1 MiB file with the exact expected SHA-256. The fixture process exits normally after its owned background work is joined. No interactive desktop switch or physical input is injected.

These checks use actual native menus and progress windows with bounded local HTTP transfers. They do not establish installed IDM timing, external-site capture or universal latency parity. Existing 9.42 backend/Main/Options/close results remain historical evidence; those suites are not newly rerun or counted for 9.43.

[Full current menu checks](validation/menu-acknowledgment-9.43/main-menu-checks.json), [Resume checkpoint](validation/menu-acknowledgment-9.43/menu-checkpoints/resume-acknowledgment.json).

## Build and preservation

All five native products report file and product version 9.43. Both source extension manifests are 0.73.7. The build freezes 636 repository inputs and generated COM hashes before compilation. Against the qualified 9.42 source, only the menu fixture, Version.hpp and the two extension version fields change. Bridge rebuilds because it consumes the version header; other unchanged core objects are hash-verified and reused. No new engine behavior or extension feature is implied by these version increments. No backend suites are scheduled by this product-build controller.

[Build](validation/menu-acknowledgment-9.43/build.json), [dependency inventory](validation/menu-acknowledgment-9.43/dependency-inventory.json), [qualified-source changes](validation/menu-acknowledgment-9.43/source-change-audit.json), [binary versions](validation/menu-acknowledgment-9.43/product-versions.json).

Removed five owned fixture/compiler roots and 144 temporary files (305.9 MiB logical bytes) after exporting sanitized evidence. Current products and required core caches, previous 9.42 products, installed 8.68 / extension files 0.72.32, both personal catalogs and shared media helpers retain their verified hashes. No helper is copied. No installer, installation, commit, push or release is included. [Cleanup](validation/menu-acknowledgment-9.43/cleanup.json), [preservation](validation/menu-acknowledgment-9.43/final-preservation.json).

## Next implementation work

The context-menu path still has blocking reads, independently of this fixture correction. downloadMenu/fillDownloadMenu call selected with deferred validation, commandStates without its nonblocking busy output, queue label/membership reads, and preferences. Reproduce this with a held catalog writer and a UI heartbeat, including a deferred history view. Then prepare one owned immutable selection/queue/preferences cohort off the UI thread, preserve the captured IDM grouping and labels, and invalidate/retry stale results before displaying actions. Keep invocation checks against current jobs/queues and filesystem work outside the menu event handler. Cover selection changes, queue deletion/rename, canceled preparation and window destruction before accepting a result. Reading mutable Job data without ownership or silently removing menu choices would not resolve this gap.

Actual Exit still performs layout persistence and worker joins on the window path. Trace each stage separately, preserve Manager and HWND/notifier lifetimes until all dependent workers have stopped, and exercise failure/retry as well as success. Detached workers are not a valid completion fix. Installed browser workflows and IDM comparisons including background work remain required validation. The original inside/out parity goal stays open.
