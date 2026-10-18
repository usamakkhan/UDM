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
| `slow-start-2` | 256 MiB, only 2 concurrent connections accepted | 8 requested | behaviour when the server refuses extra connections |
| `public-alpine-8` | Alpine 3.21.3 standard ISO (257 MB, CDN) | 8 connections | real network, read-only |
| `public-kernel-8` | linux-6.12.tar.xz from cdn.kernel.org (148 MB) | 8 connections | real network, read-only |

## Running

```powershell
python benchmarks/speed/bench.py --udm release-native/UDM.exe --work $env:TEMP/udm-speed-bench --out benchmarks/speed/results/<label>
```

Options: `--cases a,b`, `--clients udm,idm`, `--repeat n`, `--timeout s`,
`--no-idm-registry`. The work folder holds UDM's private `--data-dir` (instance
tag `speed-bench`) and the download folder; every case deletes its output after
hashing it, so at most one benchmark file plus UDM's output file exist at a time
(under 1 GB for the 384 MiB cases).

IDM's connection count is set through `HKCU\Software\DownloadManager\MaxConnectionsNumber` (and the older `ConnectionsNumber`)
before each IDM run and the original value is restored when the harness exits.
IDM must not be running when the harness starts (it is closed if it is).

## Output

One JSON per case and client in `--out`, plus `summary.md`. Fields:

- `wall_seconds`: from process launch until the output file is complete (hashed afterwards). For `resume-8`, `resume_wall_seconds` counts from the relaunch.
- `throughput_MBps`: file size / wall (decimal megabytes).
- `origin.connections`, `requests`, `rejected_connections`, `drops`, `peak_concurrent`, `wasted_bytes` (bytes served beyond the file size).
- `origin.request_log`: each request's range, status, bytes sent and outcome (`complete`, `dropped`, `416`, …).
- `client_details` (UDM only): segment count, dynamic splits, `TransferSeconds`, and any error message from the catalog.
- `tcp.peak_concurrent` (public cases): established TCP connections sampled with `netstat` once per second.

The results are in `docs/speed-benchmark-2026-10-18.md`.
