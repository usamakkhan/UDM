# HTTP boundary checks from write buffers

UDM retains the final overlap bytes of a range when a successful write in the current attempt covers the entire overlap. The finish check compares their SHA-256 with the digest recorded from the neighboring response. A saved range or a write that covers only part of the overlap still requires a staging-file read. The cached bytes are scoped to a single attempt. Mismatch recovery and final file verification use the existing paths.

All 77 native transfer checks passed. The suite exercised buffered overlaps, saved-byte disk fallback, a mismatch that forced a clean single-connection retry, sparse recovery, pause and restart, and HTTP 403 recovery. An initial run showed that two new assertions assumed a fixed number of healthy splits. The final assertions measure which source supplies the overlap and isolate the disk-fallback case on one connection.

Three alternating loopback trials per build and size used the same origin settings for 5.82 and 5.83. All 12 completed files matched the expected SHA-256. In six fresh 5.83 runs, all 55 overlap checks used write buffers. The six 5.82 runs had 56 overlap checks, derived from the saved range digests that 5.82 reads from disk.

| File | Median boundary check, 5.82 / 5.83 | Median worker exit to HTTP return, 5.82 / 5.83 | Median wall time, 5.82 / 5.83 |
|---|---:|---:|---:|
| 24 MiB | 109 / 0 ms | 109 / 32 ms | 3.037 / 3.567 s |
| 192 MiB | 141 / 0 ms | 141 / 94 ms | 7.324 / 10.211 s |

The measurements show less time after the HTTP workers exited. The prefix hash shutdown accounted for most of the remaining finish interval. End-to-end results varied by trial, with slower medians for 5.83 in this sample. These are paired UDM measurements. Direct IDM timing remains open.

The fixture log and request traces are in the Windows local temporary folder under `udm-http-boundary-buffers-5.83`. [Production build receipt](http-boundary-buffers-5.83-build.json)
