# Responsive file-completion action countdown — 9.93

Native **9.93**, Chromium and Firefox source manifests **0.73.57**. This increment changes desktop behavior; the browser increments follow the requested paired-version policy. It introduces no browser behavior change. All five normal native programs are built. All **20 backend and 66 UI checks pass**. Installation and release are not performed.

## Blocking point and implementation

The file-completion action dialog previously called `Manager::tick()` inside its UI timer. That method can process recovery, queue scheduling and checkpoints while holding the model lock. Its following eligibility checks also waited for the model. A writer or scheduler pause could therefore freeze the countdown, repaint and Cancel button.

The file countdown now requests ticks through the existing owned `ModelMaintenance` worker. `tryFileCompletionCountdown` uses try-lock admission, defers to an accepted File Info decision, and returns an owned filename snapshot. Initial Busy admission retains the queued action. WaitForOthers retains it behind active or queued work. A removed, non-complete, active or scanner-rejected source invalidates the action.

Inside the dialog, Busy leaves Cancel usable and displays “Checking download status...”. Eligibility is re-read on each usable pulse, including after the deadline. The UI never joins scheduler work. Cancel consumes the already-accepted action once, matching the prior cancellation policy; it cannot execute a shell or power step.

Moving the tick alone is insufficient: an expired timer must not act on a scheduler view that has not been refreshed. `CompletionCountdownDeadline` first drains older maintenance, requests a new tick and waits asynchronously for its completion count. A failed final tick or stopped worker rejects the action. After acknowledgement the UI still rechecks current membership, completion, activity, scanner and WaitForOthers policy before closing to execute. Scheduler errors remain visible through the existing status channel. No completion claim or catalog barrier is removed.

The existing main-window maintenance owner outlives this modal dialog. Normal shutdown disarms notifications and joins it before stopping Manager. No new worker, detached thread or timer is added to production.

## Scope and remaining work

This fixes the **file-completion action countdown**. Queue-finished countdowns still call scheduler/eligibility functions synchronously and remain the next UI task. Automatic file/folder shell operations and secondary-modal ownership also remain open. This is not a fix for the body/intent flush stalls recorded in the [9.92 controls](availability-controls-9.92.md), nor a new IDM speed comparison or universal parity claim.

Focused qualification covers real foreign model ownership, accepted decisions, job/scanner changes, WaitForOthers policy, final scheduler acknowledgement/failure, actual light/dark modal countdown pulses past expiry, repaint and Cancel. UI fixtures are compiled separately with `/Od`; normal programs use `/O2`. Their controlled waits are responsiveness checks rather than normal-product performance measurements. The deadline acknowledgement currently uses the existing 250 ms dialog pulse; immediate completion wake-up remains a separate improvement.

An initial attempt built the normal products but failed to compile the isolated UI fixture because of a duplicate local name and an incorrect theme variable. That attempt is unqualified. Deadline review also required explicit fresh scheduler acknowledgement; the final attempt captures updated inputs and rebuilds before acceptance. Initial hashes/logs are preserved separately. A later owned UI run correctly confirmed pulse/repaint responsiveness but incorrectly expected modal destruction inside the Cancel callback. Form uses EndDialog, which destroys the window after that callback returns. The final fixture checks callback latency and then verifies actual window destruction after the modal loop unwinds, with the writer still held; the failed assertion remains recorded.

Host installed **8.68 / 0.72.32** remains unchanged; the lab remains off. No installer, installation, commit, push or release is included. Temporary fixture cleanup remains blocked by the earlier automatic approval rejection; no cleanup is claimed.

## Final qualification and provenance

| Focused suite | Checks |
| --- | ---: |
| Read admission, accepted decisions, current scanner/job/wait policy and actual final-tick acknowledgement/failure | 20 |
| Actual light/dark countdown: held initial admission, pulse past expiry, enabled Cancel, repaint, click latency, modal destruction while still held and single consumption | 16 |
| Retained completion-claim UI: durable claim, cancellation, error/retry and shutdown acknowledgement | 50 |

The current owned UI observations record **0 ms** pulse duration in both themes and Cancel callback duration **16 ms** in the first/light case, **0 ms** in the second/dark case. These millisecond clock readings are not zero-time execution guarantees. Both writers remain held without watchdog expiry; after the callback returns, the modal loop destroys the window before the fixture releases its writer. The accepted scheduler work subsequently joins through the existing maintenance owner.

Final verification checks **721 source inputs**, **42 core objects**, all eight produced binaries and **nine protected host installation/tool files**. Only the version-bearing Bridge core object changes; App/resources/entry products are rebuilt. The other 41 core objects retain their qualified 9.92 identities. All five normal PE product versions read 9.93. Source changes during the final UI correction are limited to its test translation unit; final UI inputs and unchanged normal-product hashes are recorded. The earlier 20 backend results remain applicable and are not rerun or double-counted.

[Sanitized inputs, build logs, initial failures, exact final source snapshots and all focused results](validation/completion-countdown-9.93/). This is focused qualification, not a full suite, installed-browser acceptance, fresh normal download benchmark or proof of complete IDM parity. Queue countdown ownership, immediate deadline wake-up, shell/folder actions, installed checks and the finishing-speed gate remain open.
