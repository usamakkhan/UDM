# Media prefetch startup in UDM 5.39

File Info now saves the selected media working folder in the same catalog write that records the confirmation gate. When the worker sees a matching durable record, it starts without repeating that write. A failed admission restores the prior record in memory. The saved state for a new unconfirmed media prefetch reopens paused after interruption, including offers that were previously queued.

The worker checks the saved job identity, folder, confirmation state and status. It uses the usual startup save if that proof is absent or if it is processing a retained output record.

## Native validation

All 253 streaming and recovery checks passed. The new cases injected a failed catalog save, checked that the live job and durable file stayed intact, and reopened a copy of the interrupted admission record. The reopened offer was paused and inactive. A separate check exercised worker startup against the saved media folder and observed zero additional catalog writes.

All 163 native presentation checks passed. The staged Chromium and Firefox 0.69.3 bundles passed 38 focused checks each.

## Browser comparison

Each timed run selected 720p MP4 with Spanish audio in a fresh Edge profile using the same eight-second loopback HLS source. The source recorded the arrival of the first native segment request. The test verified that the saved catalog already held the confirmation gate and an absolute working folder at that point. It also tracked the first observed received bytes and the time the exact-size label appeared.

The trace counts temporary catalog files flushed between prefetch startup and the first segment request. That count includes any periodic checkpoint that happens in the interval. The native regression isolates the explicit worker startup checkpoint by holding the catalog lock while checking whether the worker's startup logic requests another write.

| Median of three 720p MP4 runs | 5.38 | 5.39 |
|---|---:|---:|
| Click to enabled Start observation | 603 ms | 481 ms |
| Click to first segment request | 1,587 ms | 924 ms |
| Click to first observed received bytes | 1,607 ms | 964 ms |
| Click to exact-size label | 4,564 ms | 3,444 ms |
| Catalog flushes before first segment request | 3 | 2 |

The first segment request ranged from 996 to 2,931 ms in the baseline and from 902 to 1,678 ms in 5.39. The small, sequential sample measures this fixture. The native regression provides separate evidence that the redundant worker checkpoint was removed.

| Edge output | Runs | Exact bytes shown and published |
|---|---:|---:|
| 720p MP4 with Spanish audio | 3 | 6,560,076 |
| 360p TS with English audio | 1 | 1,961,216 |
| M4A with Spanish audio | 1 | 98,791 |

All five 5.39 runs received bytes before Start, left the main window hidden, and matched the exact size and staging SHA-256 after publication. Every output decoded through its full timeline. All temporary native host registrations were removed. Five production executables and both browser bundles are staged. Cold public YouTube timing and a paired installed IDM run remain open. [Build receipt](prefetch-catalog-startup-5.39-build.json)
