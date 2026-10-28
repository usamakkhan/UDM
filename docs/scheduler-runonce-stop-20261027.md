# Run-once stop control

Run-once schedules now retain a visible Stop download at checkbox beside the time
picker on the Schedule page. Previously this mode replaced the checkbox with a
Stop at label and required the user to enable the stop date on Automation.

The Schedule checkbox and advanced Stop on date checkbox synchronize both ways.
The native stop-time picker follows their enabled state, and Apply saves or clears
the run-once UTC deadline accordingly. The advanced date picker remains available
for stop dates different from the start date.

## Validation

The native Scheduler harness passed 78 checks, including both checkbox directions,
enabled state, second-precision deadline persistence, deadline removal and catalog
restart. Existing numeric, quota, daily, independent-stop and time-picker checks
also passed. The [rendered dialog](validation/scheduler-runonce-stop-20261027.png)
was inspected with an enabled 6:30:15 PM stop visible on Schedule.

[Native dialog results](validation/scheduler-runonce-stop-ui-20261027.json).
The test build, desktop build and diff check passed.
[Candidate and installation status](validation/scheduler-runonce-stop-20261027.json).
The preceding seconds-build installation completed before this follow-up was
installed. This candidate was then installed successfully, with matching SHA-256:
`C41432222C2129EAAA95F4B8166549DB01663F1F4B026F9F6FCFC48E5F6CDD87`.
