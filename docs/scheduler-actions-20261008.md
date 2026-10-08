# Scheduler queue-action symbols — 8 October 2026

The scheduler's compact Move up, Move down and Remove from queue controls had repeatedly misencoded Unicode strings. They displayed unreadable text and replaced their native button names with those strings. Source inspection found these three corrupted control captions in SchedulerUi.hpp.

ActionButton now supports a separately painted glyph while retaining its normal window text. Scheduler uses explicit Unicode escapes for up arrow, down arrow and minus; its text names, tooltips, original placement and action callbacks remain. Theme refresh preserves the symbol painter in light and dark mode. Default native accessibility naming can therefore use the readable button text, although no screen-reader session was tested.

The corrected-order unchanged-source baseline failed to find Move up. An earlier baseline skipped the scheduler because a preceding main-window fixture posted quit; that run is not scheduler validation. The final fixture runs scheduler first and explicitly verifies that its callback executed.

Final 82 checks passed: existing appearance coverage plus queue entries, named compact actions, move-up/down ordering, theme transitions, removing membership while retaining both catalog records, and no download starts. Both light and dark scheduler images were inspected. Tabs, tree background and list header still have a separate scheduler dark-theme gap; this change does not claim to fix them.

The app entry point and test executable were rebuilt using unchanged backend objects. Not a full clean build; compiler warnings remain. Raw scripts, images and results are under `C:\Users\Abuzar\AppData\Local\Temp\udm-scheduler-actions-20261008`. See [candidate and validation receipt](validation/scheduler-actions-20261008.json). Not packaged, installed or published. Complete IDM parity remains unverified.
