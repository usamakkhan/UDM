# HTTP prefix hash shutdown

The background prefix hash previously checked for network completion between batches of up to 16 MiB. It now stops between 1 MiB chunks when the HTTP workers return. A normal stop preserves the hash state so a required digest or the existing post-publication hash can continue from the verified prefix. The trace separates range worker exit, boundary validation, prefix hash shutdown and return from the HTTP phase.

All 74 native transfer checks passed. They included the new finish-event ordering check, exact SHA-256 verification, pause and restart, boundary mismatches, TCP refusals and HTTP 403 recovery.

Three alternating runs per build used 24 MiB and 192 MiB files on the same loopback origin settings as the healthy-range experiments. All 12 outputs matched their expected SHA-256.

| File | 5.80 median boundary validation end to HTTP return | 5.81 median | 5.80 median wall time | 5.81 median wall time |
|---|---:|---:|---:|---:|
| 24 MiB | 31 ms | 0 ms | 2.396 s | 3.344 s |
| 192 MiB | 16 ms | 0 ms | 4.187 s | 4.044 s |

All six 5.81 hash joins finished by the next recorded tick after boundary validation. A slow 24 MiB run still took 6.906 seconds. Its trace separated 469 ms from the last byte to worker exit and 1,062 ms for the subsequent boundary-validation interval, with no measurable hash join. That interval includes file access and scheduling. The measurements leave the cause of the boundary delay and overall speed against IDM open.

The isolated transfer log and paired request traces are under `udm-http-prefix-hash-5.81` in the Windows local temporary folder. [Native build receipt](http-prefix-hash-stop-5.81-build.json)
