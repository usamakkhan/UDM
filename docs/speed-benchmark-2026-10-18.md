# UDM vs IDM 6.43 transfer and recovery benchmark (2026-10-18)

Measured with [benchmarks/speed](../benchmarks/speed/README.md) on this PC
(Windows 11 Pro 26100, UDM 1.0.1 source at the start of this branch, IDM
6.43 build 10). Both managers were driven through their command lines and
downloaded identical files from the same loopback origin or public URL. Each
result is one run; loopback runs repeat to within about half a second.

Metrics: `wall` is from process launch until the output file is complete.
`startup` is launch to the first request at the origin, `transfer` is first
request to the last connection close, `finish` is what is left (assembly,
verification, publication). `MB/s` is file size over wall (decimal). `wasted`
is bytes the origin served beyond the file size. The raw JSON for every run is
under `benchmarks/speed/results/`.

Loopback origin: Python, 384 MiB (256 MiB for `no-range-1` and `slow-start-2`),
8 MB/s cap per connection, 20 ms response latency unless noted. With the cap
the ideal transfer is 6.3 s at 8 connections and 3.1 s at 16. The uncapped
origin tops out near 880 MB/s aggregate.

## Before (UDM 1.0.1)

| case | client | wall s | MB/s | startup s | transfer s | finish s | conns | peak | requests | rejected | drops | wasted MiB | sha | notes |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---|
| single-8 | IDM | 9.3 | 43.3 | 1.2 | 7.5 | 0.7 | 15 | 10 | 15 | 0 | 0 | 1.6 | yes |  |
| single-8 | UDM | 13.6 | 29.6 | 0.8 | 8.8 | 4.1 | 8 | 8 | 9 | 0 | 0 | 0.7 | yes | segments=8 splits=0 |
| single-16 | IDM | 9.1 | 44.4 | 2.4 | 7.5 | -0.8 | 49 | 17 | 49 | 0 | 0 | 5.7 | yes |  |
| single-16 | UDM | 9.8 | 41.0 | 0.7 | 5.3 | 3.9 | 16 | 16 | 17 | 0 | 0 | 0.7 | yes | segments=16 splits=0 |
| uncapped-8 | IDM | 4.8 | 84.0 | 1.8 | 2.1 | 0.9 | 14 | 8 | 14 | 0 | 0 | 4.6 | yes |  |
| uncapped-8 | UDM | 6.7 | 59.8 | 0.9 | 1.7 | 4.2 | 9 | 8 | 13 | 0 | 0 | 3.2 | yes | segments=12 splits=4 |
| resume-8 | UDM | 9.3 | 43.4 | 0.9 | 4.1 | 4.2 | 16 | 8 | 19 | 0 | 0 | 6.7 | yes | segments=9 splits=1 |
| outage-8 | IDM | 16.8 | 23.9 | 3.1 | 14.3 | -0.5 | 32 | 9 | 32 | 1734 | 8 | 2.2 | yes |  |
| outage-8 | UDM | 9.1 | 44.4 | 1.5 | 3.4 | 4.1 | 8 | 8 | 9 | 24 | 8 |  |  | FAILED HTTP send failed (Windows error 12030). segments=8 splits=0 |
| drop-8 | IDM | 11.0 | 36.7 | 2.4 | 8.0 | 0.5 | 64 | 9 | 64 | 0 | 44 | 10.0 | yes |  |
| drop-8 | UDM | 27.1 | 14.9 | 1.9 | 25.1 | 0.1 | 30 | 8 | 31 | 0 | 25 |  |  | FAILED HTTP read failed (Windows error 12030). segments=8 splits=0 |
| no-range-1 | IDM | 4.4 | 61.1 | 3.3 | 1.0 | 0.0 | 4 | 3 | 4 | 0 | 0 | 0.5 | yes |  |
| no-range-1 | UDM | 6.9 | 38.7 | 1.1 | 3.2 | 2.6 | 2 | 2 | 2 | 0 | 0 | 0.2 | yes | segments=1 splits=0 |
| slow-start-2 | IDM | 23.5 | 11.4 | 3.0 | 20.2 | 0.3 | 9 | 2 | 9 | 65 | 0 | 0.8 | yes |  |
| slow-start-2 | UDM | 5.5 | 49.1 | 1.2 | 4.3 | 0.0 | 2 | 2 | 3 | 24 | 0 |  |  | FAILED HTTP send failed (Windows error 12030). segments=8 splits=0 |
| public-alpine-8 | IDM | 63.3 | 4.1 |  |  |  |  | 8 |  |  |  |  |  |  |
| public-alpine-8 | UDM | 161.9 | 1.6 |  |  |  |  | 1 |  |  |  |  |  | segments=9 splits=1 |
| public-kernel-8 | IDM | 40.6 | 3.6 |  |  |  |  | 8 |  |  |  |  |  |  |
| public-kernel-8 | UDM | 61.7 | 2.4 |  |  |  |  | 1 |  |  |  |  |  | segments=9 splits=1 |

