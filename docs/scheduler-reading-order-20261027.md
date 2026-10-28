# Schedule-page keyboard order

The recorded daily and run-once traces in
[the quota keyboard report](scheduler-quota-keyboard-20261027.md) exposed a
separate navigation defect: relocated Schedule controls retained their original
creation order, so Tab reached bottom actions before scheduling checkboxes and
completion actions.

The Schedule controls now have an explicit native child order. The downloading
kind and timing radio pairs remain contiguous; the native date control ends the
timing radio group. Hidden or disabled controls continue to be skipped by
Windows. Bottom actions follow their displayed order, including Help before
Apply as in the retained dialog-308 reference.

New harness checks cover forward and reverse daily navigation, run-once date
navigation, and bottom action order. The retry-field lookup now uses its visible
checkbox instead of relying on adjacency to an obsolete hidden label.

The native harness rebuilt and passed all 684 assertions, including 24 new
navigation/date checks. The total includes repeated scale/theme assertions,
not 684 independent scenarios. [Results](validation/scheduler-reading-order-ui-20261027.json),
[daily traversal](validation/scheduler-daily-tab-order-after-20261027.json),
and [run-once traversal](validation/scheduler-once-tab-order-after-20261027.json).
The rendered Schedule page was inspected and retains its layout.
Build log: `candidates/scheduler-reading-order-build.log`.

The desktop build passed. Its tested candidate is preserved in
`candidates/scheduler-reading-order-install/release-native/UDM.exe` with the
SHA-256 in the [build record](validation/scheduler-reading-order-20261027.json).
Installation completed on 2026-10-27 after the user closed UDM normally.
The installed executable matches the candidate SHA-256:
`CC091430309D474C3487AC131D2A08606D759DFD1DDFBA9616185ACE995B8526`.
UDM was relaunched from Program Files and reported Responding. The previous
executable is preserved in the installation stage as `UDM.previous.exe`.
Current source order and resource geometry alone do not prove live IDM keyboard
equivalence; subsequent installed acceptance remains required.

## Native radio-key follow-up

A follow-up harness drives `WM_KEYDOWN` through MFC pre-translation with focus on
the actual native timing radios. Down selects Daily, Up returns to Once, and
Apply persists each corresponding scheduling mode. The separate startup
checkbox remains unchanged. All follow-up assertions passed;
[results](validation/scheduler-arrow-keys-ui-20261027.json). This changed the
test harness only, so the staged desktop executable is unchanged. This is an
owned-dialog keyboard test, not a fresh live IDM comparison.
