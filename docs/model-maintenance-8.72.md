# Desktop model maintenance, 8.72 / browser 0.72.36

The main window and Options pulse previously called `Manager::tick()` and queue dispatch directly. Those calls hold the catalog while examining schedules, launching recovery/scans, preparing project-link conversion and potentially saving state. A timer or browser queue wake could therefore keep the window thread inside model work even after the history snapshot waits were removed.

## Worker and UI ownership

`ModelMaintenance.hpp` provides one lazy, owned worker for regular desktop ticks and queue dispatch. Repeated wake-ups coalesce into a pending flag per work kind; they do not launch parallel tick/dispatch workers or accumulate an unbounded command queue. Work runs outside the worker-state lock. Exceptions are counted, retain bounded messages, and do not terminate later maintenance; a failed notification also cannot stop the worker.

MainWindow callbacks carry only a live Manager pointer and enter existing model methods under its mutex. They recheck `fileInfoDecisionPending` while owning that mutex. Already queued maintenance therefore defers processing while File Info is awaiting UI acknowledgement. The later acknowledged timer tick reconsiders the model's existing queued work; these are wake-ups, not transferred download commands.

The worker uses `UiTaskSignal` to post only the live window's owned integer identity. The UI rejects stale identities, refreshes after completed work, then displays maintenance errors so that history refresh cannot immediately erase them. Regular timer work is requested after display work, and completion wake-ups refresh the display without waiting for another 500 ms timer interval. No HWND/control work runs inside the maintenance action itself.

The real timer's routine hangup observation, layout update, quota notice/pulse and queue-completion discovery now skip a busy model. Authoritative command paths keep their normal validation and persistence behavior; shutdown still captures the final layout synchronously. Options queues/ticks use the same worker rather than executing model work while holding a try-lock on the UI thread.

Completion countdowns retain a fresh synchronous scheduler tick before their authoritative exit/power readiness checks. Moving those checks requires a separate acknowledged scheduling boundary. Network-capture polling, active completion actions, authentication persistence and later capture/confirmation reads also retain distinct dependencies; this increment does not claim that every window path is nonblocking.

## Shutdown and recovery

MainWindow disarms its wake target, stops accepting maintenance, drops unstarted maintenance flags and joins every active callback before integration and Manager shutdown. Manager then retains its existing joins and final durability barrier. No maintenance thread remains behind the visible shutdown. A failed durable shutdown can restart the same live owner's maintenance and rearm notifications. The window destructor also disarms and joins its worker.

## Build and validation

All five production resources report **8.72**, and both source manifests are **0.72.36**. Bridge is the only changed core unit; Transfer retains the 8.71 hash. [Build identities](validation/model-maintenance-8.72/build.json).

Sixteen backend suites on C: and D: pass **1,934 assertions**. The new maintenance suite covers coalesced request storms, one-worker serialization, independent owner responsiveness during a held real Manager, stop/join and pending-work disposal, forbidden unjoined restart, successful restart, bounded error delivery, notification failures and retained CLI intent. Existing task-signal, CLI admission/completion, search, history, modal-size, writeback, checkpoint, publication and recovery suites also pass. [Backend assertions](validation/model-maintenance-8.72/backend-checks.json).

The real MFC main-window fixture passes **54 assertions** on a separate owned Windows desktop. It invokes the actual timer, queue-wake handler and Options model pulse while a different thread holds Manager. Each returns while that writer is still held; background work remains active waiting for Manager and the existing row stays displayed. After release the real tick/dispatch finish; layout catches up, stale completion identities are rejected and a valid completion wake updates the row. It then verifies that queued maintenance cannot advance Manager's tick counter while the File Info decision guard is held, and that a later acknowledged tick advances it once. Earlier modal, tooltip, toolbar, coherent navigation/history and deleted-row validation cases remain included. The fixture closes normally, without switching desktops or injecting user input. [UI assertions and core/source identities](validation/model-maintenance-8.72/ui-checks.json).

During prequalification review, the App entry was rebuilt against the final countdown policy, post-refresh error display and File Info guard. Only the final App/source pair and final UI fixture are claimed here; backend inputs remained unchanged, and the UI fixture's core hashes are checked against the final product build. The earlier owned UI fixture is preserved only as a receipt before its data is removed. [Design adjustment](validation/model-maintenance-8.72/prequalification-adjustment.json).

All **34 owned fixture roots** and **34 temporary test executable/object files** were removed after preserving evidence. Source, qualified products/cores, rollback, installed 8.68 bytes and both personal catalog hashes remain. [Cleanup](validation/model-maintenance-8.72/cleanup.json), [preservation](validation/model-maintenance-8.72/final-preservation.json).

## Delivery and next work

**8.72 is a qualified development candidate. Installed UDM remains 8.68 / extension manifest files 0.72.32.** No installer, installation, browser activation, live IDM comparison or whole-download speed improvement is claimed. These are owned component and backend tests, not operation of the user's browser or IDM.

Next capture and defer the remaining automatic discovery reads without consuming confirmations or authentication/review work prematurely. Treat network polling and completion actions according to their own ownership/acknowledgement requirements. Then obtain fresh authorized live browser/File Info and IDM comparisons, including all joined background work, before claiming broader timing or parity closure.
