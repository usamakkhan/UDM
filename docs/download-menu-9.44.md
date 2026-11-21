# Background download-menu preparation — UDM 9.44

Move download-context-menu preparation off the window thread, retain its existing groups and queue choices, and carry the captured command advice through native popup updates. Discard obsolete requests and finish the reader before window destruction. Build five native 9.44 products; source extension manifests advance to 0.73.8. All 84 focused backend and 480 current native UI assertions pass. Installed 8.68 remains unchanged.

## Measured cause and result

The 9.43 download-menu path called selected with deferred validation, then blocking command-state, queue and preferences readers. A generated baseline retained that path and added only a test presentation hook to avoid the interactive popup loop. Its frozen QueueMenus and Version copies match 9.43, and it uses the 9.43 core objects. A separate writer held Manager with a two-second watchdog. The request returned after 2,016 ms, after that watchdog had expired; its third assertion failed.

The current request returns in 0 ms at GetTickCount64 resolution with the same writer still held and its watchdog unexpired. The test then processes a main-window heartbeat before releasing the writer. The popup is constructed only after its complete model cohort becomes available. This measures request-handler return, not instantaneous popup visibility during a blocked model read. It is a bounded local contention comparison, not an installed IDM timing comparison.

[Baseline and current measurements](validation/download-menu-9.44/contention-comparison.json), [retained failing baseline](validation/download-menu-9.44/preflight/baseline.json), [baseline scope](validation/download-menu-9.44/preflight/scope.json).

## Ownership and native menu behavior

DownloadMenuTask owns one joinable reader. It tries the model lock on its worker and waits cancellably when another writer or accepted File Info decision owns the model. The UI captures only its current selected Job pointers and request position; it does not wait to validate deferred rows. The reader verifies job ownership and captures command decisions, queue names/labels, membership and the completed-file double-click policy under the model lock. No native controls or UI-owned AsyncFileStatus state are touched by that reader. Existing address-policy/decryption work now runs there as part of preparation.

The result wakes the existing window task signal, with the main timer as a retry. Before display, the owner rejects changed selections, removed jobs, hidden/disabled windows and loss of activation after an active-window request. A nonblocking final read detects queue, membership and double-click-policy changes and rereads an obsolete cohort. Repeated requests cancel the old generation and publish only the newest one. Escape cancels a pending request.

Menu rendering consumes the owned view after releasing Manager. Queue labels, literal ampersands, synchronization-only destinations for completed files, Add/Move wording and the existing menu grouping are retained. Regular-file advice continues to use the UI's background status cache. Windows' WM_INITMENUPOPUP command updates use the same captured advice during the popup rather than repeating model/decryption reads. The scoped advice owner is independent of MainWindow's lifetime and restores the previous advice after the popup returns. Actual command invocation retains the existing live validation path.

Normal Close cancels the pending request. Final Exit and the MainWindow destructor join the menu reader before its callback dependencies are destroyed. No detached reader retains a window pointer. This does not move the rest of Exit's layout persistence, integration shutdown or Manager worker joins off the window thread.

## Current validation

The four backend suites pass 84 assertions: 16 menu-reader checks, 22 selection-snapshot checks, 32 history-snapshot checks and 14 close-policy checks. They cover immutable values, a held model writer, duplicate starts, cancellation/join without acquiring the writer's lock, accepted-decision deferral, failure/retry, notification after releasing Manager and unchanged catalog contents. The new suite is registered in native/backend-tests.json.

The new native menu component passes 22 assertions. It exercises real menu construction and WM_INITMENUPOPUP routing, held-writer responsiveness, Add/Move choices, synchronization-only completed-file destinations, selection changes, Escape, repeated requests, late queue/policy edits, removed jobs, advice cleanup and direct window destruction with a reader pending. A test-only presenter intercepts TrackPopupMenu to avoid physical input; the interactive popup loop itself is not exercised by this component.

The full legacy menu workflow passes all 458 assertions with the permanent 9.43 Resume acknowledgment retained. Its synchronous menu-layout assertions explicitly prepare a view; the new component exercises the asynchronous interactive opening path. The legacy workflow also covers selected Stop, deferred Stop All, scheduler non-restart and three deterministic 1 MiB downloads that complete with exact expected SHA-256 values. No prior Main/Options, browser or installed-IDM result is counted as a new 9.44 run.

[Backend checks](validation/download-menu-9.44/backend-checks.json), [new menu checks](validation/download-menu-9.44/download-menu-checks.json), [full legacy workflow](validation/download-menu-9.44/main-menu-checks.json), [current UI totals](validation/download-menu-9.44/ui-summary.json).

## Build, qualification and cleanup

The dependency inventory contains 639 repository inputs. All five native products report file and product version 9.44. Bridge rebuilds for the version header. Four products unaffected by the later App-only amendment retain their verified hashes; App and the corrected new backend fixture are rebuilt against the final frozen inputs. Current core hashes and every declared UI input match that final cohort.

The first compilation rejected a mixed enum/UINT initializer list; an explicit UINT array corrects it. The first backend fixture read state.json before saving its fresh catalog and failed before any assertion; saving that fixture corrects the setup. A preliminary 16-check UI run passes but is superseded and excluded from the current total. Three owned compiler processes were deliberately stopped to add the destructor join and direct-destruction regression, then rebuilt; this was a source amendment, not an observation timeout. These initial inputs, failures and canceled-build records are retained separately.

[Final build](validation/download-menu-9.44/build.json), [initial dependency inventory](validation/download-menu-9.44/dependency-inventory.json), [final amendment inputs](validation/download-menu-9.44/qualification-history/amended-source-inputs.json), [source changes](validation/download-menu-9.44/source-change-audit.json), [binary versions](validation/download-menu-9.44/product-versions.json), [initial fixture failure](validation/download-menu-9.44/qualification-history/unsaved-fixture-results.json), [deliberate compiler cancellation](validation/download-menu-9.44/qualification-history/cancelled-build-reason.json).

Removed 13 owned fixture/compiler roots and 96 temporary files (319.9 MiB logical bytes) after exporting sanitized evidence. Current products and required core caches, previous 9.43 products, installed 8.68 / extension files 0.72.32, both personal catalogs and shared media helpers retain their verified hashes. No media helper is copied. No installer, installation, commit, push or release is included. [Cleanup](validation/download-menu-9.44/cleanup.json), [preservation](validation/download-menu-9.44/final-preservation.json).

## Remaining work

Measure the real popup's appearance and interaction on the installed app against IDM, including idle and busy cases, large selections, filesystem latency and modal/keyboard transitions. The owned tests establish handler responsiveness and state ownership; they do not prove universal latency or visual parity. Continue separating actual Exit's blocking stages while preserving durable catalog completion and worker lifetimes. Installed browser capture and final-background-work comparisons also remain required. The original inside/out parity goal stays open.
