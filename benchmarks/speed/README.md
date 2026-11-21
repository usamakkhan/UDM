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


## Finishing IO attribution

`finish-io.wprp` is an optional Windows Performance Recorder profile for attributing final file/catalog flush waits. The current prepared memory variant uses 64 KiB × 1,024 system buffers (64 MiB), file/disk IO and process/thread scheduling events, and DiskReadInit/DiskWriteInit/DiskFlushInit/FileFlush/FileOpEnd stacks. A real earlier 16 MiB capture exported successfully but retained no identifiable UDM PID events and contained circular-overwrite metadata. Its zero ordinary lost-event count was insufficient to establish coverage. The original profile is [archived](../../docs/validation/finishing-8.34/trace-profile-initial-16m.wprp). The larger profile passes parser validation; its live collection remains unverified.

Read-only validation:

```powershell
wpr -profiles .\benchmarks\speed\finish-io.wprp
wpr -profiledetails '.\benchmarks\speed\finish-io.wprp!UDMFinish.Verbose'
```

`finish_io_trace.py` now supplies `plan`, `run` and separate administrator `recorder` roles. The controller refuses an elevated token before launching its benchmark. It creates a fresh private request, prints the separately elevated recorder command, and waits for a live recorder pinned by PID **and creation time**. The recorder pins that controller, preflights visible WPR/kernel sessions, starts only its random named instance, and collects one paired case via `compare-completion.py --case ... --trials 1`. No UAC request is launched automatically by this script. Use same-user elevation for the recorder; alternate administrator-account path handling is not verified.

The configured 20–45 second budget reserves 15 seconds for stop/cancel verification. Stop confirmation uses remaining recording time with twelve seconds reserved for cancellation/reaping/probes, rather than the five-second fixed limit that interrupted the first real export. If the controller exits, requests completion, or reaches the deadline, the recorder stops its own instance. ETL merge may continue after physical collection has stopped. An overrun, failed export or unverified stop is a failure, not a successful capture. A live supervisor remains and retries only its named cancellation until absence is confirmed; its command log is bounded. Forced termination of that supervisor or an OS failure is outside this tested protection. Never infer liveness from a leftover JSON/lock file alone.

Read-only example (substitute the verified executable path and SHA; `plan` creates no request directory):

```powershell
python .\benchmarks\speed\finish_io_trace.py plan --output-root "$env:LOCALAPPDATA\UDM-Development\fresh-finish-capture" --udm 'C:\path\to\qualified\UDM.exe' --expected-udm-sha256 '<qualified-SHA256>' --label 'qualified-io-trace' --case split-64m
```

For a live pass use `run` with a fresh output root, then execute its printed recorder command in an administrator terminal while leaving the controller unelevated. The pre-start approval wait is 300 seconds and has no active collector; it does not extend the recording cap. A delayed approval cannot reuse a dead controller identity. No download process is killed to fit the trace budget. Recordings that end before the whole paired workload are incomplete. Pending/failed cleanup retains its exact instance identity for inspection; no unqualified global cancel is issued.

`--workload-output-root` optionally places the normal-token download fixture on D: while recorder files remain in the private development root on C:. Only fresh children of the project cache/private development roots are allowed. `--idm-first` stages a hash-bound private helper with UDM last to preserve its recent events. The per-transfer body and entire main function except client ordering were AST-checked against the unchanged qualified comparison helper. This is attribution-only ordering, never a replacement performance protocol or a way to discard baseline losses.

Keep raw ETL private, correlate known download PIDs/paths, reject incomplete/dropped IO pairs and export a sanitized summary before deleting the exact owned ETL. Check circular overwrite, schema warnings and actual owned-process/operation coverage, not just the lost-event counter. Stop/rundown metadata can exceed configured buffer size. `recorder-result.json` and `controller-result.json` distinguish export, collection-stop and workload outcomes. A 23.6-second privileged collection completed and stopped safely; its event coverage failed validation, so IO attribution remains **unverified**. Raw exports and payloads were deleted after decoding; [sanitized outcomes](../../docs/validation/finishing-8.34/io-trace-attempts.json) remain.

