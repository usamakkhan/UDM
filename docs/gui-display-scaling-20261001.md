# Display scaling candidate

The current IDM file still matches the retained dialog-resource reference hash. UDM already uses measured dialog-unit sizes for Add URL, File Info, progress, completion and Properties. This reference does not establish runtime visual parity.

A broader UDM gap was identified: its executable declared system DPI awareness, its main window scaled only at startup, and pixel-layout dialogs ignored DPI changes. Nine baseline actual-control assertions failed.

The staged app now declares PerMonitorV2 with a per-monitor fallback. Main-window transitions rebuild fonts, toolbar/category/queue icons, resize the layout and apply Windows' suggested bounds. Column widths use retained logical measurements so fractional transitions do not accumulate drift; user-resized and hidden columns remain respected. Both dialog layout types rescale controls, and the floating basket updates its size, font and menu button. Dialog outer sizing now accounts for the target DPI. Missing category artwork uses UDM's own icon rather than shifting image indices.

The final build passed 19 actual-control DPI checks, 27 desktop workflows, and 17 real progress-dialog checks. Tests sent DPI-change messages for 100%, 125%, 150% and 200% to private controls without changing display settings. The actual executable's embedded per-monitor manifest was independently extracted and checked. These tests do not reproduce physical movement between different-DPI displays.

The installer was rebuilt after the final dialog-border correction. Only UDM.exe changed among 132 payload inputs. Prior app/package versions, failed fixture/build attempts and intermediate results are preserved. Personal catalogs and installed binaries remain byte-identical. No installation or publication occurred.

Physical multi-monitor moves, common-dialog behavior, all themes/custom controls and accessibility remain unqualified. Full IDM parity, installation lifecycle and public-video compatibility are still incomplete. The new dialog scaling API requires Windows 10 version 1703 or newer.

Windows references: [DPI-change handling](https://learn.microsoft.com/en-us/windows/win32/hidpi/wm-dpichanged), [dialog scaling policy](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setdialogdpichangebehavior), [DPI-aware outer sizing](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-adjustwindowrectexfordpi).

[Exact tests, hashes and retained fixture issues](D:/UDM-Workspace/candidates/release-084-055/control/dpi-acceptance.json)
