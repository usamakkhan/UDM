# UDM vs IDM speed and recovery benchmark

`bench.py` drives UDM (`UDM.exe /d /p /f /n /q`, resume through `/s`) and the
installed Internet Download Manager (`IDMan.exe /d /p /f /n /q`) through
identical downloads and records wall time, throughput, connection counts,
request/reconnect behaviour and (for UDM) the segment plan.

`origin.py` is the loopback HTTP/1.1 origin. It synthesizes a deterministic
file (no disk use) and offers the knobs the cases need: a per-connection
bandwidth cap, response latency, dropping a connection after every N bytes,
ignoring `Range`, and a concurrent-connection cap. Every TCP connection and
request is logged.

## Cases

| case | origin | client setting | what it shows |
|---|---|---|---|
| `single-8` | 384 MiB, 8 MB/s per connection, 20 ms latency | 8 connections | steady multi-connection throughput (ideal 64 MB/s) |
| `single-16` | same | 16 connections | scaling with more connections (ideal 128 MB/s) |
| `uncapped-8` | 384 MiB, no cap | 8 connections | engine overhead; the Python origin tops out near 880 MB/s |
| `resume-8` | as `single-8`; client killed at 45 % served | 8 connections | restart after a crash: resumed or restarted, wasted bytes |
| `outage-8` | as `single-8`; at 45 % served every connection is reset and new ones refused for 5 s | 8 connections | recovery after a network outage (measurable for IDM, unlike `resume-8`) |
| `drop-8` | as `single-8`; each connection reset after 8 MiB | 8 connections | reconnection pacing and whether the download survives |
| `no-range-1` | 256 MiB, `Range` ignored, no cap | 8 requested | sequential path throughput and correctness |
| `range-ignored-16m` | 16 MiB, 4 MiB/s per connection, no range header, range requests return 200 | 8 requested | the cost of one unsuccessful range probe |
| `range-none-16m` | same, with `Accept-Ranges: none` | 8 requested | explicit server opt-out |
| `range-unadvertised-16m` | same, with valid 206 replies and no range header | 8 requested | the benefit of testing for unadvertised range support |
| `slow-start-2` | 256 MiB, only 2 concurrent connections accepted | 8 requested | behaviour when the server refuses extra connections |
| `small-256k` | 256 KiB, uncapped | 8 requested | launch and publication overhead on a tiny download |
| `mixed-speed-32m` | 32 MiB, 4 MiB/s per connection except every third at 128 KiB/s, paced in 16 KiB writes | 8 requested | whether faster workers take over a slow worker's remaining bytes |
| `alternating-drop-50m` | 50 MiB, 4 MiB/s per connection, every second connection drops after 2 MiB | 8 requested | retry and reassignment while healthy workers continue |
| `public-alpine-8` | Alpine 3.21.3 standard ISO (257 MB, CDN) | 8 connections | real network, read-only |
| `public-kernel-8` | linux-6.12.tar.xz from cdn.kernel.org (148 MB) | 8 connections | real network, read-only |

## Running

```powershell
python benchmarks/speed/bench.py --udm release-native/UDM.exe --work $env:TEMP/udm-speed-bench --out benchmarks/speed/results/<label>
```

Options: `--cases a,b`, `--clients udm,idm`, `--repeat n`, `--timeout s`,
`--no-idm-registry`. The work folder holds UDM's private `--data-dir` (instance
tag starting with `speed-bench-`) and the download folder. Use a fresh work
directory outside the results directory. Every case deletes its output after
hashing it, so at most one benchmark file plus UDM's output file exist at a time
(under 1 GB for the 384 MiB cases).

IDM's connection count is set through `HKCU\Software\DownloadManager\MaxConnectionsNumber` (and the older `ConnectionsNumber`)
before each IDM run and the original value is restored when the harness exits.
Use a disposable Windows profile for paired runs. The harness refuses to start
IDM cases while an IDM instance is already running. Use `--clients udm` for a
UDM-only baseline while IDM is open. Repeated trials write separate
`-r01`, `-r02`, and subsequent receipts. A failed transfer or hash mismatch
returns a nonzero exit code after recording the results.

## Output

One JSON per case and client in `--out`, plus `summary.md`. Fields:

- `wall_seconds`: from process launch until the output file is complete (hashed afterwards). For `resume-8`, `resume_wall_seconds` counts from the relaunch.
- `throughput_MBps`: file size / wall (decimal megabytes).
- `origin.connections`, `requests`, `rejected_connections`, `drops`, `peak_concurrent`, `wasted_bytes` (bytes served beyond the file size).
- `origin.range_requests` and `ignored_range_requests` count outgoing requests carrying Range and those that receive HTTP 200.
- `origin.launch_to_headers_seconds`, `launch_to_body_seconds`, and `launch_to_full_get_body_seconds` measure dispatch to the first observed file response headers, first body chunk, and first body chunk of an ordinary HTTP 200 GET. They describe sends at the origin. The last value is empty for a run that uses only ranged responses.
- `origin.body_window_seconds` spans the first through last observed body writes, including stalls and retries. `origin.last_body_to_completion_seconds` measures from that last write to observed completion. It includes network delivery, client finalization, and polling latency. `completion_poll_seconds` records the configured sleep between completion checks (10 ms for `small-256k`, 200 ms otherwise).
- `origin.request_log` records each method, range header, status, bytes sent, outcome, and origin-relative timestamps for request receipt, headers, first and last body writes, and finish. Timestamps are empty when the event was unobserved. `origin.connection_log` records the rate and byte cutoff assigned to each socket. Faults are selected by one-based TCP accept order, including retries.
- `client_details` (UDM only): segment count, dynamic splits, `TransferSeconds`, and any error message from the catalog.
- `tcp.peak_concurrent` (public cases): established TCP connections sampled with `netstat` once per second.

The results are in `docs/speed-benchmark-2026-10-18.md`.