The IDM `resume-8` row is omitted: IDM runs as an elevated tray process here and the unelevated harness could not kill it, so that run was never interrupted.

### What the baseline showed

- **Three reliability failures that IDM does not have.** With a server that
  resets every connection after 8 MiB (`drop-8`), UDM failed with "HTTP read
  failed (Windows error 12030)" after 25 resets: every drop consumed one of the
  three retries of its range, and each retry waited 0.5, 1 and 2 s first. With
  a server that accepts only two concurrent connections (`slow-start-2`), UDM
  failed in 5 s: the six refused workers exhausted their retries while the two
  accepted connections were still transferring, and one worker's failure stops
  the whole download. A 5 s outage (`outage-8`) also failed: the three retries
  waited 0.5, 1 and 2 s in total, less than the outage. IDM completed all
  three.
- **Finish phase.** After the last byte UDM spent about 4 s on a 384 MiB file
  (copy the parts into the assembled file, then read it again for SHA-256, then
  publish); IDM is done within a second of its last byte. On the uncapped
  origin this phase was longer than the transfer itself.
- **Per-read overhead.** Sequential transfer (`no-range-1`, one connection,
  uncapped) ran at 84 MB/s against IDM's 268 MB/s: each 16 KiB read was one
  asynchronous WinHTTP round trip plus four lock acquisitions on the manager
  mutex for limiter, quota and progress accounting.
- **Steady state is close.** With 8 capped connections UDM's transfer phase was
  8.8 s against IDM's 7.5 s; with 16 it was 5.3 s and UDM scaled while IDM did
  not (IDM 16 connections: 7.5 s). UDM's startup (0.7 to 0.9 s) is faster than
  IDM's (1.2 to 2.4 s), and it wastes fewer bytes (0.7 MiB against 1.6 to
  5.7 MiB) because IDM opens many short tail requests.
- **Resume after a kill** works: UDM resumed its 8 ranges from the part files
  with 6.7 MiB of in-flight data lost. The IDM run of this case is invalid:
  IDM runs as an elevated tray process on this machine and cannot be killed
  from the unelevated harness, so it was never interrupted (the harness
  rejects that result). The `outage-8` case (every connection reset and new
  ones refused for 5 s) measures recovery for both.

## Changes made to native/Transfer.cpp

1. **Read buffer 16 KiB to 256 KiB** (heap allocated per attempt). Fewer
   WinHTTP round trips and fewer mutex acquisitions per megabyte.
2. **Reconnection pacing.** A connection that broke after delivering bytes is
   reopened immediately instead of after an exponential wait, and after at
   least 1 MiB of progress it restores the retry budget, so a server that
   resets every connection after N MB no longer fails the download after
   N x Retries MB. Retries=0 still means no reconnection at all. Consecutive
   failures without any byte keep the retry budget but wait 1, 2, 4, 8 then
   15 s (previously 0.5, 1, 2, 4, 8 then 10 s), so the default three retries
   ride out a 5 s outage.
3. **Refused connections give their range back.** A worker whose connection
   fails before any byte while another worker is receiving releases its range
   to the pool (`owners[index] = -1`) and backs off; once its budget is spent
   it pauses for 2, 4, 8, 16 then 30 s between attempts instead of failing the
   download. Workers that run out of ranges stay available while other workers
   still own ranges, so a released range is always picked up.
4. **Assembly hashes while it copies.** The SHA-256 is computed from the bytes
   written to the assembled file (1 MiB copy buffer) instead of reading the
   file a second time.

Native checks added to `TransferChecks.hpp`, with new `dropEvery` and
`maxConnections` modes in `TransferFixture.hpp`: a server that resets every
connection after 2 MiB completes with the exact hash and without the backoff
wait; Retries=0 still rejects such a download and publishes nothing; a
two-connection server limit completes a four-worker download with the exact
hash while refusing connections.

## After

