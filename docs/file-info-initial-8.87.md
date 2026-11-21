# Initial File Info preparation — UDM 8.87

UDM 8.87 / extension manifests 0.72.51 carries 8.86's owned initial preparation and makes advanced navigation usable during background admission. Once `downloadInfo` is entered, an ordinary or protected-form download paints one File Info window before Manager can be read. Basic fields and Start remain disabled until a coherent view is ready. The same window then receives the actual address, destination, categories, size and credentials; it is not replaced by a second dialog. Prepared pending-media presentations keep their existing early path.

## State and ownership

`FileInfoInitialTask` owns a cancellable model-lock wait, checks history membership and captures immutable render inputs. Address decoding and remembered login resolution use owned inputs after the model lock is released. The task changes no job, receipt or transfer state. Render-cache reuse is compared on that reader; a stale size or preference snapshot is refreshed before editing begins. The private address/credential result is not serialized into diagnostics or browser messages.

Queue names and display labels are captured with the initial view. More can build those fields without reading Manager or waiting for admission. Edit-category and scheduling mutation controls retain their admission guard; actual decisions still validate current backend state. For prepared media, More becomes available only after accepted binding. This change does not turn a rendering snapshot into acceptance authority.

Form performs the bootstrap-to-controls transition **after the current pulse returns**, avoiding destruction of the callback currently executing. The normal mutable pulse is not copied, so its first-admission and completion state persists. Old control IDs are retired monotonically; a queued command for a disabled bootstrap button cannot target a newly created Start control. Focus is restored inside the form only when it belonged there, and the transition does not request foreground activation again.

Cancel uses the existing owned decision command: join/cancel initial preparation, join any prefetch admission, and run the established pause/fresh-offer cancellation helper. The window remains responsive while that command waits for durable state. It closes after the command completes, preserving ordinary/form history as paused and removing a fresh awaiting-confirmation offer. Normal and exceptional teardown join the reader before its Manager can be destroyed. Initial-read failures expose Retry and Cancel without enabling a partial draft.

## Qualification

All five native products carry 8.87. Freeze 583 repository inputs, resources and linked local libraries plus generated COM inputs before compilation. BOM-aware quoted-include traversal reports no unresolved includes. Bridge recompiles for 8.87; 41 core hashes are reused and requalified by the current runs, including CaptureAdmission from the superseded 8.86 native build. [Build](validation/file-info-initial-8.87/build.json), [dependency inventory](validation/file-info-initial-8.87/dependency-inventory.json), [generated inputs](validation/file-info-initial-8.87/generated-com-inputs.json).

Pass **2,984 backend assertions across 62 C:/D: runs**. The new initial-reader fixture contributes 20 assertions per drive: exact metadata and queue labels, private basic credentials, cache refresh, nonmutation, cancellation during a real model hold, an outstanding decision, a real protected POST request, removed membership and explicit Retry after a consumed read error. [Primary backend assertions](validation/file-info-initial-8.87/backend-checks.json).

Pass **617 actual UI assertions**: 272 Main/File Info/network/login/progress, 257 pending-admission/retry/cancellation and 88 progress-menu assertions. New ordinary/form cases first hold Manager before entering File Info, proving first paint, disabled Start, responsive timer/paint and shutdown protection. After capture, a different writer holds Manager while the same window fills, displays its fixture's known exact byte count and expands advanced fields. Stale bootstrap commands do not start a transfer. Cold fresh-offer Cancel and cancellation after readiness preserve their established outcomes. Existing real HTTP prefetch/reuse and queued decision cases still pass. These are component fixtures, including intentionally held model stages and known fixture metadata, not a live browser final-file-size or IDM timing comparison. [Main UI](validation/file-info-initial-8.87/ui-checks.json), [pending UI](validation/file-info-initial-8.87/pending-ui-checks.json), [menu UI](validation/file-info-initial-8.87/progress-menu-ui-checks.json).

All three UI fixtures create and close separate owned desktops, never switch desktops and inject no personal-session input.

## Superseded 8.86

The first 20-check preflight passed, but the UI compiler rejected an ambiguous MFC `GetNextDlgTabItem(nullptr, FALSE)` call. Use the explicit Win32 HWND API, add exact-byte and stale-command cases, freeze new inputs and rebuild. That 8.86 native build passed 2,984 assertions, while its Main UI run failed: the readiness test waited for More to become enabled, but admission deliberately kept it disabled until the held writer timed out. No 8.86 UI qualification is claimed.

With owned queue inputs, advanced navigation no longer needs that restriction. Advance both versions to 8.87 / 0.72.51, keep mutation guards, freeze inputs again and qualify the native products and all three UI fixtures. Preserve both the compiler diagnostic and unsuccessful UI assertions; exclude the preflight and superseded runs from the primary counts above. [Compiler correction](validation/file-info-initial-8.87/initial-compiler-correction.json), [superseded UI](validation/file-info-initial-8.87/superseded-ui-checks.json), [superseded native checks](validation/file-info-initial-8.87/superseded-backend-checks.json), [advanced control correction](validation/file-info-initial-8.87/advanced-controls-correction.json).

## Cleanup and delivery

Remove 129 owned fixture directories and 214 temporary executable/object/resource files across current and superseded attempts. Preserve redacted evidence and verify all current product/core hashes, seven installed-file hashes, both personal catalogs and installed media-helper hashes. Keep the reused CaptureAdmission object required by the current build instead of deleting its containing superseded folder wholesale. [Cleanup](validation/file-info-initial-8.87/cleanup.json), [preservation](validation/file-info-initial-8.87/final-preservation.json).

The pending fixture reuses the existing 8.83 alias to installed ffmpeg/ffprobe. No media binaries or new aliases are created. The prior alias remains after automatic approval review rejected its removal as “blocked by policy”; no removal retry is attempted. [Runtime helpers](validation/file-info-initial-8.87/shared-runtime-helpers.json).

**8.87 is built and component-qualified; installed UDM remains 8.68 / extension files 0.72.32.** Browser JavaScript is unchanged. No installer, installation, commit, push, release or personal browser/IDM timing run is included.

## Remaining work

This removes view preparation and advanced queue construction from the initial UI lock path. Callers can still wait while restoring offers, taking automatic-capture contexts, admitting a job or preparing duplicate decisions before reaching `downloadInfo`. Preview/refresh preparation, category/scheduling actions and final prefetch teardown still contain synchronous model reads. The initial reader also has a cancellable 10 ms lock-retry wait; its release-to-ready latency is not yet compared with IDM.

Move the remaining preparation and teardown onto owned operations without dropping cancellation or acknowledgement barriers. Then compare cold/warm first paint, complete field readiness, exact pre-Start size, complete downloads and finishing/background work against IDM in the live environment. Full inside/out, visual, browser and timing parity remains unproven, and the goal remains active.
