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

## Multiple jobs at one origin

`host_concurrency.py` starts three independent downloads with one connection per
job against a loopback origin that accepts two connections. Each file defaults
to 8 MiB at 512 KiB/s. The app stays open until all jobs finish or the timeout
expires. The fixture reports every job, checks the completed files' SHA-256,
and removes its payloads and HTTP partials. It returns a nonzero exit status
when any job fails, remains unfinished or has incorrect bytes.

```powershell
python benchmarks/speed/host_concurrency.py --udm release-native/UDM.exe --work D:/UDM/candidates/host-cap-work --out D:/UDM/candidates/host-cap-results.json
```

Use new work and output paths. `--jobs`, `--server-connections`, `--size-mib`,
`--rate-kib`, `--retries` and `--timeout` adjust the workload. Set
`--lift-cap-after` to a positive number of seconds to remove the origin's cap
while downloads are active and check whether admission restores capacity.
This fixture uses UDM's private
catalog and an instance tag. A paired IDM run still requires the disposable
profile described above.

Include the slower first-payload case when changing admission: use
`--size-mib 2 --rate-kib 128 --lift-cap-after 5` with fresh paths. All three
files must complete, and peak concurrency must grow to three after the cap
is removed. With `--retries 0` and the cap retained, the expected negative
control is two correct completed files and one failed initial open after one
refusal; that run intentionally exits nonzero. These cases distinguish
productive-capacity learning from headers alone and preserve the explicit
zero-retry policy.

Use `--proxy-mode windows` to run the same workload through Windows automatic
proxy selection. The receipt records the chosen mode. This tests the machine's
current configuration without changing its proxy settings; a passing loopback
case does not qualify VPN, authenticated enterprise proxy or WPAD deployment.

`proxy_routes.py --udm <exe> --work <fresh-folder> --out <new-json>` exercises
proxy-only delivery, dead-first-proxy failover, failure without an implicit
direct fallback, and the Windows resolver's explicit bypass for local targets.
The three positive transfers are 1 MiB and are checked by SHA-256. The negative
case requires a failed selected-proxy connection, no payload requests and no
published file. All generated payloads and partials are removed. No Windows
proxy settings are changed.

`proxy_accounts.py --fixture <Udm.HttpAdmissionTests.exe> --work <fresh-folder>
--out <new-json>` drives four downloads through normal native queue dispatch.
The first account has a two-response cap; its three managers independently
encrypt the same password. The fourth download uses another account and must
begin receiving while the first account remains full. All four 2 MiB outputs
must match SHA-256. The native receipt records whether matching credentials
produce one admission identity and whether the third download waits for a
slot. Credentials are fixture-only; neither passwords nor authorization headers
are written to the evidence. Payloads and partials are removed afterwards.
Add `--challenge` to use Automatic authentication and require at least four
407 challenge responses before the four authenticated downloads complete.
Use `--header-timeout` for a negative control: the third request stays silent,
uses a one-second response deadline and one retry, and must fail after exactly
two wire requests without entering admission wait. The other three files must
complete correctly. This option can be combined with `--challenge`.
Use `--pause-waiter --challenge` separately to pause the third job while it waits
for capacity, acknowledge cancellation while peers remain active, resume it
through normal queue dispatch, and verify all four final files. The receipt
records pause acknowledgement time; it is not an IDM latency comparison.
Use `--pause-active --challenge` for the saved-partial handoff case. It pauses
a receiving file after real progress, requires its waiter to begin receiving
within 1.5 seconds of pause acknowledgement, and resumes the saved partial.
The proxy must observe a nonzero resumed range and all four files must match
SHA-256. Account isolation is checked from the proxy's actual occupied-slot
counter when the independent account starts, rather than inferred from end times.

Add `--cap-reduction` to `--pause-active --challenge` to lower that account's
active-response cap from two to one when the paused response closes. The waiter
must remain without payload during the 1.5-second observation interval, then all
four files must complete with exact hashes, including the resumed partial. Every
later acceptance must respect the one-response cap. Add `--cap-restore` to raise
the cap back to two after the first reduced-cap acceptance; an additional job
must then start while that response is still active. Evidence records cap-change
times, accepted concurrency and refused GET attempts. Refusals include libcurl
safe reconnects; they are not one-to-one with logical admission attempts.
These modes leave Windows proxy settings unchanged and remove generated payloads.

## Interrupted form submissions

`post_recovery.py --fixture <Udm.HttpAdmissionTests.exe> --work <fresh-folder>
--out <new-json>` checks direct and authenticated proxy POST recovery, including
empty bodies, lost responses, receive deadlines, explicit resubmission, successful
forms, and safe GET retries. Basic and Digest proxy challenges are controlled
locally. Use `--cases name,name` to select individual cases.

The proxy counts authenticated submissions and compares their body hashes.
An ambiguous POST must not be silently submitted again by the transport or
queue, while an explicit restart may submit again. GET controls must still
recover. Five successful response files across all ten cases are checked by
SHA-256; failed submissions must publish no file. Payloads and partials are
removed. No real forms, credentials or system proxy settings are used.
