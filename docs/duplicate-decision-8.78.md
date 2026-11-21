# Owned duplicate decisions, 8.78 / browser 0.72.42

Duplicate confirmation previously resolved every choice on the window thread. Replacement could hash an existing file there; Use existing could also run recovery and durable resume there. Main-list Resume now prepares its duplicate prompt on its worker, and all duplicate dialog choices run through an acknowledged command.

## Prepared main-list prompts

DuplicateDecisionTask.hpp captures candidate and original identity, displayed address, original activity and replacement eligibility without filesystem probing. ResumeSelectionTask carries this immutable view alongside its duplicate follow-up. Before presenting it, Main validates both the Resume context and the duplicate context with nonblocking reads. Busy reads retain the view; obsolete views are discarded without deleting or resuming a changed record.

The prepared duplicate dialog constructs its controls entirely from that view. It does not clone live job JSON or read Manager to initialize its address, default option or replacement availability. A missing original disables Existing and defaults to Numbered; an active original cannot be selected for replacement.

## Choice, validation and follow-up

DuplicateDecisionTask reuses FileInfoDecisionTask's joined command and acknowledgement guard. The UI captures the selected mode and Remember checkbox, starts its command and disables repeat actions. Cancel, Escape and title Close cannot abandon in-flight persistence. The duplicate presentation also owns a close guard before any backend command starts, protecting dialogs whose immediate owner is not the main window.

The worker validates the displayed candidate identity. Existing also checks the original's request/destination identity while allowing benign progress/status changes; Numbered remains usable if the original was independently removed. Replacement requires the original context and eligibility to remain current. Its actual file probing, hash verification and replacement preparation still use Manager's authoritative algorithm, now on the worker. Cancel from an obsolete candidate view closes the old presentation without deleting the changed record.

The worker resolves the duplicate, then runs presentOffer and any existing-download recovery/resume before publishing the next presentation. Queued retries dispatch there too. A successful saved choice followed by a failed next action returns a distinct warning and does not replay or roll back the committed choice on the UI thread. The surrounding presentation uses the worker's Information/Progress/Complete result instead of synchronously calling presentOffer again for that choice.

Failed choice persistence retains Manager's rollback behavior for the candidate, original file and remembered policy. Controls are re-enabled and the real error is shown; the user can retry or Cancel. Normal result acknowledgement joins the worker before releasing the shared decision guard. Forced owner teardown also joins accepted work rather than detaching it.

## Scope still outstanding

**Nonprepared duplicate entry points still capture their initial minimal projection through the existing synchronous catalog route.** This increment gives Main-list Resume a prepared view; it does not move every browser/Add URL/Properties/Progress entry's initial snapshot to a worker. Automatic-capture context operations and subsequent Download File Info initialization also retain their existing authoritative reads. Replacement verification still holds Manager's existing lease on its worker, so other unmigrated synchronous UI commands remain separate work. No full-app zero-wait claim is made.

## Qualification

All five native products report **8.78**; both source manifests are **0.72.42**. Bridge is the only recompiled shared core. Transfer and the authoritative Duplicates core are byte-identical to 8.77. Both independent UI compiler-input/core maps match the qualified product build. [Build identities](validation/duplicate-decision-8.78/build.json).

Twenty-three backend suites pass **2,376 assertions** across forty-six C:/D: runs. New duplicate cases cover real offered candidates, held replacement work, duplicate/early-result rejection, acknowledgement, actual pinned-catalog save failure and recovery, remembered policy rollback, safe stale cancellation, original retarget/removal, existing-download retry, committed-choice follow-up failure and joined teardown. A real chosen replacement publishes the complete exact new payload and preserves the exact previous file; Use existing also publishes the complete exact authenticated payload. [Backend evidence](validation/duplicate-decision-8.78/backend-checks.json).

The actual MFC main/login/progress fixture passes **159 assertions** on an owned separate desktop. It invokes real main-list Resume for an offered duplicate, then independently holds Manager before the actual prompt renders from its prepared view. Clicking Cancel returns while its command waits; repeated Close cannot abandon it. After release, the candidate is durably removed and original bytes remain. Real Numbered, Existing and Replace radio/OK controls return the corresponding worker results, preserve original bytes before transfer and verify the replacement hash. Direct duplicate presentation is also protected from main-window Close before a command is active. Earlier held-model Resume/content, login and complete local download cases remain included. [Main/UI evidence](validation/duplicate-decision-8.78/ui-checks.json).

The existing separate progress-menu/window fixture passes **88 assertions**. Both desktops close normally without switching to them or injecting input into the user's applications. These selected checks are not a full UI acceptance-suite result, public-browser download or fresh IDM timing comparison. [Progress evidence](validation/duplicate-decision-8.78/progress-menu-ui-checks.json).

The first compilation found a const-iterator declaration mismatch and two fixture-only compilation issues. They were corrected before any backend checks or UI qualification ran. Unchanged compiled dependencies were reused where recorded; failed compilation is not included as passing evidence. [Correction scope](validation/duplicate-decision-8.78/prequalification-adjustment.json).

After evidence capture, all **48 owned fixture roots** and **50 temporary test executable/object files** were removed. Qualified products/cores, rollback, installed 8.68 files and both personal catalog hashes remain. [Cleanup](validation/duplicate-decision-8.78/cleanup.json), [preservation](validation/duplicate-decision-8.78/final-preservation.json).

## Delivery and next work

**8.78 is a qualified development candidate. Installed UDM remains 8.68 / extension manifest files 0.72.32.** No installer, installation, extension activation, push or release is included. The manifest increment records this paired improvement; browser scripts are unchanged.

Continue remaining duplicate/browser-capture preparation and File Info context handoff, address-refresh initialization, network polling and completion paths. Fresh authorized installed-app/browser comparisons with IDM, including joined background work, remain necessary for the broad parity goal.