Twenty-one lifecycle/role/profile checks passed, including slow-stop/deadline cases and real owned child-handle/creation-time checks. The read-only D: plan passed, created no directories, and three unsafe/existing workload paths were refused. These are tooling checks, separate from native qualification. A separate earlier real `--case single-4m` smoke completed exactly two files with matching hashes and restored settings; its payloads were deleted. That smoke does not replace a losing qualification trial. The benchmark helper was a frozen measurement input: its optional-filter amendment is explicitly recorded, and exact reconstruction of the original helper hash proves no other loop-body changes. The original default case triples/order and trial count remain, with all four selections checked. Current qualified native app/core binaries are unchanged by these tracing-tool amendments.

```powershell
python .\benchmarks\speed\test_finish_io_trace.py
```

See [the 8.33 findings and separate portable/installed status](../../docs/finishing-performance-8.33.md) and [Microsoft's WPR command-line reference](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/wpr-command-line-options).

### Compact, read-only ETL consumer

`finish_etl_reader.cpp` uses Windows `OpenTrace`/`ProcessTrace` and TDH directly, avoiding an intermediate system-wide XML dump. It never starts or stops a logger and refuses an elevated token for actual file consumption. Output must be a fresh file inside the private `UDM-Development` root. Input is limited to 256 MiB, metadata and operation collections are bounded, and the final JSON is capped at 32 MiB. Related disk fields remain raw; the reader does not invent time-unit conversions or parent-IRP relationships.

For the declared fresh ordinary split scenario, structural coverage requires one matching process start/end, decoded circular-history metadata preceding the target interval, no lost-event/buffer indicators or selected schema errors, unambiguous complete IO pairs, contemporaneous owned file identities and exactly two successful `.data` flush pairs. Missing lifecycle events, unknown paths, overwrite and capacity exhaustion fail closed. Image matching is a basename check; the caller must supply PID provenance and the qualified executable identity. `structuralCoveragePassed` is a necessary evidence gate, **not** a root-cause verdict.

Build using the same MSVC/Windows SDK environment as the native build, with the project `native` include directory and `tdh.lib`, `advapi32.lib`, `shell32.lib`. For example, from a configured compiler prompt at the repository root:

```powershell
$etlToolRoot=Join-Path $env:LOCALAPPDATA 'UDM-Development\finish-etl-reader'
New-Item -ItemType Directory -Path $etlToolRoot -Force | Out-Null
cl /nologo /std:c++17 /EHsc /MT /O2 /utf-8 /DNOMINMAX /DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0A00 /I native benchmarks\speed\finish_etl_reader.cpp ("/Fo"+(Join-Path $etlToolRoot 'finish_etl_reader.obj')) ("/Fe"+(Join-Path $etlToolRoot 'finish_etl_reader.exe')) /link tdh.lib advapi32.lib shell32.lib /DYNAMICBASE /NXCOMPAT /HIGHENTROPYVA /INCREMENTAL:NO /MANIFEST:NO
& "$etlToolRoot\finish_etl_reader.exe" --self-test
```

Actual invocation is `reader.exe <absolute.etl> <fresh-private-output.json> <recorded-UDM-PID> <owned-workload-root> [expected-image-basename]`. Exit 0 means the structural gate passed, exit 1 means the trace was consumed but coverage failed, and exit 2 means an input/API/report error. Even exit 0 leaves `rootCauseVerified=false`; an analyst must correlate the relevant operations before making a causal claim.

The separate `finish_etl_fixture.cpp` is a tiny process-local TraceLogging test producer. It enables only its own user provider, requests no elevation and stops only its successfully created session. It returns 77 if the OS denies creation; do not change privileges or request UAC merely for that fixture. On this PC its normal-token fixture emitted three events plus two metadata records. The native consumer decoded five records, matching Windows `tracerpt`, then correctly rejected the absence of kernel lifetime/flush coverage.

Twenty normalized-event fault checks and five actual CLI refusal/protection checks passed. These validate coverage logic, file consumption and TraceLogging metadata; **kernel Process/FileIo/DiskIo mapping remains unverified until tested on a valid owned kernel ETL**. The fixture ETL, reference CSV, producer binary and compiler objects were individually removed. The ready consumer and compact reports remain. [Validation and exact tool hashes](../../docs/validation/finishing-8.34/etl-reader-validation.json) · [Cleanup](../../docs/validation/finishing-8.34/etl-reader-cleanup.json)

API references: [Windows TDH consumer flow](https://learn.microsoft.com/en-us/windows/win32/etw/retrieving-event-data-using-tdh), [event-record callback](https://learn.microsoft.com/en-us/windows/win32/api/evntrace/nc-evntrace-pevent_record_callback), [private logger sessions](https://learn.microsoft.com/en-us/windows/win32/etw/configuring-and-starting-a-private-logger-session).

### Experimental user-mode IO observer

`finish_user_io_agent.js` is diagnostic tooling, not product code. It filters paths by a unique workload token, records Win32 opens/writes/mappings/closes and native writes/flushes, and resolves live native handles. It never replaces arguments or return values. STATUS_PENDING is recorded as submission rather than completed bytes. ReadFile interception is deliberately excluded because unrelated blocking pipe reads can prevent prompt unload.

The x64 256 KiB fixture captured all eleven expected events, including one complete native write and two native flushes, and retained matching bytes/hash. An actual x86 IDM attempt timed out without settled output. Its writer path remains unverified; do not treat this observer as qualified for a performance comparison. The current private controller also needs timeout-path snapshot persistence before another attempt. [Evidence, failure and cleanup](../../docs/validation/finishing-8.39/native-observer-followup.json).


The later 8.42 owned 64 MiB sequential follow-up streamed observer events because a process-exit hook did not deliver its final snapshot. Streamed observations are explicitly incomplete coverage; native/Win32 flush pairs are counted once. A separate bounded native-marker diagnostic retained its trace through normal exit and localized the dominant measured waits to the body/intent/catalog persistence paths, with a sub-millisecond Finished-to-close handoff. Neither result is an uninstrumented IDM comparison or a kernel-cause attribution. No production source/version change was made. Diagnostic binaries/objects and all three final payloads were removed. [Results and limitations](../../docs/finishing-io-research-8.33.md#842-targeted-d-64-mib-sequential-observations).


## Journal discovery 8.51

The [8.50 control](recovery_discovery_control.cpp) holds the actual journal-read loop in ordinary, replacement and original-scope recovery; all three block model access. The [8.51 qualification](../../docs/recovery-discovery-8.51.md) verifies detached discovery and same-object claim, including actual synchronization retries. This is a recovery correctness/responsiveness qualification, not an IDM speed comparison.


## Accepted recovery cleanup 8.52

The later [8.53 receipt-pruning control](recovery_pruning_control.cpp) links qualified 8.53 core objects and intentionally requires Model to be unavailable at a held, real catalog flush during unscoped receipt-only pruning. Run it with one fresh absolute owned output directory. It exits nonzero if that baseline changes, if its hold expires, if completed bytes change or if pruning is not durable. It is a diagnostic baseline, not a production backend test or a natural-latency benchmark. Reconstruct the matching 8.53 headers before rebuilding this control against old objects; the [8.54 source delta](experiments/recovery-pruning-8.53-to-8.54.json) records the exact source hashes and reverse reconstruction. [C: and D: baseline results](../../docs/finishing-research-8.53.md#receipt-only-pruning-verified-responsiveness-baseline).

The [8.51 control](recovery_cleanup_control.cpp) holds the actual owned journal-retirement boundary in ordinary, replacement and original-scope recovery; all three block model access. The [8.52 qualification](../../docs/recovery-cleanup-8.52.md) verifies detached accepted cleanup and full operation/source reservations, including real read-only retirement failure and hard-link retained cleanup. This is recovery correctness/responsiveness evidence, not an IDM elapsed-time comparison.


## Properties UI dispatch 8.53

The [paused 8.52 control](properties_save_ui_control.cpp) holds an actual catalog flush for 500 ms: its OK handler executes IO on the UI thread and pumps no heartbeats during the hold. Use its `paused` mode with the reconstructed 8.52 source/core; its completed fixture was invalid and is not a qualified control. The [8.53 qualification](../../docs/properties-save-8.53.md) tests the actual dialog in paused/completed success and failed-save retry cases, including reentrant Close/OK during preparation. Injected hold durations are not natural latency or IDM comparisons.


## Receipt retirement 8.54

The [8.54 transaction](../../docs/recovery-pruning-8.54.md) passes 267 pruning checks on each drive and 22,769 selected checks in 21 final-binary-matched runs. Actual journal checks and catalog persistence permit model snapshots while affected destinations and journal names stay reserved. Real publication tests cover same-identifier/different-destination refusal and unrelated success. Snapshot-bound absence proof and successor-safe rollback are separate from row-completion proof. This is a recovery responsiveness/correctness increment; original finishing-speed acceptance remains open.

## Filename and catalog constructor control (8.57)

`filename_startup_control.cpp` is a console entry linked against the native core objects and resources for the engine under investigation. It takes an existing synthetic catalog directory and a fresh report path:

```powershell
.\filename_startup_control.exe C:\owned-fixture\catalog C:\owned-fixture\report.json
```

Use an owned catalog: the controller performs normal save/shutdown after its measured intervals. Records use valid 32-digit hexadecimal IDs, `FileName` equal to `history-<index>.bin`, a 2,048-character `x` Description, and Complete status. The controller first measures the real Manager constructor in Desktop recovery mode, then a deterministic 7,500-name normalization batch, recording current-thread cycles and wall time separately. It validates loaded fields and persisted count. Compare normalized arrays exactly between engines before retaining compact hashes. Reseed identical input bytes before each process and balance A/B order. GUI startup and end-to-end/background timing are outside these boundaries. [8.57 measurements and limitations](../../docs/filename-startup-8.57.md).


## Native observer scope/lifecycle follow-up

`finish_user_io_agent.js` now limits native path queries to disk handles and retains only owned-path samples. The 8 KiB `finish_observer_fixture.cpp` verifies owned writes/flushes, an unrelated file and a pipe on both C: and D:. An early-unload shutdown failure is retained; the qualified short-process sequence captures a snapshot, stops hooks, releases the target and retains the script until normal process exit. It does not qualify early unloading in a long-running personal app. See [actual controls, scope correction and limitations](../../docs/native-io-observer-8.58.md). These tool changes do not increment or rebuild the UDM product.


## Backend-destruction observation control (8.58)

`finish_backend_observer_control.cpp` links the qualified core objects, requires an existing `.udm-backend-observer` marker, a fresh catalog and a loopback URL, and refuses an administrator token. Its first stdin newline starts normal resume/tick processing; after stop/Manager destruction it emits DONE and waits for a second newline so an observer can capture before ordinary process exit. The report write and handshake are outside its recorded backend endpoint. This is not the production GUI entry point or a replacement for an installed application lifetime comparison. [Real observations, losing experiments and cleanup](../../docs/finishing-flush-attribution-8.58.md).


## Standalone ordered range-buffer prototype

`ordered_range_buffer.hpp` and `ordered_range_buffer_tests.cpp` implement an isolated accepted-versus-written range component. The final standalone binary passes 44 checks on each drive, including real ordered output and paused-tail readback. It is not included by native product sources and has a separate experimental budget: production activation first requires shared buffer ownership, generation-bound catalog overlays and cancellation/publication integration. The class does not perform the final durability flush. See [implementation contract, evidence and ordered integration gates](../../docs/ordered-range-write-plan.md). Product versions remain 8.59 / 0.72.23 for this prototype-only work.


## Shared body storage qualification — 8.60

The full-body cache and standalone ordered writer now share one lifetime-managed byte allocation and process-budget lease, with unique mutable admission and completed-body handoff after join. This closes the first ordered-writer prerequisite; ordered network writes remain inactive pending disk-only resume-credit and cancellation integration. Native qualification passes 1,303 checks in twelve runs, with 99 cache checks on each drive; the standalone component passes another 94 checks. Five products and eight fixtures built from 518 inputs. A real 64 MiB split App download passed exact bytes/hash/catalog, normal exit and separate post-exit flush checks using write-buffer hashing. Generated fixtures and payloads were removed, and retained inputs/products and protected files rechecked. Installed desktop remains 8.32; no installer, deployment, push or release. No IDM speed parity claim. [Evidence](../../docs/shared-body-storage-8.60.md).


## Disk-credit checkpoint research adapter

`disk_credit_checkpoint.hpp` and its fixture exercise generation/file-owner binding, acknowledged disk prefixes, detached snapshot pins and nonblocking retirement. The final binary passes 74 checks on each drive, including real successful/failed `atomicText` saves through the catalog coordinator, concurrent writes and unchanged actual Job/Manager UI snapshots. This is not connected to production saves or network transfers; source/build versions remain 8.60 / browser 0.72.24. All owned fixtures and the test binary/object are removed after evidence and hash checks. [Contract, evidence and remaining integration](../../docs/disk-credit-checkpoint-plan.md).


## Native disk-credit integration — 8.61

The synchronous cache-eligible fresh root HTTP path now binds successful disk writes to its exact worker generation and parts file. Catalog saves project written resume prefixes while UI snapshots remain live; captured leases protect retirement through actual save acceptance/failure. Transfer retirement reconciles counters outside pending saves before reset/publication. All 1,644 selected checks pass in eighteen runs, including 46 Manager integration and 74 native checkpoint checks per drive. Five products and twelve fixtures built from 522 inputs; Core, Transfer and Bridge rebuilt with thirty-nine qualified core objects retained. A real 64 MiB split App download used the binding, reconciled all bytes and passed hash/catalog/normal-exit/post-exit-flush checks. Removed 487 generated files / 157,425,750 logical bytes plus its payload. Installed desktop remains 8.32; no installer, deployment, push or release. Ordered network writes remain inactive, and no IDM speed parity is claimed. The rejected attribute-only pin attempt is retained separately. [Evidence and remaining gates](../../docs/native-disk-credit-8.61.md).


## Ordered HTTP candidate — 8.62

The native ordered-body candidate now runs through real parallel HTTP reception, retry overlap checks, disk-credit catalog leases and same-allocation advisory hashing. Normal builds keep synchronous writes; the candidate requires a private compiler define. All 1,780 selected checks pass in twenty final-binary-matched runs. A real 64 MiB private App download passes; 16 MiB actual Manager pause/reload/resume and 22 dropped-connection recovery pass with independent byte/hash verification. Five normal products also build and pass an 8 MiB lifecycle/hash smoke with ordered writes disabled. Unused ordered-worker handles and stale generation diagnostics are removed. Generated fixtures/payloads were cleaned, with protected installed/personal files and both builds rechecked. Installed UDM remains 8.32; no installer, deployment, push or release. Whole-pipeline fault/race and balanced IDM performance gates remain open. [Evidence, compiler gating and next checks](../../docs/ordered-body-8.62.md).


## Ordered storage-fault controller

`ordered_storage_fault_control.cpp` links the matched 8.62 candidate core and temporarily intercepts only its own executable's WriteFile imports, applying faults to its exact owned parts file. Fourteen cases / 126 assertions on C:/D: cover zero/partial/short writes, deliberately corrupt uncredited bytes, early/EOF holds, pause-save rollback and independent full recovery. The fixture binary/object and all payloads are removed after qualification; production source, default activation and versions stay unchanged. [Contract, evidence and remaining gates](../../docs/ordered-storage-fault-8.62.md).

## Ordered response controller

`ordered_response_control.cpp` links the unchanged private 8.62 core; `response_policy_origin.py` extends the synthetic loopback origin with twelve immutable-body response policies. Twenty-four C:/D: cases / 126 native assertions cover one-use and no-range responses, helper capacity/refusal, wrong ranges/tags, truncated helper recovery and representation replacement blocked by a detached old-generation save. Independent final hashes and post-exit flush pass. Controller executable/object and payloads were removed. This is correctness qualification, with no production version/default activation change or performance claim. [Evidence and remaining gates](../../docs/ordered-responses-8.62.md).

## Ordered shutdown controller

`ordered_shutdown_control.cpp` links the unchanged private 8.62 core. Four C:/D: processes / 20 assertions exercise direct Manager stop while an older detached catalog save is held, joined partial-credit persistence and fresh-process complete recovery. Saved segment prefixes and full final hashes are independently verified. All fixture payloads and executable/object are removed. [Evidence, rejected driver and limits](../../docs/ordered-shutdown-8.62.md).

## Ordered lifecycle controller

`ordered_lifecycle_control.cpp` links the unchanged private 8.62 core. Six C:/D: scenarios / ten processes / 62 assertions exercise actual folder, refresh-address and removal commands, active refusal during held-save retirement, real operation-save rollback and fresh-process exact recovery. Owned payloads and fixture executable/object are removed. [Evidence and scope](../../docs/ordered-lifecycle-8.62.md).

## Normal versus ordered App comparison

`compare-ordered-body.py` compares hash-pinned normal/private App products on C:/D: with alternating order, real process-exit timestamps, separate output flushes, independent hashes and immediate scoped payload removal. The first 36-transfer run keeps all outliers and does not pass the no-regression promotion decision. No IDM or installed-app comparison is claimed. [Measured results and remaining blocking work](../../docs/ordered-comparison-8.62.md).

## Native finishing wait attribution, 8.63

Native HTTP records now separate receiver join duration (including reception), ordered settlement, optional writeback teardown and prefix-hasher stop. New writeback statistics distinguish an outstanding callback from queued discarded work and retain the first finish observation. The normal/private 8.63 builds pass 696 affected checks and four App samples; no latency improvement is inferred. [Evidence, field semantics and cleanup](../../docs/join-waits-8.63.md).

## Pinned catalog proof comparison, 8.64

`pinned_catalog_read_control.cpp` compares the unchanged pathname reader with the new pinned reader under the same restrictive pin, JSON parse/equality and temporary destruction. Ten alternating pairs at three record counts on C:/D: provide 120 correct measured proofs. All six medians improve; seven slower individual pairs remain. It is a warm component comparison, not full App or IDM timing. [Evidence, cleanup and scope](../../docs/pinned-catalog-read-8.64.md).

## Accepted catalog text comparison, 8.65

The subsequent [8.66 split-writeback comparison](../../docs/data-writeback-8.66.md) uses `compare-completion.py` with an optional frozen `--baseline-udm`/hash pair and three-client order rotation. `--remove-owned-payloads` removes only each verified output inside its freshly marked comparison root. Nine D: split transfers compare actual exit and symmetric post-exit durability; both-drive pause/reload/resume controls exercise the new worker. The report retains the full-elapsed regression versus 8.65 alongside the observed IDM wins.

`capture_app_finishing.py` stages a headless real-MFC-App capture behind live fixture/hash/worker admission. [Protocol, cleanup controls and current unexercised live path](../../docs/headless-app-capture-8.65.md) explicitly distinguish infrastructure checks from App performance or kernel-cause evidence.

`analyze_app_finishing.py` selects the final body flush and joins admitted native/kernel timelines with explicit identity, coverage and correlation checks. [Offline controls, five-observation calibration and interpretation limits](../../docs/app-finishing-phase-analysis-8.65.md) distinguish the retained native evidence from the still-unexercised live join.

`analyze_cswitch_prefix.py` and `test_cswitch_prefix.py` perform a separately scoped offline analysis of retained context-switch records. [Selected fields, TDH controls, measured off-CPU overlap and limitations](../../docs/cswitch-prefix-analysis-8.65.md) preserve the failed full-schema gate and do not claim a product speed fix.

`finish_scheduling_lifetime.cpp` builds an explicitly separate disk/scheduling kernel collector without FileIo providers. [Preparation and independent policy gates](../../docs/scheduling-profile-8.65.md) distinguish the narrower observation model from the unresolved full-FileIo schema gate. No live kernel capture is qualified by the offline checks.

The subsequent [real MFC-app I/O observation](../../docs/app-io-observation-8.65.md) locates native body/catalog flushes and scoped reads in existing 8.65 builds. Its console wrapper and observer are diagnostic fixtures; the results are not uninstrumented performance comparisons. `summarize-app-io.py` validates contiguous body coverage and nested call envelopes and emits redacted evidence.

`accepted_catalog_read_control.cpp` compares the same pinned reader with parsed JSON proof versus accepted-text equality. `accepted_catalog_manager_control.cpp` separately measures actual Manager stop/destruction against matching 8.64/8.65 headers and frozen cores, including live snapshot work. All 120 disk proofs and forty stop measurements succeed without extra stop/destructor writes. Results retain their warm-cache, group-order and shared-vendor provenance limits. [Measured results, ownership tests and cleanup](../../docs/accepted-catalog-text-8.65.md).

## Metadata write-through research on 9.64

`intent_write_through_probe.cpp` measures cached, buffered write-through and aligned unbuffered metadata transactions. `catalog_padding_reuse_probe.cpp` verifies exact physical catalog text and counts writes through stop/destruction. `write_through_recovery_probe.cpp` terminates an owned seed process at publication boundaries and verifies recovery in a fresh process. These are diagnostic fixtures, not production policy. The aligned prototype failed the all-pairs speed gate. See [results and preserved experimental patches](../../docs/metadata-write-through-investigation-9.64.md).
