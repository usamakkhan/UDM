# File Name column visibility — 18 October 2026

Live IDM 6.43 observation: selecting File Name in Columns enables Hide and Move Down, disables Show and Move Up, and displays its current width. The dialog was cancelled without saving. This agrees with the retained Columns visibility-handler pseudocode; the earlier UDM-only protection was not reference behavior.

UDM now permits File Name to be hidden using the checkbox or Hide. Applying and restoring a saved layout honors its zero width instead of forcing it to at least 40 pixels. Its previous positive width remains available to Show. Cancel discards visibility changes. This change does not alter download records or file contents.

The isolated native harness passed 158 checks with no failures (`columns-visibility-ui-20261018.json`). Added coverage checks hidden File Name after applying, restoring the saved layout, reopening, cancelling Show, and applying Show to recover the remembered width. The existing simulated 96/144/192 DPI layout checks also passed. This is not a full process-restart or physical-monitor test.

Source hashes at validation:

- `MainFeatures.hpp`: `4AE78DEF40DBB890A53BA82D7E129AEABAC19AF75C20AED32E2B9475B3554797`
- `ColumnControlsUiTests.cpp`: `9478E74AA24A4C6B92E82B20BB4F6A31F9E1690815689D74FA57026753F08576`

The repository build script now exposes `-ColumnControlsUiTestsOnly`. Its syntax and conflicting-target rejection were checked. The validated executable was built using the existing incremental developer script and previously validated backend objects; a full clean build was not run for this change.

The desktop build succeeded. Candidate SHA-256: `139D563C95CC3E9325C6E09DCCFC00F9B1B1E6D1943FD23F72B0102999C4874B`. It was installed successfully after elevation. The installed hash matches the candidate, UDM reopened normally, and all 28 download IDs, statuses and sizes were unchanged. Live selection of File Name enabled Hide and disabled Show, with its remembered width displayed; the dialog was cancelled without saving. The four-pixel caption/frame mismatch remains unresolved; this functional change does not claim to fix it.
