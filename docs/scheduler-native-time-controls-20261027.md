# Native Scheduler time controls

The retained IDM Scheduler capture shows segmented time controls with spin
buttons. UDM now uses native time pickers for scheduled start and stop instead of
free-text HH:MM fields. Keyboard arrows and spin buttons edit the time, and the
controls follow the existing daily, run-once and independent-stop enabled states.

Existing daily 24:00 end-of-day values remain 1440 in the catalog when unchanged,
although the native control displays midnight. Overnight schedules and run-once
local-date conversion keep the existing backend semantics. UDM still schedules
to the minute: seconds editing shown by IDM remains a separate backend/UI gap.
This is progress toward the reference interaction, not complete Scheduler parity.

## Validation

The native Scheduler dialog harness passed **65 checks**, including spin-button
presence, loaded times, midnight preservation, overnight edits, arrow-key editing,
disabled-start behavior, independent stop, run-once conversion and catalog reload.
Existing numeric-input and Download limits checks also passed. The rendered dialog
was inspected: time labels and spin controls fit without clipping.

Reference: `docs/gui-parity-2026-10-18/idm-scheduler-schedule.png`.
Rendering: [Scheduler time controls](validation/scheduler-native-times-20261027.png).
Results: [65 native dialog checks](validation/scheduler-native-times-20261027.json).
The harness uses its own catalog and windows; no personal queues are edited.
The UI test and desktop builds succeeded; `git diff --check` passed.
Candidate SHA-256:
`0F0663EF7040BBFDC5B3E5A8BD5D5E52F4C4431A46C562CBDB186C5D208F566A`.
Windows reported that elevation was cancelled; this candidate was not installed.
The subsequent seconds-support work builds on this native-control change.
