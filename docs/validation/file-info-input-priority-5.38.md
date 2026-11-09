# File Info input and prefetch timing

The 5.38 File Info dialog uses a short Windows timer to start background prefetch after the first paint. Windows schedules timer messages after higher-priority messages. This gives queued input a chance to reach File Info before the existing prefetch admission saves the confirmation state and starts a transfer. The regular File Info refresh timer can still start prefetch if the short timer cannot be set. [Windows timer scheduling](https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-timer)

## Trace coverage

The test driver records native admission, UI message dispatch, offer lookup and restoration, independent owner creation, dialog initialization, control construction, theme and paint, foreground request return, first prefetch call and first observation of the enabled Start button. It uses one Windows performance counter across the pipe and UI threads. Trace collection is opt-in, limited to 32 stage records and compiled only into the presentation fixture. The trace contains stage names and numeric timings.

The main window can find a durable offer during its periodic refresh before the pipe notification reaches the UI. The trace records both paths. Stage timestamps before the native notification are negative relative to that notification, and a later dispatch during the modal loop remains visible in the same trace.

The Edge fixture uses a fresh profile and empty catalog per run. It selects a 720p MP4 with Spanish audio from the same eight-second loopback HLS source. It records the selection click, first observed received bytes, exact-size label, Start and publication. For each build the fixture alternates five traced runs and five untraced runs. The native test binaries use matching compiler optimization settings. Runs are grouped by build.

## 5.37 baseline

| Metric | Observation |
|---|---:|
| Median click to actionable observation, all ten trials | 567.5 ms |
| Median click to actionable observation, trace on | 588 ms |
| Median click to actionable observation, trace off | 547 ms |
| Median native admission to UI dispatch, trace on | 0.2 ms |
| Median control creation, trace on | 120.2 ms |
| Median first prefetch call, trace on | 79.6 ms |
| Enabled Start observed before first prefetch call | 0 of 5 traced trials |

The timer-based observer samples on the UI thread. Its first enabled Start observation is an upper bound on when the dialog was ready to receive input. The click interval includes the browser-to-native handoff. The trace measures when paint calls return. Physical display timing requires a separate measurement.

## 5.38 results

| Metric | 5.37 | 5.38 |
|---|---:|---:|
| Median click to actionable observation, all trials | 567.5 ms | 385.5 ms |
| Median click to actionable observation, trace on | 588 ms | 374 ms |
| Median click to actionable observation, trace off | 547 ms | 390 ms |
| Click to actionable observation within 500 ms | 2 of 10 | 8 of 10 |
| Enabled Start observed before first prefetch call | 0 of 5 traced | 5 of 5 traced |
| Median click to first observed bytes | 1,083.5 ms | 1,161 ms |
| Median click to exact size | 3,953 ms | 3,571.5 ms |

In all ten 5.38 runs File Info left the main list hidden, received bytes before Start and showed an exact size of 6,560,076 bytes before publication. The published output matched that byte count and the staging SHA-256. Each file decoded through the full eight-second timeline with the selected 720p video and Spanish audio. All temporary host registrations were removed. A preliminary run completed and matched the output but exposed the periodic-refresh trace assumption. The table uses the complete series collected after the fixture learned both paths.

All 163 native presentation checks passed. The Chromium and Firefox 0.69.2 bundles passed 38 focused checks each. Five production executables and 95 browser files are staged. [Native build receipt](file-info-input-priority-5.38-build.json)

The timing series supports the change in event order. The median time to first observed bytes rose by 77.5 ms in this sample. The next investigation should separate prefetch admission persistence, worker startup and first response. Cold public YouTube timing and a paired installed IDM comparison remain open.
