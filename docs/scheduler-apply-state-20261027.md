# Scheduler Apply state

The retained installed IDM comparison showed Apply disabled on the unchanged
Scheduler while UDM always enabled it. UDM now snapshots editable control values
when each queue or quota page is loaded and compares them during the dialog pulse.
Apply is disabled initially, enabled after values change, and disabled again when
those values are restored or successfully saved. Changes are not collected or
written merely to calculate button state, so partial numeric input remains editable
and existing validation still occurs at save time.

The snapshot covers edit/combo text, check/radio state and native date/time values;
selection, focus, progress text and file list scrolling do not make the page dirty.
Quota controls are isolated from queue controls. Successful Start updates the saved
baseline; Stop rebases only its persisted enable flag, preserving other edits.
Existing save-on-close and queue-switch behavior is retained.

Interactive checks are deferred at the user's request. Compilation passed; output is in
`candidates/scheduler-apply-state-build.log`. The combined update is now installed;
the previous combined queue-workflow candidate remains separately preserved.

## Unchanged close and navigation

`saveAll` now skips persistence if editable values still match the saved baseline.
Previously, closing or switching queues unconditionally submitted the entire old
queue snapshot, even when the user had edited nothing. A queue that finished or
changed state while the dialog remained open could therefore have its newer
settings overwritten. The unchanged path now performs no queue or quota write.
Actual edited saves retain validation and merge only fields changed from the
loaded queue snapshot into the current manager record under its lock. Other queues
and unchanged runtime fields are preserved. Successful saves reload the committed
record and establish a fresh baseline. A concurrently deleted queue produces an
error rather than recreating it. Creating a queue saves current edits and adds only
the new record instead of replacing the entire old queue collection.

New queue creation also rechecks the current queue names under the manager lock,
preventing an externally created queue from being replaced by a stale dialog.

The combined candidate and rollback-capable installer are staged under
`candidates/scheduler-save-state-install/`. See the
[build record](validation/scheduler-save-state-build-20261027.json) for hashes.

Installation verified October 27 (America/Los_Angeles): target SHA-256 matched
`2267409C6F5A7C3FA4BE53AE99D7A8416C5A1240CD09086095139902F51588FC`.
UDM was reopened from Program Files and its process reported responding. This is
installation/startup verification, not the deferred interactive acceptance suite.
