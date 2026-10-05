# Offline Help and stable column widths — candidate acceptance

The native-084 overlay now provides working Help contents, Tutorials, Scheduler and queues, and Grabber Help. F1 opens contents. The offline dialog supports topic selection and Previous/Next navigation with wraparound. The text is original UDM documentation; IDM help content was not copied.

The first app run found that the topic selector did not update: its callback now uses the combo change notification. Subsequent runs exposed column widths shrinking when converted between logical units and display pixels. MainFeatures.hpp now uses rounded MulDiv conversions in layout save/restore and Customize Columns. The final run preserves the saved widths exactly through a real app restart at 120 DPI (125% scaling).

## Current verification

- Rebuilt MenuStateTests against the current App.cpp and overlay: all 200 native checks passed. This includes real loopback Stop/Resume transfers with byte verification, queue behavior, menu routing and enablement. This is a scoped suite, not an exhaustive app test.
- Fresh isolated actual-app run: all 33 checks passed, including four Help routes, topic navigation, F1, unchanged preferences, menus, queue Start/Stop, limiter persistence, recovery handoff and exact widths after reopen.
- Personal state and the installed UDM, native host and monitor binaries remain identical to the previous protected SHA-256 values.
- Failed earlier Help runs remain retained. They are not counted as successful acceptance.

Evidence: [acceptance manifest](D:/UDM-Workspace/candidates/native-084-backup/control/help-topics-acceptance.json), [native checks](D:/UDM-Workspace/candidates/native-084-backup/control/help-current-native-results.json), [actual-app checks](D:/UDM-Workspace/candidates/native-084-backup/control/ui-help-topics-current-app.json).

## Scope still open

This candidate is staged, not installed or released. The overlay depends on the native-083 base project and current Core/Bridge/Queue objects. Help is not yet the complete reference workflow: Tip of the Day, update/support/share actions and the full help presentation remain open. Multi-monitor DPI changes, other scaling levels, rendered visual equivalence and accessibility are not established by this 120-DPI test. Full IDM parity is unproven.

The regression fixtures for this acceptance are in the Windows temporary directory on C: to preserve D: working space. Earlier compression attempts reclaimed zero bytes; no savings are claimed.
