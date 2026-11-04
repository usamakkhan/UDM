# Periodic synchronization end-of-day boundary — 1.2.7

[IDM's Scheduler documentation](https://www.internetdownloadmanager.com/support/idm-scheduler/idm_scheduler.html), retrieved 2026-10-30, says periodic synchronization repeats until its stop time or the end of the day when no stop time is specified.

UDM's daily scheduled mode already limits repeating cycles through its window and manual-stop logic. Unscheduled synchronization with RepeatMinutes greater than zero instead had no boundary unless StopWithoutStartEnabled was selected, so its cycles could continue indefinitely.

The independent-stop policy now includes that unscheduled synchronization case and chooses the next local midnight. A user-selected stop time remains authoritative. The existing deadline is preserved on an ordinary settings save, including after it expires; explicit Start, enabling a stopped queue, or configured startup activation re-arms it. The existing scheduler expiry path disables the queue and pauses its active work. No fixed 24-hour arithmetic is introduced: the existing local-time boundary resolver handles clock transitions.

Older repeating synchronization records without a stored deadline fail the existing expired-deadline check until explicitly started or saved, rather than silently continuing without a boundary. Disabling repetition, changing to download mode, or switching to a scheduled mode removes this implicit independent stop. One-time download queues retain their existing behavior.

Native version advances to 1.2.7; browser 0.63.82 is unchanged. All five production binaries compiled as 1.2.7 and installer packaging passed. Source hashes remained unchanged during the build. See [build evidence](validation/sync-end-of-day-build-20261030.json). Runtime tests remain deferred by user instruction. Installed UDM remains 1.2.5. This source change is not proof of complete Scheduler or IDM parity.
