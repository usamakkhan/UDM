# Streaming progress sorting — 18 October 2026

The download list displayed timeline-based streaming percentages and time estimates but sorted Status and Time left using raw byte counts. A 25% stream with unknown byte length sorted after a 75% ordinary download. Changes to a stream's remaining-time estimate also did not affect its order correctly.

Sorting now uses the same `downloadPermille` and `downloadSecondsLeft` helpers as the displayed cells. Streaming and ordinary transfers sort together, updated estimates change the ordering, and live recordings retain unknown remaining time. Selection survives reordering.

The isolated native menu/state test reproduced the percentage failure before the fix. With the fix, all 392 checks passed, including seven new checks for mixed streaming/byte progress, ascending and descending status order, changing ETA, live recording, and retained selection. These fixtures do not start personal downloads.

Evidence: `stream-sort-before-20261018.json` retains the failing baseline; `stream-sort-after-20261018.json` records the passing suite. The concurrent 1.0.3 commit captured the tests before the App.cpp fix; the fix remains a subsequent working-tree change. No fresh IDM behavior comparison or speed benchmark is claimed.

Installation follow-up: the tested A53AE586 desktop was installed and reopened successfully. All 28 catalog records retained their IDs, statuses, and sizes. The later viewport change is tracked separately.
