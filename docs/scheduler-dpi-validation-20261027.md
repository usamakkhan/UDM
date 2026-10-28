# Scheduler scaling validation

The Scheduler UI harness now exercises 96, 120, 144 and 192 DPI in both light
and dark themes. It sends `WM_DPICHANGED` to its own test dialog without changing
Windows display settings. Each combination checks that native date/time values
survive, visible controls remain within the client area, and the seven bottom
action buttons do not overlap.

The expanded harness passed 651 assertions, including its existing functional
checks. These are repeated control-level assertions across eight combinations,
not 651 independent scenarios. All eight captures were visually inspected:
long dates, time values and action captions fit. Dark-theme disabled controls
retain the existing embossed native rendering; this work does not fix that style.

[Machine-readable results](validation/scheduler-dpi-ui-20261027.json).
The captures remain in `candidates/scheduler-dpi-ui-20261027/`.

This validates simulated DPI handling, not physical mixed-monitor transitions.
Only the test harness changed in this step. The desktop candidate and pending
installation are recorded in
[the date/shortcut update](scheduler-date-shortcuts-20261027.md).
