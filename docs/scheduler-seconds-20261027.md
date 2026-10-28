# Scheduler seconds support

This candidate extends the native Scheduler time controls to hours, minutes and
seconds, matching the time-editing fields in the retained IDM reference capture.

Daily windows use optional `StartSecond` and `StopSecond` fields alongside the
existing minute fields. Independent stops use `StopWithoutStartSecond`. Missing
seconds default to zero, preserving legacy catalogs. Validation rejects malformed
or out-of-range seconds and a nonzero second after the 24:00 end-of-day boundary.

The same second precision is carried through window membership, overnight weekday
ownership, manually started daily stop deadlines, independent stop deadlines and
wake planning. Wake planning checks minute boundaries plus configured start/stop
seconds, rather than scanning every second of eight days. Run-once dates retain
their existing UTC representation. Applying an unchanged ambiguous autumn time
preserves its saved occurrence; editing seconds uses the existing first-occurrence
rule and clears hidden milliseconds.

## Validation status

`--schedule-seconds-checks` passed 113 checks, including existing queue and wake
checks plus second-boundary, legacy, validation and daylight-saving cases.
The Scheduler dialog harness passed 65 checks, including nonzero seconds in
overnight, independent-stop and run-once settings and persistence. The rebuilt
core also passed all 256 `--adaptive-prefetch-checks` media regressions.

Both test builds succeeded, and `git diff --check` passed. The
[rendered dialog](validation/scheduler-seconds-20261027.png) was inspected:
hours, minutes, seconds and spin buttons fit without clipping.
[Native dialog results](validation/scheduler-seconds-ui-20261027.json).

The previous minute-only candidate was not installed because Windows reported
cancelled elevation. The successor desktop build and installation succeeded;
the installed hash matched this candidate before installing the run-once stop
follow-up. See [run-once stop delivery](scheduler-runonce-stop-20261027.md) for
the newer installed executable.

Candidate SHA-256:
`48C69B019202032F310CDF3A764FAFD92A8396F21AD0D18BD9F925037A4A3013`.
[Build, test and installation status](validation/scheduler-seconds-20261027.json).