| case | client | wall s | MB/s | startup s | transfer s | finish s | conns | peak | requests | rejected | drops | wasted MiB | sha | notes |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---|
| single-8 | IDM | 10.2 | 39.5 | 2.0 | 7.3 | 0.9 | 15 | 9 | 15 | 0 | 0 | 1.5 | yes |  |
| single-8 | UDM | 11.0 | 36.5 | 1.0 | 7.2 | 2.8 | 8 | 8 | 10 | 0 | 0 | 0.5 | yes | segments=9 splits=1 |
| single-16 | IDM | 6.9 | 58.8 | 0.9 | 5.4 | 0.6 | 25 | 18 | 25 | 0 | 0 | 5.3 | yes |  |
| single-16 | UDM | 7.7 | 52.5 | 0.7 | 4.3 | 2.7 | 16 | 16 | 18 | 0 | 0 | 0.7 | yes | segments=17 splits=1 |
| uncapped-8 | IDM | 2.6 | 153.6 | 0.9 | 1.2 | 0.6 | 7 | 4 | 7 | 0 | 0 | 2.5 | yes |  |
| uncapped-8 | UDM | 5.1 | 79.7 | 0.7 | 1.8 | 2.6 | 8 | 8 | 9 | 0 | 0 | 0.7 | yes | segments=8 splits=0 |
| resume-8 | IDM | 8.6 | 46.7 | 1.0 | 7.2 | 0.5 | 23 | 10 | 23 | 0 | 0 | 216.8 | yes |  |
| resume-8 | UDM | 11.2 | 36.0 | 0.7 | 6.7 | 3.8 | 16 | 8 | 18 | 0 | 0 | 2.0 | yes | segments=8 splits=0 |
| outage-8 | IDM | 14.1 | 28.6 | 1.2 | 12.5 | 0.4 | 22 | 10 | 22 | 1974 | 8 | 2.6 | yes |  |
| outage-8 | UDM | 19.5 | 20.6 | 0.7 | 15.7 | 3.2 | 16 | 8 | 17 | 24 | 8 | 0.0 | yes | segments=8 splits=0 |
| drop-8 | IDM | 9.6 | 41.8 | 1.1 | 8.1 | 0.5 | 59 | 9 | 59 | 0 | 44 | 12.8 | yes |  |
| drop-8 | UDM | 11.5 | 35.0 | 0.7 | 7.7 | 3.1 | 56 | 8 | 58 | 0 | 48 | 12.4 | yes | segments=9 splits=1 |
| no-range-1 | IDM | 1.8 | 148.0 | 0.9 | 0.8 | 0.1 | 4 | 3 | 4 | 0 | 0 | 0.5 | yes |  |
| no-range-1 | UDM | 4.5 | 59.8 | 0.7 | 1.5 | 2.2 | 2 | 2 | 2 | 0 | 0 | 0.2 | yes | segments=1 splits=0 |
| slow-start-2 | IDM | 21.3 | 12.6 | 1.0 | 20.0 | 0.3 | 9 | 2 | 9 | 30 | 0 | 0.6 | yes |  |
| slow-start-2 | UDM | 21.9 | 12.3 | 0.7 | 19.3 | 1.9 | 8 | 2 | 17 | 40 | 0 | 4.2 | yes | segments=16 splits=8 |
| public-alpine-8 | IDM | 64.8 | 4.0 |  |  |  |  | 8 |  |  |  |  |  |  |
| public-alpine-8 | UDM | 64.7 | 4.0 |  |  |  |  | 8 |  |  |  |  |  | segments=18 splits=10 |
| public-kernel-8 | IDM | 40.1 | 3.7 |  |  |  |  | 8 |  |  |  |  |  |  |
| public-kernel-8 | UDM | 50.7 | 2.9 |  |  |  |  | 9 |  |  |  |  |  | segments=24 splits=16 |

### What changed

| case | UDM before | UDM after | IDM (after run) |
|---|---:|---:|---:|
| `drop-8` (reset every 8 MiB) | failed after 27 s | 11.5 s, 48 resets survived | 9.6 s |
| `slow-start-2` (2 connections accepted) | failed after 5.5 s | 21.9 s | 21.3 s |
| `outage-8` (5 s blackout) | failed after 9.1 s | 19.5 s | 14.1 s |
| `public-alpine-8` (257 MB, CDN) | 161.9 s, 1 TCP connection | 64.7 s, 8 connections | 64.8 s |
| `public-kernel-8` (148 MB) | 61.7 s, 1 TCP connection | 50.7 s, 8 connections | 40.1 s |
| `no-range-1` transfer phase | 3.2 s (84 MB/s) | 1.5 s (179 MB/s) | 0.8 s |
| `single-8` transfer phase | 8.8 s | 7.2 s | 7.3 s |
| `single-16` transfer phase | 5.3 s | 4.3 s | 5.4 s |
| `uncapped-8` transfer phase | 1.7 s | 1.8 s | 1.2 s |
| finish phase, 384 MiB | 3.9 to 4.2 s | 2.6 to 3.8 s | 0.4 to 0.9 s |

