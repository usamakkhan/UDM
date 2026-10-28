# Scheduler quota keyboard navigation

A runtime trace using Windows `GetNextDlgTabItem` confirmed that selecting
Download limits left its page controls after the bottom actions. Starting from
the queue tree, Tab visited New queue, Apply, Help and Close before the quota
checkbox. [Recorded order](validation/scheduler-quota-tab-order-before-20261027.json).
The diagnostic run passed its existing 653 assertions; those assertions did not
yet cover navigation order.

The quota page controls are now ordered immediately after the queue tree using
native child-window ordering, without changing their positions or activation.
Hidden quota controls remain skipped when a queue page is selected. New tests
check forward/reverse navigation, disabled-field skipping, and enabled order:
tree, enable checkbox, amount, hours, warning checkbox, New queue.

The corrected harness built successfully and passed all 660 assertions,
including seven new navigation checks. This total includes repeated scale/theme
assertions. [Results](validation/scheduler-quota-tab-ui-20261027.json) and
[corrected runtime order](validation/scheduler-quota-tab-order-after-20261027.json).
The rendered quota page was inspected and retains its layout.
Build log: `candidates/scheduler-quota-tab-build.log`.

The desktop executable built successfully and is preserved in
`candidates/scheduler-quota-tab-install/release-native/UDM.exe`. Its hash and
status are in the [build record](validation/scheduler-quota-keyboard-20261027.json).
This correction was installed on 2026-10-27 after the earlier date/shortcut
installation completed. The installed SHA-256 matches the candidate:
`CD5ACD560AB7587052499F34501863202D414BBB831ADE06C86BDE1A1BCFC690`.
UDM was relaunched from Program Files and reported Responding. The earlier
alignment candidate remains separately staged and was not installed on its own.
This fixes a demonstrated UDM navigation defect; a fresh live IDM keyboard
comparison remains required before claiming identical navigation throughout.

## Further Schedule-page finding

A subsequent diagnostic harness run passed its existing 660 assertions, while
recording [daily](validation/scheduler-daily-tab-order-before-20261027.json) and
[run-once](validation/scheduler-once-tab-order-before-20261027.json) navigation.
The daily trace reaches bottom actions before Start download at, Daily, Stop
download at and completion controls. The stop-time picker also precedes weekday
choices. These observations reveal a separate Schedule-page navigation defect;
the quota-page fix does not resolve it. This remains the next correction, with
page-specific navigation regression checks required.
