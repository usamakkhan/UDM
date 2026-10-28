# Unattended queue dialogs

IDM's Scheduler documentation states that queue-started downloads do not show
Download Complete or Error/Warning dialogs:
https://httpwww.internetdownloadmanager.com/support/schedulerD.html
The retained Downloads-options capture also says its dialog preferences do not
relate to queue processing. This is documented reference behavior, not a new live
IDM acceptance result.

UDM previously suppressed completion dialogs only for queues with completion actions
or a repeat interval. It also automatically opened credential dialogs for queued
failures, bringing the main window forward during unattended work.

The implementation now suppresses automatic completion and authentication dialogs
for queue-origin transfers. Authentication failures and pending-login metadata remain
in the catalog; no credentials are discarded and the retry policy still avoids blind
retries. Explicitly opening a completed record bypasses automatic suppression.

Actual queue launches mark QueueOrigin, including automatically eligible pending
jobs. Individual Resume continues to clear it; this now also applies to manually
retrying a failed synchronization check and its replacement transfer. Manual
completion is no longer suppressed merely because the assigned queue has repeat
or completion-action settings. Queue progress presents its completion-dialog
checkbox unchecked and disabled to reflect the automatic behavior.

Interactive and regression testing are deferred at the user's request. Source
checks and compilation are separate from runtime acceptance. This change is not
installed; the previous Scheduler save-state build remains installed.

The desktop build completed successfully. The executable and rollback installer
are preserved in `candidates/queue-unattended-dialogs-install/`.
[Build and source hashes](validation/queue-unattended-dialogs-build-20261027.json).
The open progress window refreshes its completion-dialog control when a queued
record becomes an individual resume. No automated or interactive tests were run.
