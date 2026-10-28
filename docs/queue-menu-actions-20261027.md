# Named queue actions

The retained actual IDM main-menu capture lists `Start Main download queue`,
`Start Synchronization queue`, and corresponding Stop entries, without an extra
generic Start/Stop entry or separator.

UDM already refreshed queue names dynamically, so the old audit's claim that it
only offered Choose queue was stale. This change removes the extra generic row
and separator and adds Start/Stop to each named action. Toolbar and tray queue
popups use the same labels and escape ampersands in user-created queue names.
Existing queue names, saved configuration and direct toolbar-button behavior are
preserved. The large-list fallback remains available for more than 1,000 queues.

Source evidence: `docs/gui-parity-2026-10-18/idm-main-menu.txt` (UTF-16).
The native build passed with these changes; interactive checks are
deferred at the user's request. This does not claim complete menu-state parity.

## Stop command availability

The same retained IDM capture marks both idle queues' Stop commands grayed.
The implementation now disables Stop for queues with no active or pending run.
The shared manager predicate keeps it enabled for active transfers, enabled queued
files, pending synchronization, and manually started or cycling queues. Menu command
execution rechecks availability so a queue that becomes idle while the menu is open
does not receive a stale Stop action. Toolbar/tray popup entries use the same predicate.

An initial write encountered the active compiler's header lock and changed nothing.
The later retry succeeded. Compilation passed; runtime checks are deferred.

The Scheduler Stop button now shares this predicate on page changes and each UI
pulse, and rechecks it before changing queue state. Its Download limits page keeps
queue actions disabled. The direct toolbar button is unchanged because disabling
the split button would also block its menu of other queues.

[Combined build record](validation/queue-workflow-build-20261027.json). The candidate
is staged in `candidates/queue-workflow-install/` and is not installed.
