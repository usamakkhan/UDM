# ZIP completion notifications — UDM 8.99

UDM 8.99 / source extension manifests 0.72.63 wakes ZIP windows when remote reads, local directory parsing or completed-prefetch readiness settle. Results no longer depend on the next 250 ms UI polling pulse to become visible. The periodic pulse remains as a fallback; this is not a zero-latency or live IDM timing claim.

## Publication and lifetime

`ZipPreviewTask` and `LocalZipPreviewTask` accept an optional completion function. A worker publishes the immutable result under its result mutex, releases the mutex and its I/O reader slot, then invokes the function. Success, cancellation and read errors follow the same publication path. Immediate constructor errors also notify; the UI performs one initial snapshot so it can display an already-terminal result immediately.

Completion functions are wake-ups only. They own their state, may run before a terminal constructor returns and must not capture controls or an owning task. Exceptions from notification are contained and cannot undo or replay the published result. Existing callers remain source-compatible through default arguments; affected native callers are rebuilt.

`PrefetchedZipReadTask` publishes its future value or exception before notifying. Its preparation owner passes the same weak window notifier to either this reader or the remote ZIP task, while retaining its separate preparation-ready notification. A completed-prefetch result can therefore wake the child, start local parsing and receive another wake when that parsing completes.

All production UI callbacks use `Form::taskNotifier`: an owned integer identity posted through a weak `UiTaskSignal`, disarmed before window destruction. No new callback retains a Form, HWND control wrapper, preparation-task pointer or Manager. Existing browser-session persistence leases and bounded nonjoining archive workers keep their prior lifetime rules. Joined Manager preparation and readiness ownership remain as in 8.97.

The existing preparation-notification test now owns its counter on the heap because HTTP completion can outlive preparation. It requires both preparation and actual HTTP completion notifications.

## Window behavior

Local ZIP windows, ordinary remote ZIP previews and prefetched-file parsing install their own notifier before starting the worker. They also consume an initial snapshot once their pulse is installed. A completion message consumes state; it does not schedule another read or create a timer/message loop. List virtualization, selected-request validation, pending-close ownership and guarded prefetch restoration remain unchanged.

## Qualification

All five native products report **8.99**, and both source manifests report **0.72.63**. Freeze 590 repository inputs and generated COM inputs. Recompile 1 affected core units and reuse the remaining 41 verified objects. [Build](validation/zip-completion-8.99/build.json), [dependency inventory](validation/zip-completion-8.99/dependency-inventory.json), [generated COM](validation/zip-completion-8.99/generated-com-inputs.json), [product versions](validation/zip-completion-8.99/product-versions.json).

Pass **3,204 backend assertions across 70 C:/D: runs**. The new 16-check suite waits for notification rather than repeatedly reading a result. It checks actual local and authenticated HTTP ZIP reads, complete 22-byte empty-archive listings, filesystem/authentication errors, and successful results despite throwing completion functions. Prefetched-file readiness adds an exactly-once completion assertion; the preparation suite requires both preparation and real HTTP wake-ups. A separate 16-check preflight is excluded from the primary count. [Backend assertions](validation/zip-completion-8.99/backend-checks.json), [preflight](validation/zip-completion-8.99/preflight.json).

Pass **657 actual UI assertions**: 312 Main/File Info, 257 pending admission and 88 progress menu. The new scene holds real HTTP response headers, removes the ZIP window's polling timer and queued timer messages, then releases the response. The real virtual list receives the complete 22-byte archive and displays OK through its owned completion message. Its ready-state hook records `taskPulseActive`, proving that the update came through the posted task identity rather than a timer. [Main UI](validation/zip-completion-8.99/ui-checks.json), [pending UI](validation/zip-completion-8.99/pending-ui-checks.json), [menu UI](validation/zip-completion-8.99/progress-menu-ui-checks.json).

The UI fixture uses an owned isolated desktop, does not switch the personal desktop and injects no personal-session input. These loopback and component checks do not verify a live browser download or compare IDM's actual timing or visuals.

## Superseded 8.98

8.98 / extension 0.72.62 builds all native products and passes 3,204 backend assertions. Its Main UI run fails an existing first-paint assertion when the test sees a child HWND during modal initialization, before the window is visible. Keep the held-writer/deadline assertions, wait for actual visibility before observing first paint, add the same visibility precondition to the new held-response case, record the paint state and advance both versions. No production callback behavior is weakened. Preserve the failed UI and successful backend evidence separately from current qualification. [Superseded UI](validation/zip-completion-8.99/superseded-ui-checks.json), [superseded backend](validation/zip-completion-8.99/superseded-backend-checks.json).

## Cleanup and delivery

Remove **148 owned fixture directories and 176 temporary executable/object/resource files**, including six displaced core objects no longer linked by the current build. Preserve all current products and required core objects, the previous 8.97 native products, installed files, both personal catalogs and installed media-helper hashes. [Cleanup](validation/zip-completion-8.99/cleanup.json), [cache pruning](validation/zip-completion-8.99/pruned-cache.json), [preservation](validation/zip-completion-8.99/final-preservation.json).

The pending fixture reuses the existing 8.83 alias to installed ffmpeg/ffprobe without copies or a new alias. Automatic approval review previously rejected removal of that alias as “blocked by policy”; no retry or bypass is attempted.

Installed UDM remains 8.68 / extension files 0.72.32. Browser JavaScript is unchanged. No installer, installation, commit, push or release is included.

## Remaining work

The prefetched-file reader still checks receipt availability using its cancellable 10 ms wait. Message delivery and painting still depend on Windows scheduling. Caller offer restoration/admission, address refresh, category/scheduling and final prefetch teardown also retain synchronous work. Continue those ownership changes, then compare cold/warm dialog readiness, exact pre-Start size, complete downloads and all finishing/background work with IDM. Full inside/out parity remains unproven.
