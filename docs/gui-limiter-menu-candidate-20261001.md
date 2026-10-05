# Speed Limiter menu — staged candidate

Installed IDM's menu resource confirms Downloads > Speed Limiter > Turn on / Turn off / Settings. UDM now exposes these routes with checked state. Turning off retains the previous positive rate; turning on restores it. Settings can edit and save that remembered rate while disabled. Input must be an integer from 1 to 1,000,000 KB/s. The dialog's explanation reflects the existing aggregate or per-download mode.

Verification:
- 80 native menu/transfer checks passed, including 11 new limiter checks: on/off state, rate retention, repeated off, per-download record preservation, mode preservation, persisted settings and invalid-rate rejection.
- Twelve actual app/dialog checks passed: reading the disabled rate, changing it with OK, retaining it on Cancel, toggling, restarting and enabling the remembered value.
- Personal installed binaries and catalog remain unchanged.

The initial cross-process test used SetWindowText/GetWindowText on the edit control and appeared to change 321 to 384, while the app's save handler still read 321. A separate diagnostic build confirmed the callback value. The corrected harness sends WM_SETTEXT and reads WM_GETTEXT; it passed on the unchanged production candidate. Failed runs and the diagnostic remain retained, not counted as acceptance. LimiterTrace.cpp, LimiterTrace.obj and UDM.LimiterTrace.exe are diagnostic-only and must not be packaged.

Evidence: D:/UDM-Workspace/candidates/native-084-backup/control/limiter-acceptance.json.
The reference establishes the menu structure, not every IDM limiter behavior or default value. UDM's existing transfer rate limiter is unchanged; this turn does not claim a new bandwidth measurement. Cross-interface remembered-rate changes through general Options, wider error paths, keyboard/DPI and complete visual parity remain unqualified. Candidate is staged, not installed or packaged. Full IDM parity remains unfinished.
