# Owned Sign in and retry, 8.75 / browser 0.72.39

The login dialog previously saved credentials and resumed the download on the window thread. The automatic preparation task introduced in 8.74 did not move this explicit command off the UI thread. This increment gives the command its own acknowledged worker, while preserving authoritative validation and persistence.

## Command and ownership

`AuthenticationLoginTask.hpp` reuses `FileInfoDecisionTask` and its shared presentation guard. The UI captures the displayed challenge identity, expected pending state, username, password and Remember choice before starting. The worker has no HWND or control references. Credential validation and saving run under Manager on that worker; guarded resume rechecks the saved identity, then immediate queue dispatch runs on the worker too.

The shared decision guard remains set after worker completion until the dialog consumes the outcome and joins the worker. Duplicate submissions are rejected without dropping another command's guard. A busy dialog disables its edits, Show password, Remember, Sign in and Cancel, displays a short saving message, and continues dispatching window messages. Escape, title-bar Close and main-window Close cannot abandon the active command. Forced owner teardown joins real work rather than detaching it.

## Failure and retry

A credential-save failure retains Manager's existing rollback behavior for the job and remembered-site settings. The dialog re-enables its controls and leaves the original displayed identity available for retry.

A successful save followed by a failed retry returns a distinct outcome with the committed identity. The dialog retains that identity for a freshly validated retry. A changed or removed download cannot receive credentials or be resurrected from an obsolete dialog. If scheduling fails after retry was queued, the outcome reports that distinction instead of pretending credentials were rolled back.

Automatic prepared dialogs and manual/queue-origin dialogs share the new command. Manual and queue-origin **initial preparation remains synchronous**; this increment does not remove every possible authentication-related UI wait. Nor does it promise immediate UI refresh while disk work is unfinished: the acknowledgement guard deliberately defers conflicting presentations.

## Qualification

All five native products report **8.75**; both source manifests are **0.72.39**. Bridge is the only recompiled shared core; Transfer remains byte-identical to 8.74. The UI fixture's independently captured compiler inputs and core hashes match the product build. [Build identities](validation/authentication-login-8.75/build.json).

Twenty backend suites on C: and D: pass **2,170 assertions** across forty runs. New command cases cover held persistence, duplicate submission, early result consumption, actual pinned-catalog save failure and recovery, post-save retry failure, stale/removed challenges, manual pending-state validation, joined teardown and another decision's guard. Basic and Digest retries publish exact complete local-origin payloads; these are private fixtures rather than public-site or browser downloads. Existing File Info decision and authentication preparation suites also pass. [Backend evidence](validation/authentication-login-8.75/backend-checks.json).

The actual MFC window fixture passes **104 assertions** on a separate owned desktop. It obtains a real HTTP authentication challenge, opens the real Login required dialog, fills owned credentials and clicks Sign in and retry. While credential saving deliberately holds Manager, it verifies disabled duplicate actions, dispatches the actual main timer, requests main-window and login-window Close, and sends a repeated submit. The dialog stays owned, dispatches these messages, and invokes one command. After release it closes normally, and the retried local download matches the complete payload. Earlier prepared rendering and model-contention checks remain included. No switch to that desktop or user-app input injection occurs. [UI evidence](validation/authentication-login-8.75/ui-checks.json).

All **41 fixture roots** and **42 temporary test executable/object files** were removed after evidence capture. Qualified products and shared cores remain; installed 8.68 files, rollback data and both personal catalogs are preserved. [Cleanup](validation/authentication-login-8.75/cleanup.json), [preservation](validation/authentication-login-8.75/final-preservation.json).

## Delivery and next work

**8.75 is a qualified development candidate. Installed UDM remains 8.68 / extension manifest files 0.72.32.** No installer, installation, Chrome extension activation or fresh IDM timing comparison is included. The manifest increment records this paired development improvement; it does not imply browser script changes in this increment.

Next move manual/queue-origin login preparation to a coherent view boundary, then continue the remaining authoritative presentation, network polling and completion-action work. Fresh authorized live browser and IDM comparisons, including joined background work, are still required before any full parity or universal timing claim.
