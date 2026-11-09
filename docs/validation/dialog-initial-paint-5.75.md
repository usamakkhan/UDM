# Native dialog initialization

Native forms assign fonts without requesting an immediate redraw while their controls are hidden. They apply the frame theme once during initialization and avoid toggling redraw around a hidden combo. File Info also skips resetting the empty category picker. The initial layout and control theme are ready before the dialog is shown. [Windows font messages](https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-setfont) · [Windows redraw messages](https://learn.microsoft.com/en-us/windows/win32/gdi/wm-setredraw)

Three alternating runs per build used the same local eight-second 720p HLS MP4 selection with Spanish audio in fresh Edge profiles. Each run showed the exact 6,560,076-byte output before Start, matched the staged hash after publication and decoded the whole file. All six temporary native host registrations were removed.

| Median metric | UDM 5.73 | UDM 5.75 |
|---|---:|---:|
| File Info control construction | 90.9 ms | 68.4 ms |
| Native admission to first paint return | 143.3 ms | 117.4 ms |
| Browser selection to actionable File Info observation | 232 ms | 190 ms |
| Browser selection to exact-size observation | 955 ms | 673 ms |

The candidate's actionable observations ranged from 185 to 280 ms. The baseline ranged from 230 to 334 ms. The comparison includes the address input change from 5.74 and ordinary run-to-run variation. It measures one local workflow. Installed YouTube and paired IDM timing remain open.

All 163 native presentation checks and 144 completion checks passed. The paired traces are in `udm-dialog-initial-paint-5.75` in the Windows local temporary folder. [Native build receipt](dialog-initial-paint-5.75-build.json)