- The three reliability failures are gone: every loopback case completes with
  the expected SHA-256. In `slow-start-2` the six refused workers released their
  ranges and the two accepted connections finished the file (17 requests, 8
  splits); in `drop-8` 48 resets were followed by immediate reconnections.
- On the public HTTPS origins UDM had been multiplexing all eight ranges over a
  single HTTP/2 connection (`tcp.peak_concurrent` = 1), so one TCP flow carried
  the whole download at 1.6 MB/s while IDM's eight HTTP/1.1 connections
  reached 4.1 MB/s. Parallel ranges now use one HTTP/1.1 connection each
  (`DisableHttp2` per worker request when more than one worker runs) and the
  Alpine case is at parity with IDM (64.7 s against 64.8 s); the kernel.org
  run was 50.7 s against 40.1 s in this pass and 38.4 s against 38.8 s in the
  previous pass of the same binary, which is the link's variance. The probe
  and single-connection downloads keep HTTP/2.
- Transfer phases now match IDM's on the capped origin (7.2 s against 7.3 s at
  8 connections, 4.3 s against 5.4 s at 16), and the sequential path is twice
  as fast as before, though IDM's single stream is still faster (0.8 s).
- The outage case recovers, but 3.2 s later than IDM: after a connection with
  progress breaks, the no-byte reconnect attempts wait 1, 2 and 4 s, and the
  blackout ended during the 4 s wait. IDM attempted 1,974 connections during
  the 5 s blackout; UDM 24.
- The native suite passes: 2,785 checks, 0 failed, including the three new
  transfer checks.

## Remaining gaps and notes

- **Finish phase (assembly).** Still 2.6 to 3.8 s on a 384 MiB file (it was
  7.6 s in one earlier pass on the uncapped origin), against 0.4 to 0.9 s for
  IDM; on the uncapped origin it is longer than the transfer itself and is the
  whole 5.1 s against 2.6 s difference. Fusing the hash into the copy removed one read
  pass, but the phase is bound by writing the 384 MiB a second time and
  flushing it before publication, while the part files' own writes are still
  being flushed on the uncapped origin. The remaining fix is structural:
  write every range directly into one preallocated file (the part layout,
  resume checks and recovery fixtures all assume one file per range), which
  would make the finish phase a rename plus a hash pass. This is now the
  largest gap on fast links and the next piece of work.
- **Wall time on the capped origin** is still 1 to 2 s behind IDM for that
  reason only; transfer phases match.
- **Outage recovery** is 3.2 s slower than IDM (see above). Reconnecting
  without a byte could probe more often while another worker was recently
  healthy, at the cost of more connection attempts.
- **IDM observations worth noting:** IDM at 16 connections was not faster than
  at 8 on the capped origin in one of two runs (9.1 s vs 9.3 s; 7.1 s in the
  after run); it wastes 2 to 13 MiB per download on short tail requests; and
  when its process was killed mid-transfer in the after runs it re-downloaded
  about 217 MiB of a 384 MiB file instead of resuming (two observations; the
  before-run kill did not take effect). UDM lost 2 to 7 MiB of in-flight data.
- **Public origins** vary with the shared link (3 to 4 MB/s here); the public
  rows are single runs, taken back to back, and establish parity, not a
  ranking.

## Reproducing

```powershell
.\native\build.ps1 -ToolchainRoot D:\UDM\.media-cache\driver-toolchain
python benchmarks\speed\bench.py --udm release-native\UDM.exe --work $env:TEMP\udm-speed-bench --out benchmarks\speed\results\<label>
python benchmarks\speed\report.py benchmarks\speed\results\<label>
```

The harness writes IDM's connection count to
`HKCU\Software\DownloadManager\MaxConnectionsNumber` for its runs and restores
the previous value. IDM was running as an elevated process during these runs,
so the IDM command lines were forwarded to that instance.
