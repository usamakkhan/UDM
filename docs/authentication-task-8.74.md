# Owned automatic authentication, 8.74 / browser 0.72.38

Automatic authentication previously saved its pending-flag claim on the window thread, then read settings and decrypted login sources during dialog initialization. A slow claim could block refresh and input; a failed or unpresented dialog also lacked an explicit restoration owner.

## Preparation and durable ownership

`AuthenticationPromptTask.hpp` adds one owned task per main window. It snapshots the eligible challenge and encrypted login sources under Manager, resolves initial credentials outside that lock on its worker, then rechecks current data and policy before making the existing durable claim. It publishes no ready view until the claim is saved. Claim-save failure retains the previous pending flag and reports an asynchronous error.

The immutable prepared view includes the job reference, selected challenge fields, encrypted saved-login source identity and initial credentials. It is never serialized or logged. MainWindow polls only completed results and uses a try-lock to validate the view against current job membership, challenge, pending state, queue ownership, suppression policy and saved-login sources. Busy validation retains ownership for a later pulse; stale views go through restoration rather than being presented.

The identity-checked `UiTaskSignal` wake-up is shared with the existing maintenance completion path. No HWND or control reference enters the authentication action. The regular timer remains free to dispatch while the worker waits for Manager or persists its claim.

## Presentation, retry and cancellation

Prepared automatic Login required construction reads its supplied view, including initial credentials, rather than locking Manager or decrypting settings. It uses the existing isolated capture-dialog owner instead of explicitly showing the main download list. Live foreground/focus parity with IDM is not claimed by the isolated test.

A normally dismissed dialog, including Cancel, acknowledges the already-saved claim without another save. Failed dialog creation retains the claimed view until a joined asynchronous rearm restores pending intent. Failed rearm persistence leaves pending true in memory and retains the unfinished restoration owner for retry. A changed or removed challenge cannot be overwritten or resurrected by restoration.

The Sign in and retry action validates the displayed challenge before applying credentials, then uses a guarded resume so a changed download cannot silently receive an obsolete retry. A real login dialog construction failure now throws instead of being silently treated as dismissal. Queue-origin/manual login callers retain their existing preparation route and receive the actual shown/not-shown result.

**Sign in and retry's explicit credential save/resume, and manual/queue-origin login preparation, remain synchronous commands.** This increment moves the main automatic preparation/claim/restoration path; it does not claim that every authentication operation is asynchronous.

## Shutdown

Window shutdown joins any active preparation/restoration and durably rearms an unpresented claimed view before Manager shutdown. Restoration failure prevents successful close and retains its owner for retry. The task destructor also settles owned work. A close request cannot destroy MainWindow during its active authentication presentation. There is no detached authentication work hidden after visible process completion.

File Info acknowledgement still defers claim, presentation and restoration. Suppression or a move to queue ownership restores the pending flag for the appropriate later workflow. Normal Cancel, changed challenges, claim failure, rearm failure and unpresented shutdown have separate tested outcomes.

## Qualification

All five production resources report **8.74**; both source manifests are **0.72.38**. Bridge is the only changed core unit and Transfer remains identical to 8.73. All compiler inputs remain stable across the qualified build. [Build identities](validation/authentication-task-8.74/build.json).

Eighteen backend suites on C: and D: pass **2,082 assertions**. The 43 new assertions per drive cover credential projection, durable claim state, exact challenge matching, current policy/source changes, asynchronous held work, duplicate ownership rejection, normal acknowledgement, joined shutdown/restoration, actual pinned-catalog claim and rearm failures, recovery, File Info deferral and removed jobs. FTP challenge metadata is covered without a live FTP transfer. Local Basic and Digest HTTP challenges each retry through the prepared-login application path and publish an exact **1,048,647-byte** file on both drives. These are owned origin fixtures, not public-site/browser downloads. [Backend assertions](validation/authentication-task-8.74/backend-checks.json).

The real MFC window fixture passes **97 assertions** on a separate owned Windows desktop. Its updated discovery cases use owned presenter callbacks to exercise asynchronous handoff, held persistence, current-view validation, failed creation and rearm, recovery and shutdown ownership. It additionally creates the **actual Login required dialog** from a worker-prepared view while another thread holds Manager, verifies the initial username/password controls against owned fixture credentials, and clicks that owned dialog's Cancel button. The dialog constructs and closes while the independent model hold remains active. Earlier control/history/timer/reservation cases remain included. The test frame and its desktop close normally, with no desktop switch or input injection into the user's apps. [UI assertions and input/core identities](validation/authentication-task-8.74/ui-checks.json).

The UI fixture records its independently captured declared-input hashes and all core object hashes; they match the qualified product build. No failed preliminary artifact is counted as passing evidence in this increment.

All **37 owned fixture roots**, including the four authenticated payload files totaling **4,194,588 bytes**, and **38 temporary test executable/object files** were removed after recording evidence. Qualified products/cores, source, rollback, installed 8.68 files and both personal catalog hashes remain. [Cleanup](validation/authentication-task-8.74/cleanup.json), [preservation](validation/authentication-task-8.74/final-preservation.json).

## Delivery and next work

**8.74 is a qualified development candidate; installed UDM remains 8.68 / extension manifest files 0.72.32.** No installer, installation, extension activation or fresh IDM/public-browser timing result is included.

Next give explicit Sign in and retry persistence/resume and queue-origin/manual login preparation their own acknowledged command/view boundaries. Continue reviewing capture/review presentation, network polling and completion actions according to their authoritative validation requirements. Fresh authorized live browser/IDM comparisons, including all joined background work, remain necessary before broad timing or full parity claims.
