# Scheduler file-tab reference correction

The older image named `idm-scheduler-files.png` shows Schedule, not Files in the
queue. Its associated audit claim that IDM uses wide captioned action buttons was
unsupported. Live inspection was attempted but Windows control returned
`GetCursorPos: Access is denied`, and a fresh window lookup found no targetable
IDM window. No IDM downloads or settings were changed.

Read-only extraction from installed IDM 6.43.10.2 shows that dialog 307 uses a
264×181 dialog-unit file list and three compact 15×14 action controls, at x=9,
29 and 47 and y=210 relative to the page. UDM's native layout already matches
these rectangles with its page offset (146,30). No button enlargement was needed.

The expanded Scheduler harness passed **84 checks**, including actual control
rectangle comparisons under the dialog's measured font metrics. The owned
[file-tab rendering](validation/scheduler-files-geometry-20261027.png) was
inspected. [Results](validation/scheduler-files-geometry-20261027.json).
[Reference metadata and executable hash](reference/idm-scheduler-files-20261027.json).

Resource geometry does not prove live interaction or complete visual identity.
The start-date picker remains a known difference: IDM dialog 306 uses a native
long-date format, while UDM still displays its ISO-style date. Further live
acceptance, date formatting and other Scheduler differences remain open.
