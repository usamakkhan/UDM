# Compact Download File Info in UDM 5.37

File Info creates the controls used in its compact view first. Opening More creates the source-page, queue and login controls. Closing and reopening More keeps its edited fields. Download Later uses the selected queue even after More closes. Confirmations that start expanded, such as authenticated or POST downloads, build their full form immediately. Held media shows the retained byte count and refresh instruction on the initial compact view.

The 5.37 build staged five desktop executables and 95 files across the Chromium and Firefox 0.69.1 bundles. The focused browser checks passed 38 cases per bundle. The native presentation fixture passed 163 checks, including queue selection, login draft retention and the original Start, Later and Cancel actions. [Native build receipt](file-info-first-paint-5.37-build.json)

## Native presentation

| Component run | Child controls created before File Info showed | Native dispatch to show |
|---|---:|---:|
| 5.36 | 41 | 219 ms |
| 5.37 | 25 | 156 ms |

These are individual WinEvent observations from separate component runs. The control count identifies the work removed from the opening path. The timing samples do not establish a latency distribution or an IDM comparison.

## Cold HLS selections in Edge

Each case used a fresh Edge profile, an empty download catalog, the staged extension and native host, and the native presentation fixture running the production File Info and transfer code. The browser had already loaded the player and its format menu. The fixture served an eight-second HLS source on loopback. Timing started immediately before the browser automation issued the final selection click. The native timestamps report the first observation of an enabled Start button, positive received bytes and the exact-size label. The dialog observation used a 15 ms timer and includes Windows message scheduling.

| Selected output | Click to native admission | Click to actionable File Info | Click to first observed bytes | Click to exact size | Shown and published bytes |
|---|---:|---:|---:|---:|---:|
| 720p MP4, Spanish audio | 177 ms | 986 ms | 1,786 ms | 5,084 ms | 6,560,076 |
| 360p TS, English audio | 250 ms | 712 ms | 1,207 ms | 3,948 ms | 1,961,216 |
| M4A, Spanish audio | 212 ms | 451 ms | 1,070 ms | 2,885 ms | 98,791 |

In each run File Info was actionable while the main list was hidden. Bytes arrived before Start, and the destination was absent while File Info showed the verified final size. The published byte count and SHA-256 matched the verified staging file. FFmpeg decoded the entire output with errors treated as failures. FFprobe and the audio tone check confirmed the expected timeline, selected resolution or audio-only output, and language. The temporary native host registrations were removed.

The staged production desktop also passed 25 checks across five transfers in a separate Edge run with File Info bypassed. This covered nested 720p MP4, M4A, nested 360p TS, flat MP4 with Spanish subtitles and a cyclic master playlist. The browser fixture distinguishes the quick quality buttons from their adjacent audio and subtitle options.

The provisional 500 ms click-to-actionable target was met in the M4A sample. Native admission to actionable observation varied from 234 to 797 ms. Trace the delay between the native notification, its UI-thread dispatch and the first actionable paint next. Cold public YouTube size and a paired comparison with installed IDM still need separate runs.
