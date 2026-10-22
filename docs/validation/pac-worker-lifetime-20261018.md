# PAC cancellation leaves evaluator workers running — qualification failure

Three Windows `pacjsworker.exe` processes were observed consuming approximately
one CPU core each after the cancellation probes had already returned. Their
parent was the WinHTTP AutoProxy service process (PID 2208).

| Worker PID | Local creation time | Matching owned fixture |
|---|---|---|
| 22816 | 2026-10-18 11:23:33.905012 | `udm-pac-cancel-20261018-c`, created 11:23:33 |
| 28612 | 2026-10-18 11:26:02.819572 | `udm-pac-inline-20261018`, created 11:26:01 |
| 15460 | 2026-10-18 11:36:40.169934 | `udm-pac-inline-20261018-boundary`, created 11:36:36 |

The CPU sample reported 95%, 100% and 100% respectively (one core is 100%).
A fourth PAC worker, PID 22940, was not busy in that sample and is excluded from
cleanup. The exact creation-time matches to the three infinite-script fixtures
identify the affected workers. Termination without elevation returned Access
Denied. A guarded administrator cleanup was requested for only these three
IDs, validating name, parent and creation time before each termination.
The elevated cleanup receipt subsequently confirmed `ok: true` and `stopped`
for all three IDs, with no error. The shared service and fourth worker were
not targets of the script.
The service subsequently created two replacement workers. A follow-up process
performance query reported zero CPU for all three remaining workers (22940,
5364 and 32128). The ongoing native compiler's CPU time advanced from roughly
56 seconds before cleanup to 155 and then 227 seconds in subsequent samples.
No replacement worker was stopped.

Earlier probe results establish bounded **caller return**, not termination of
script execution. They are not sufficient cancellation acceptance. The inline
and URL PAC candidates must not be installed on the strength of those results.
The leaked CPU work is a concrete contributor to system contention; individual
handshake/media timeouts are not independently attributed solely to it.

`tests/pac-cancellation.native-live.cjs` no longer submits an infinite script to
the shared Windows evaluator. It records that case as blocked and exits nonzero,
while retaining the other cases. Qualification requires a dedicated evaluator
whose execution can be stopped, plus verification of worker lifetime and CPU
after cancellation. A larger caller timeout does not solve this issue.

