# Download-list sort indicators — 8 October 2026

UDM changed download ordering without showing an active header indicator. The new baseline test reproduced this in the actual owned test app. The list now sets the native ascending/descending header flags for exactly the selected column. Menu sorting, repeated header clicks, cleared sorting and restored layout all update the indicator while preserving selected download identity.

The shared dark header painter now draws a thin chevron above the caption, aligned with the native light header placement. The glyph is clipped to its own column, respects physical window DPI and leaves column captions intact. An initial right-edge triangle was rejected during visual review; both final test executables and the app were rebuilt after alignment.

Final acceptance: 365 actual-app menu/workflow checks and 183 native rendering/shared-dialog checks pass. Rendering covers ascending, descending and cleared indicators in light/dark layouts at 96, 144 and 192 simulated DPI, with before/after pixel comparisons. Representative final captures were visually inspected. [Results and app hash](validation/sort-header-20261008.json).

Candidate: `C:\Users\Abuzar\AppData\Local\Temp\udm-sort-header-20261008\app\UDM.exe`. App and test entries were rebuilt using existing backend objects, with current Adaptive, Bridge and Network overrides. Existing compiler warnings remain. This candidate is not installed or packaged. These results establish UDM's sorting feedback and regression behavior, not exact IDM rendering or physical mixed-monitor parity. Temporary test objects were removed; reports, captures, executable and reusable App object remain.
