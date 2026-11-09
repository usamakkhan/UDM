# HTTP prefix hash reader

The HTTP prefix hasher now reuses one staging-file handle and one read buffer per transfer attempt. It snapshots committed ranges only when it can acquire the manager lock immediately, then sorts and hashes outside the lock. When the range workers finish, a condition-variable signal wakes an idle hasher. A read or hash already in progress can finish its current chunk. The worker closes its handle before range repair or publication, and verified hash progress remains available to the existing checksum and publication paths.

All 77 native HTTP transfer checks passed. They include exact checksums, pause and resume, overlap mismatch recovery, and server connection refusals.

Three alternating loopback trials per size and version compared 5.83 and 5.84. All 12 outputs matched the expected SHA-256.

| File | Median hash shutdown, 5.83 / 5.84 | Median wall time, 5.83 / 5.84 |
|---|---:|---:|
| 24 MiB | 0 / 0 ms | 1.933 / 2.158 s |
| 192 MiB | 46 / 0 ms | 4.834 / 5.484 s |

The longest measured hash shutdown was 375 ms on 5.83 and 31 ms on 5.84. Five of six 5.84 shutdowns completed within the same recorded tick as the boundary check. Total download times varied and the median was slower in this sample at both sizes. The timing comparison used two UDM builds. Direct IDM timing remains open.

The fixture log and request traces are in the Windows local temporary folder under `udm-http-prefix-reader-5.84`.

[Production build receipt](http-prefix-reader-5.84-build.json)
