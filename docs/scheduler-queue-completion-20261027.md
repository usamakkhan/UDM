# Queue page completion

Following the user's request to prioritize implementation, the confirmed live
file-queue differences are addressed together:

- Added Time left using the same ETA calculation and shared formatting as the
  main download list. Unknown, stalled, paused and live-recording estimates stay
  blank according to the existing calculation.
- Refresh filename, size, status and ETA while the Scheduler is open, updating
  only changed cells. Size uses the existing media-aware main-list formatter.
- Order compact actions Down, Up, Remove, and use a cross for Remove.
- Keep the existing 1–16 native parallel-download spinner; source review showed
  it was already implemented.

The existing geometry expectations were adjusted for the corrected button order.
No extra interactive test cycle or installation request was introduced. The
initial desktop build exposed an unavailable main-window helper. Its existing
size calculation was moved to a shared UI function; the main-window wrapper
retains its interface. The rebuild passed, as did `git diff --check`.
Logs: `candidates/scheduler-queue-completion-build.log` and
`candidates/scheduler-queue-completion-rebuild.log`.

The candidate is preserved under `candidates/scheduler-queue-completion-install/`,
with its SHA-256 in the [build record](validation/scheduler-queue-completion-20261027.json).
This batch has not been installed or interactively tested. The installed Schedule
navigation build remains unchanged while implementation is batched.

## Live queue updates

The next implementation batch reconciles the open queue against current queue
membership and order on each UI pulse. It rebuilds rows only when membership or
order changes, preserving surviving selections, keyboard focus and the top visible
file. Reconciliation waits until a drag finishes. Existing rows still update only
changed text cells.

Queue progress now uses the main list's shared percentage/error formatter, while
completed synchronization entries retain their synchronization status. This avoids
separate progress formatting drifting between the two views.

Interactive testing is deferred at the user's request. Compilation is recorded
separately; these changes do not establish complete IDM parity.

The live queue batch compiled successfully. Its candidate is preserved in
`candidates/scheduler-live-refresh-install/`; the build/hash record is
[here](validation/scheduler-live-refresh-20261027.json). It is not installed.
