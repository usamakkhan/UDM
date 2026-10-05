# Find dialog alignment — staged candidate

The current installed IDM resource 506 defines a 230 by 148 dialog-unit Find dialog with one input, selectable fields, case/whole-string controls, and Find/Cancel buttons. The older resource 487 uses two separate inputs and was not used as the current design reference.

UDM DesktopUi.hpp now uses the resource 506 dialog-unit dimensions and control rectangles, the exact address-field label, and a default Find push button. UDM uses its shared Tahoma 8-point dialog-unit font; the resource says MS Shell Dlg 8. Therefore identical font rendering and pixel fidelity are not claimed.

The existing search engine was retained. A fresh actual-app test passed 17 checks for control labels/default button, filename search, F3 next/wrap, retained query, description-only whole-string search, Cancel preserving the previous query/fields, address-only search, case-insensitive filename matching and normal shutdown. Two synthetic completed records and empty files were used in an isolated temporary profile. The first fixture failed to open; the revised fixture used valid hexadecimal record IDs and actual empty files and passed. The first failure is not counted as successful acceptance.

[Reference controls](D:/UDM-Workspace/candidates/native-084-backup/control/find-dialog-reference.json) · [App checks](D:/UDM-Workspace/candidates/native-084-backup/control/find-dialog-app-results.json) · [Acceptance](D:/UDM-Workspace/candidates/native-084-backup/control/find-dialog-acceptance.json).

The app was rebuilt. The preceding 209 native checks predate this dialog-layout change and were not rerun or relabeled as current acceptance. Visual clipping, mixed-DPI transitions, every search negative/validation case and rendered IDM comparison remain unverified in this turn. Personal catalog and installed UDM/native-host/monitor hashes are unchanged. Candidate remains uninstalled; full IDM parity remains unfinished.
