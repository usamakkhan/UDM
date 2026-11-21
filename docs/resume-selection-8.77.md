# Owned main-list Resume, 8.77 / browser 0.72.41

Main-list Resume previously took synchronous availability and deferred-selection leases, classified selected jobs under Manager, and ran recovery and durable retry queueing on the window thread. Content choices also wrote into live job state between successive warning dialogs. This increment moves ordinary selected retry processing and accepted content decisions into joined commands.

## Worker selection and authoritative retry

The actual CMD_RESUME entry captures selected row identities with selected(false), bypassing the old synchronous availability and deferred-membership reads. ResumeSelectionTask reuses the existing FileInfoDecisionTask owner and acknowledgement guard. Its worker validates current membership, activity, relocation and retry eligibility, handles recovery through Manager::resumeIf, durably queues ordinary retries and dispatches transfer work.

Selected identities are coalesced across active commands, queued requests, retained follow-ups, content decisions and requested login ownership. A stale or removed row cannot resurrect its former download. The guarded resume predicate reclassifies the job before and after recovery; a newly required duplicate, address-refresh, login or content decision becomes a follow-up rather than an unchecked retry. Per-item failures report their outcome while allowing later selected jobs to proceed.

Ordinary selected retries can proceed without waiting for a different selected job's confirmation. Follow-ups preserve their relative order, with address refreshes deferred until the other follow-ups settle. The main window retains follow-up views until a nonblocking current-context check succeeds. Busy observations retain the view; changed jobs are discarded. Queue login transfers ownership to the prepared requested-login pipeline from 8.76.

## Complete content decisions

The two existing native content warning dialogs read their immutable view, not Manager. Closing the main window cannot destroy their presentation owner. Cancel at either warning abandons the whole decision: no first-choice flag is left behind when the second warning is cancelled.

After all required choices are supplied, one worker validates the original request context and saves the complete choice together, rolling back all choice fields if that save fails. Choosing Keep replaces an old Strip choice, and choosing Retry clears an old permission to save web-page content. The worker then uses a guarded resume against the saved context and immediately dispatches accepted work.

This rare content-warning path deliberately uses a separate durable choice save before retry queueing. If the choice is committed but subsequent retry fails, the outcome says that the choices were saved. It does not pretend to roll back a successful save. The ordinary Resume path adds no such choice save, and transfer request generation is unchanged. Neither operation is represented as a timing benchmark against IDM.

## Presentation, acknowledgement and shutdown

The command guard remains owned until the UI consumes a completed result and joins its worker. Main timer and completion messages poll only ready commands, then release the guard before processing new follow-ups. Transfer-event presentation checks the guard before consuming any queued notifications or entering Manager, so early progress/completion events cannot block behind an accepted Resume save. Duplicate commands cannot drop another owner's guard. Main close defers during accepted backend work or a confirmation presentation. On normal shutdown, unstarted UI requests and read-only follow-ups are discarded, and accepted worker work is joined before Manager shutdown.

**Duplicate handling and address-refresh dialog initialization still use their existing authoritative presentation helpers.** This increment checks their request view first but does not remove every synchronous read inside those dialogs. Other command availability reads, network polling, capture review and completion actions remain separate work.

## Qualification

All five native products report **8.77**; both source manifests are **0.72.41**. Bridge is the only recompiled shared core. Transfer is byte-identical to 8.76, and both independent UI compiler-input/core maps match the qualified product build. [Build identities](validation/resume-selection-8.77/build.json).

Twenty-two backend suites pass **2,314 assertions** in forty-four C:/D: runs. New Resume tests cover a held catalog, duplicate and early-result rejection, coherent follow-up ordering, stale and removed jobs, actual pinned-catalog queue/choice save failures, rollback and recovery, committed-choice retry failure, later-item progress after an earlier failure and joined teardown. The duplicate normal selection causes one accepted local-origin body request. Successful retries are checked against the complete exact payload. [Backend evidence](validation/resume-selection-8.77/backend-checks.json).

The actual MFC main/login/progress fixture passes **139 assertions** on an owned separate desktop. It selects the real main-list row with deferred history validation, holds Manager independently, invokes the actual CMD_RESUME twice, dispatches the main timer and Close, then checks one owned retry and a complete exact file. It also opens the real two native content warning dialogs while an independent writer owns Manager, cancels the second prompt and verifies the entire original job is unchanged. A second real No/Yes confirmation saves the choices on the worker and publishes the complete exact file. Earlier automatic/requested login, progress and authenticated transfer cases remain included. [Main/UI evidence](validation/resume-selection-8.77/ui-checks.json).

The existing separate progress-menu/window fixture passes **88 assertions**. Both test desktops close normally, without switching to them or injecting input into the user's applications. [Progress evidence](validation/resume-selection-8.77/progress-menu-ui-checks.json). These selected tests do not establish a full UI acceptance-suite result, installed public-browser behavior or fresh IDM timing parity.

A prequalification review found the transfer-event presentation read described above. UDM and both UI fixtures were rebuilt after adding that gate and its held-model notification-retention test. The backend source closures and shared cores were unchanged, so the forty-four backend runs still match their compiled dependencies. Preliminary UI assertions are excluded from the final counts; their two owned fixture roots are included in cleanup. [Adjustment identities](validation/resume-selection-8.77/prequalification-adjustment.json).

After evidence capture, all **48 owned fixture roots** and **48 temporary test executable/object files** were removed. Qualified products/cores, rollback data, installed files and both personal catalog hashes remain. [Cleanup](validation/resume-selection-8.77/cleanup.json), [preservation](validation/resume-selection-8.77/final-preservation.json).

## Delivery and next work

**8.77 is a qualified development candidate. Installed UDM remains 8.68 / extension manifest files 0.72.32.** No installer, installation, extension activation, push or release is included. The manifest increment records this paired improvement; browser scripts are unchanged.

Continue duplicate/address-refresh presentation preparation and the remaining network, capture/review and completion paths. Fresh authorized installed-app/browser comparisons with IDM, including joined background work, remain required for the broad parity objective. This increment does not claim universal speed or visual parity.