Microsoft documents that the [Ex API executes out of process and supports caller
cancellation](https://learn.microsoft.com/en-us/windows/win32/api/winhttp/nf-winhttp-winhttpgetproxyforurlex).
The [RUN_INPROCESS option has no effect on that Ex API](https://learn.microsoft.com/en-us/windows/win32/api/winhttp/ns-winhttp-winhttp_autoproxy_options).
These documents do not establish that this observed worker has stopped when the
caller returns. No service restart or system proxy change was requested.

## Follow-up validation

After cleanup, the native connection suite completed with **471 passed, 0 failed**.
See `browser-pac-connections-after-cleanup-20261018.txt`; local fixture paths are
redacted. This does not clear the execution-lifetime qualification failure.

An isolated QuickJS-NG v0.17.0 prototype (upstream commit
`6d46d07d04041b40f4f49eaa7fdebe44c314c699`) passed eleven checks, including
top-level and function infinite loops, memory exhaustion, stack exhaustion,
invalid scripts and bounded results. Every owned helper exited, and none needed
the external three-second kill guard. See `pac-owned-runtime-prototype-20261018.json`.
The prototype uses a 250 ms execution budget, 32 MiB memory limit and 256 KiB
JavaScript stack limit without the engine's operating-system library.

This is research evidence, not an installed fix. Standard PAC helpers, production
IPC, parent-owned cancellation and process lifetime, source fetching, packaging
and full compatibility tests remain necessary before integration and installation.

The next prototype revision assigns a suspended evaluator to a Windows job with
`JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE` before resuming execution. No job handle is
inherited by the worker. The three lifetime tests passed: ordinary completion,
parent deadline terminating a looping worker in 125 ms, and abrupt parent exit
with the worker gone within 18 ms. These are observed timings, not guaranteed
latency bounds. See `pac-owned-lifetime-prototype-20261018.json`.
The original eleven runtime checks also passed again. Script input is now read
into a bounded 65,537-byte buffer, rejecting input above 65,536 bytes.

The supervisor remains a research harness; it does not yet transfer PAC results
to UDM or implement the desktop cancellation interface. No infinite script was
submitted to the shared Windows PAC service during these tests.

A clean build using the tracked `tools/idm-research/build-pac-runtime-probe.ps1`
also passed all fourteen runtime/lifetime checks. Its binary hash, pinned engine
revision and complete results are in `pac-owned-reproducible-20261018.json`.
MSVC emitted upstream engine conversion warnings; compilation and linking
completed successfully.

## Portable hostname helper integration

`native/PacFunctions.hpp` now supplies original implementations of
`isPlainHostName`, `dnsDomainIs`, `localHostOrDomainIs`, `dnsDomainLevels` and
`shExpMatch`. The prototype loads them before the captured script, within the
same memory and execution budget. Behavior follows the portable hostname and
wildcard functions documented in [Mozilla's PAC reference](https://developer.mozilla.org/en-US/docs/Web/HTTP/Guides/Proxy_servers_and_tunneling/Proxy_Auto-Configuration_PAC_file).
Only the portable `*` and `?` glob operators are supported; browser-specific
character classes are not claimed. Regular-expression punctuation is literal.

The real evaluator passed 27 helper vectors, the previous eleven runtime checks,
and an adversarial wildcard case that was interrupted at 250 ms: **39 checks**.
The literal-star input regression initially failed and passed after wildcard
precedence was fixed. All three process-lifetime checks also passed after helper
integration. Results are `pac-string-helpers-20261018.json` and
`pac-string-helpers-lifetime-20261018.json`.

These functions are currently consumed by the research evaluator only. DNS/IP
and date/time helpers, production result transport, source retrieval and desktop
integration still remain. This is not evidence of complete IDM PAC parity or an
installed application change.

## DNS and IPv4 helper integration

The prototype now registers `dnsResolve` through Windows
[`GetAddrInfoW`](https://learn.microsoft.com/en-us/windows/win32/api/ws2tcpip/nf-ws2tcpip-getaddrinfow),
requesting IPv4 results and freeing the returned address list. Host input is
bounded and rejects embedded NULs; UTF-8 is converted strictly to Unicode.
Failed lookups return null. The shared helper source supplies `isResolvable`,
`convert_addr` and `isInNet`. Subnet comparisons preserve 32-bit mask semantics.
Each evaluation permits at most 32 DNS calls and checks its deadline before and
after resolution. Exceeding a resource limit raises an error rather than
inventing a direct route.

Twenty new vectors passed using localhost and numeric addresses, without relying
on an external DNS server. The full evaluator run passed **59 checks**, and the
three lifetime cases passed again. Evidence is `pac-dns-helpers-20261018.json`
and `pac-dns-lifetime-20261018.json`.

`GetAddrInfoW` is synchronous: the JavaScript interrupt cannot stop it while it
is blocked. Parent-owned process termination remains required, and a dedicated
stalled-DNS cancellation test is still missing. The ordinary runtime harness
also has an external process deadline. Local tests do not qualify external DNS,
multi-adapter address selection, or IPv6 extensions. `myIpAddress`, date/time
helpers and production integration remain unfinished. No desktop installation
was performed.

## Date and time helpers

The shared helper source now provides `weekdayRange`, `timeRange` and
`dateRange`, with local or GMT components. The implementation uses integer
calendar fields rather than mutating Date objects, avoiding local DST conversion
when a rule explicitly requests GMT. Tests replace Date within the isolated PAC
script to supply distinct local and UTC values across a year boundary.

All 44 new vectors passed, bringing the evaluator suite to **103 checks**.
All three worker-lifetime checks passed again. See
`pac-calendar-helpers-20261018.json` and `pac-calendar-lifetime-20261018.json`.
The vectors cover single values, supported range forms, wraparound, inclusive
endpoints, malformed arguments, and local/UTC selection.

The two-hour time form remains ordered; minute/second ranges can cross midnight.
Weekday and month ranges can wrap. These choices were checked against the
behavior expressed by [Chromium's PAC helper implementation](https://chromium.googlesource.com/chromium/src/+/refs/tags/140.0.7296.0/services/proxy_resolver/pac_js_library.h)
and the documented PAC forms, not by copying its implementation. Invalid
arguments return false. Unsupported argument coercions and impossible calendar
dates are not claimed to match browser quirks. No differential IDM test has yet
qualified these helpers. `myIpAddress`, stalled-DNS cancellation, production
IPC and desktop integration remain unfinished.

## Bounded request transport

`native/PacRequest.hpp` defines a versioned, length-prefixed input frame for a
script, URL, hostname and execution budget. The evaluator's `--request` mode
reads this from binary stdin and passes the supplied URL and host as separate
JavaScript arguments. Scripts and request addresses need not be stored in a
temporary file or exposed in command-line arguments in this mode.

Lengths are validated before allocating payload buffers: script 65,536 bytes,
URL 8,192 bytes, hostname 255 bytes, execution budget 1–2,000 ms. Malformed UTF-8,
embedded NULs and invalid URL/hostname whitespace are rejected. Invalid frames
return a fixed error. Script error text is limited to 2,048 UTF-8 bytes without
cutting through a character. The URL and hostname originate from the trusted
parent's parsed request; this transport does not itself prove they correspond.

The actual helper passed **17 request tests**, **103 evaluator tests**, and
**3 lifetime tests**. Results are `pac-ipc-request-20261018.json`,
`pac-ipc-runtime-probe-20261018.json`, and
`pac-ipc-runtime-lifetime-20261018.json`. The tests cover supplied request values,
route selection, Unicode, CRLF scripts, oversized/truncated frames, malformed
fields and bounded ASCII/Unicode errors.

The existing lifetime supervisor still exercises the file-input research mode.
The desktop parent has not yet been wired to these pipes or to the new evaluator.
Its job/deadline must also cover incomplete input and blocked DNS. No complete
production transport or installed PAC fix is claimed by these checks.

## Parent-owned pipe transport

`native/PacWorker.hpp` now launches a suspended helper, assigns it to a
kill-on-close job, and resumes it only after assignment. An explicit inherited
handle list contains only its input and output pipes. The job and parent pipe
ends are not inherited. A writer thread sends bounded input while the parent
drains output, avoiding deadlock when either message exceeds the pipe buffer.
The parent checks cancellation and a deadline while draining, caps output at
65,536 bytes, and tears down the job and joins the writer on errors.

The research executable's `--parent` mode exercised the actual implementation:
all **17 protocol checks** passed through a child process. Ten additional tests
passed for timeout, cancellation, repeated cancellation followed by success,
large input and an escaped output larger than the pipe buffer. A process query
after the run found no process with the tested helper's executable path.
See `pac-parent-protocol-20261018.json` and
`pac-parent-lifetime-20261018.json`.

This is the parent component intended for integration, not yet a desktop call
site. The research wrapper reads its own stdin before invoking the parent
component; its caller must close input. Production callers will supply an
in-memory frame. The result still needs strict JSON/schema and route parsing,
the desktop's cancellation type, helper packaging and PAC-source retrieval.
Stalled DNS and parent death through this new pipe path still need dedicated
verification; the earlier parent-death test exercised the file-input supervisor.

## Validated route results

`native/PacRouteResult.hpp` validates worker exit status and JSON reply types,
rejects interrupted/error replies and duplicate fields, limits nesting, and
parses an ordered list of at most 32 routes. HTTP/PROXY, HTTPS, SOCKS/SOCKS4,
SOCKS5 and explicit DIRECT are represented separately. Ports are bounded and
bracketed IPv6 addresses are validated. No DIRECT route is synthesized; a
malformed or unsupported entry rejects the result rather than changing routing.

Thirty parser checks passed, including malformed JSON, duplicate fields, deep
nesting, ports, IPv6 and preservation of route order. Twelve parent tests also
passed, now including the complete request → child evaluator → JSON validation
→ route list path and rejection of an invalid proxy followed by DIRECT.
Evidence: `pac-resolve-route-result-20261018.json` and
`pac-resolve-parent-20261018.json`. The research executable exposes this path as
`--resolve <timeout-ms>`; it remains separate from the desktop resolver.

The remaining integration work includes mapping these route records into UDM's
transport preferences, source retrieval, packaged helper builds, address helper
completion, and blocked-DNS/parent-death checks through the pipe path. No
application installation or claim of complete IDM behavior is made here.
