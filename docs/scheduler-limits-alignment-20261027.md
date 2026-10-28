# Scheduler quota-label alignment

Installed IDM 6.43.10.2 dialog 309 uses right-aligned static controls for
“Download no more than” and “every.” UDM used left alignment despite matching
their positions. These two controls now use `SS_RIGHT`, retaining their existing
dimensions and quota behavior.

[Extracted reference metadata](reference/idm-scheduler-limits-alignment-20261027.json)
records the two original styles and rectangles. This is resource evidence,
not a fresh live IDM interaction test.

The rebuilt Scheduler harness passed all 653 assertions, including the two
alignment checks and existing enable/disable and quota-persistence tests.
The count includes repeated per-control scale/theme checks, not 653 independent
scenarios. [Results](validation/scheduler-limits-alignment-ui-20261027.json).
The [rendered quota page](validation/scheduler-limits-alignment-20261027.png)
was inspected: labels align beside their fields without clipping.

The desktop build passed. Its executable is preserved separately in
`candidates/scheduler-limits-alignment-install/release-native/UDM.exe`, with its
hash in the [build record](validation/scheduler-limits-alignment-20261027.json).
This change is not installed and its installer has not been launched. The
separately staged date/shortcut candidate remains unchanged and is waiting for
Windows elevation. Build logs are `candidates/scheduler-limits-alignment-build.log` and
`candidates/scheduler-limits-alignment-app-build.log`.
