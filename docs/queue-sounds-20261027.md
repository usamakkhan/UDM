# Queue sound events

The retained IDM Sounds comparison lists Queue started and Queue stopped/finished.
UDM previously exposed only download completion, failure and pause sounds.

The Sounds page now includes both queue events, with independent enable switches,
WAV file selection and preview. Existing download sound preferences are retained;
the new queue sounds default to disabled for existing and new profiles until enabled.

The manager records audible queue activity in memory. A start event occurs when a
queue actually launches a transfer or synchronization worker, rather than merely
when Start is clicked. Further files and retries in that run do not repeat it.
Stopping manually, reaching the schedule stop boundary, removing the queue, or
finishing the cycle emits one stopped/finished event. Starting again can emit a new
start event. Empty queues produce no synthetic start. Failed queue-save operations
do not publish a stop event. Sound callback errors cannot stop queue processing.

This is implementation work, not a claim of complete IDM parity. Interactive tests
are deferred at the user's request. `git diff --check` passed. The desktop build passed; output is in `candidates/queue-sounds-build.log`.
The combined candidate and rollback installer are staged in
`candidates/queue-workflow-install/`. It has not been installed.
[Build record and hashes](validation/queue-workflow-build-20261027.json).
