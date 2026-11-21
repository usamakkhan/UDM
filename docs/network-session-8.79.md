# Owned network operations, 8.79 / browser 0.72.43

The main timer, Options timer and network integration window previously enumerated processes/endpoints, exchanged monitor requests and processed original responses on the window thread. Start, Capture and Stop also ran there. Those paths now share one owned operation at a time and publish immutable cached views to the UI.

## Operation and snapshot ownership

`NetworkCaptureSession.hpp` separates the existing network backend from its UI owner. Each accepted operation runs on a joined worker without HWND or control access. Process/root enumeration, endpoint queries, Watch/Capture/Snapshot exchange, response-controller work and explicit monitor controls execute there. The UI polls only a ready result, joins it and replaces its cached snapshot.

An idle background session without a monitor does not enumerate endpoints or start workers. The visible network window requests endpoint fallback explicitly. Busy polls are coalesced; overlapping control requests are rejected while controls are disabled. There is no unbounded queue, concurrent access to Monitor or automatic replay of a failed Start/Capture/Stop mutation. One operation thread is owned at a time; this is not a permanent pool thread.

The snapshot contains connection rows and observed monitor/capture state and counters. Raw response-capture records remain on the backend/controller path. Failed operations report an error and, where available, observed backend state while retaining safe cached rows. A later explicit poll can reconnect through the existing Monitor rules. Main and requesting-window notifications use weak, integer-identity wake-ups; closing a view cannot leave a worker owning its controls.

Main and Options timers request and observe the session without an outer Manager lease. The completion handler acknowledges ready results without creating an endless poll loop. The network window updates rows on a new result rather than rebuilding them merely because its busy flag changed.

## Controls and shutdown

Start, Capture and Stop return to the dialog after accepting their operation. Controls remain disabled until acknowledgement. Shared-window Close leaves accepted work and capture owned by the main session. Local-window Close remains responsive while waiting for the current operation and final durable Stop, then dismisses the window.

Main shutdown joins network work and its final stop before Manager shutdown. Failed durable Stop retains the session for an explicit retry. Main Close cannot destroy a network integration dialog's presentation owner. Final destructor fallback also settles owned work before releasing backend resources.

The native Start worker initializes and releases its own COM apartment for the existing ShellExecuteEx launcher. The existing no-async launch flag and Windows administrator-approval behavior remain. See [Microsoft's ShellExecuteEx guidance](https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shellexecuteexa) and [COM initialization guidance](https://learn.microsoft.com/en-us/windows/win32/learnwin32/initializing-the-com-library). Live elevated signed-driver startup was not exercised here.

## Remaining scope

**The explicit Driver status diagnostic retains its existing UI callback.** Other unconverted dialogs still take authoritative Manager leases. This increment removes network work from regular timers and monitor-control callbacks; it does not establish universal UI timing parity. Existing transport, response admission, cancellation and durability algorithms remain authoritative.

## Qualification

All five native products report **8.79**; both source manifests are **0.72.43**. Bridge is the only recompiled shared core. Transfer and Network core bytes remain identical to 8.78. [Build identities](validation/network-session-8.79/build.json).

Twenty-five backend suites pass **2,578 assertions** across fifty C:/D: runs. New session cases cover held model work, idle suppression, coalesced polling, overlapping-control rejection, immutable earlier views, dual completion notifications, uncertain mutation acknowledgement, explicit recovery, actual pinned-catalog Stop failure/retry and joined teardown.

The real Monitor reconnect fixture adds owned-pipe async-session integration alongside existing response-credit, pause, unknown-admission and adoption cases. Its **74 checks per drive** prove recovery of the same helper after lost Capture acknowledgement, without replay beyond two explicit periodic Captures. These are owned test peers; no live browser or signed driver is used. [Backend evidence](validation/network-session-8.79/backend-checks.json).

`MonitorReconnectTests` includes the real `Network.cpp` for its private peer fixture and excludes `Network.obj` from that fixture's link. The exception is recorded in the build receipt; other targets retain their normal linkage.

The actual MFC main/network/login/progress fixture passes **170 assertions** on a separate owned Windows desktop. It retains network rows while a worker holds Manager, dispatches main and Options timers, protects main Close, and closes a shared view without dropping accepted work. Real Start/Capture/Close controls reject a repeated held mutation and retain local Close until the owned catalog Stop save finishes. Earlier download, Resume, content, duplicate and login cases remain included. The separate progress-menu fixture passes **88 assertions**. Both desktops close normally without switching to them or injecting input into the user's applications. [Main/window evidence](validation/network-session-8.79/ui-checks.json), [progress evidence](validation/network-session-8.79/progress-menu-ui-checks.json).

## Fixture correction and input provenance

The first new transport integration failed because its peer omitted `PendingResponses`, which the real broker supplies and the controller requires. The peer packet was corrected, its executable rebuilt, and its complete local include closure hashed before that compilation. The failed C: run is separate and excluded from passing totals. Corrected C: and D: runs each pass all 74 checks. [Correction scope](validation/network-session-8.79/test-fixture-correction.json), [initial failure](validation/network-session-8.79/monitor-reconnect-initial-failure.log), [corrected C: log](validation/network-session-8.79/monitor-reconnect-corrected-C.log), [D: log](validation/network-session-8.79/monitor-reconnect-D.log), [corrected fixture inputs](validation/network-session-8.79/monitor-controller-fixture-inputs.json).

Only `MonitorReconnectTests.cpp` changed in that correction. Product, other backend and UI dependency closures were unchanged, so the passing C: session-worker run and both UI fixtures were retained. Their broad declared inventories still contain the earlier hash for that unused test source. The inventory exception is recorded instead of rewriting captured evidence.

The build recorded **546 declared input hashes**. A subsequent transitive-include audit found four protocol/vendor headers outside that pre-build list. Their contents match tracked Git files and their write timestamps predate this build. Those post-compilation checks are supplemental evidence, not pre-build hash capture. The corrected monitor fixture separately captures its complete local include closure. Future pre-build inventories must include these dependencies. [Supplemental scope](validation/network-session-8.79/supplemental-dependencies.json).

All **53 owned fixture roots**, including the failed fixture, and **54 temporary test executable/object files** were removed after evidence capture. Qualified products/cores, rollback, installed files and both personal catalog hashes remain. [Cleanup](validation/network-session-8.79/cleanup.json), [preservation](validation/network-session-8.79/final-preservation.json).

## Delivery and next work

**8.79 is a qualified development candidate. Installed UDM remains 8.68 / extension manifest files 0.72.32.** No installer, installation, extension activation, push or release is included. The paired manifest increment does not imply browser-script changes.

Continue browser-capture/duplicate preparation, File Info handoff, address-refresh initialization, diagnostics and completion paths. Extend dependency inventories before the next qualification. Fresh authorized installed-app/browser comparisons with IDM, including joined background work and live signed-monitor behavior, remain necessary for the broad parity goal.
