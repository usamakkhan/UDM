# Inline browser PAC — candidate, 2026-10-18

**Qualification failed:** the cancellation fixtures left three Windows PAC
workers consuming CPU after their callers returned. See
[worker-lifetime investigation](pac-worker-lifetime-20261018.md). Earlier
passing caller-cancellation results below do not prove execution cancellation.
Installation requires fixing this evaluator-lifetime defect.

The candidate extends the URL-PAC handoff with a separate `pacScript` source
and the `browserPacInline: 1` capability. A URL-only desktop cannot accept this
source. The script is limited to 65,536 UTF-8 bytes and stored inside the
existing encrypted, request-bound browser proxy record. Mixed URL/script
sources and embedded NULs are rejected. Chromium and Firefox package manifests
are prepared as version 0.63.19; this candidate is not installed.

Windows' asynchronous PAC API accepts HTTP(S) script URLs and executes scripts
outside the application process. `InlinePacSource.hpp` supplies the bytes via
an ephemeral IPv4 loopback endpoint with an unpredictable path. It writes no
script file. The endpoint accepts only an exact GET path, caps request headers,
uses nonblocking sockets, and checks shutdown while waiting on a client. It
exists only during lookup. The resolver removes the temporary URL and script
from the resulting transport settings.

Source: [Microsoft WinHttpGetProxyForUrlEx documentation](https://learn.microsoft.com/en-us/windows/win32/api/winhttp/nf-winhttp-winhttpgetproxyforurlex).

## Checks completed so far

- Browser configuration: 102 passed.
- Desktop capability/handoff: 18 passed.
- Firefox route metadata: 13 passed.
- Settings-read recovery: 4 passed.
- Native transport probe: inline script selected a real loopback proxy and
  downloaded the expected bytes; invalid JavaScript failed without a target
  request; an infinite script was cancelled in 454 ms wall time (200 ms request
  deadline). The stalled-fetch control used a URL script and stopped in 362 ms.

The cancellation fixture originally used a loopback target. Windows fetched
the script but bypassed evaluation for that target, so the infinite-script
assertion failed. The corrected fixture uses a reserved `.invalid` target and
a loopback proxy, ensuring script evaluation is necessary for success. The
initial failure is not counted as a product fix or a passing cancellation test.

An extended transport-probe run also passed Unicode text and an exact 65,536-byte script, along with the transfer, cancellation, and invalid-script cases. Timings varied under concurrent compilation; these are functional checks, not a throughput benchmark.

The full native rebuild and desktop build completed. The desktop candidate
SHA-256 is `F781968FF942A31C14E14F1E4D18A2740E1E5BB04EB6AD00DA2FAB6063257219`.
The focused native PAC suite passed 25 checks, including source lifetime,
encrypted catalog recovery, preview and completed-file verification.

The broader `--connection-checks` run is not qualified: two runs stopped in
the preceding HLS checks with `Media helper timed out.`, at 131 and 134 passes
respectively. The second run had already verified the discontinuity output's
duration and streams. Running the bundled FFmpeg separately against that same
188,348-byte file decoded it successfully (exit 0, approximately 9.1 seconds).
This does not explain the suite timeout or establish that it is harmless.
No timeout was relaxed, and these runs are not counted as passing connection
coverage. The connection-specific checks occur after the HLS checks.

## Isolated browser acceptance

Chrome completed all nine checks with exit 0. Both the proxy-selected and
DIRECT downloads completed at 4,194,335 bytes with the expected SHA-256
`AD288F6AA94DEC1C5F67E3A98FD45F5193D9376F6A4E8B38C339138899F5DBC5`.
The browser canceled its transfers only after native acknowledgement. The
catalog retained encrypted configuration; extension storage contained no
inline script. This used a fresh profile and a separate native catalog.

Edge failed its initial native diagnostic request with "UDM did not
acknowledge the request" and exited 1. It did not reach PAC configuration or
download checks. This is an unresolved startup/communication failure, not a
passing Edge acceptance result or evidence of a PAC-routing failure.
An instrumented rerun reproduced the failure: native connection counters
reported one connection, zero completed replies and one failure, with no
pending request, persistent connection or browser download afterward. This
narrows investigation to the initial handshake, before PAC evaluation. The
fixture now saves these counters on failure without retrying an admission.

A separate, minimal Edge extension then received three valid native `hello`
replies in 3,794, 2,690 and 1,473 ms. Its diagnostic deadline is 15 seconds,
but it still counts a trial over the production five-second deadline as a
failure. The direct native protocol suite also passed all nine checks.
These results show that the failure is intermittent; they do not qualify
the failed end-to-end Edge workflow.

The minimal diagnostic exposed that the installed host used by these runs
reports version 0.92.0, while the desktop candidate is 1.0.3. A matching host
was built (SHA-256
`F710EA1AB8DFFFC8D7786AEB2C5399D3B5E1A1FE85A96890CF1DF927F28D5F26`)
and passed all nine real native protocol checks. The full Edge workflow still
failed its initial handshake with that host, so rebuilding it did not resolve
the failure. `tests/native-handshake.browser-live.cjs`
reproduces this diagnostic in an isolated profile and submits only `hello`.

A timed `--live-hls-checks` run passed the previously failing discontinuity
decode, then failed with `Download paused.` at the rolling recording's
20-second cancellation deadline (136 passes, one failure). Fixture generation
took approximately 28 seconds and discontinuity decoding approximately 15
seconds in that run. The source and deadlines were unchanged. This adds
evidence of variable execution time but does not establish its cause or
qualify the media suite.

For diagnosis only, a temporary copy of the extension changed the native
`hello` deadline from 5 to 15 seconds. With the matching 1.0.3 host, Edge then
passed all nine PAC workflow assertions, including both exact-file hashes.
The repository extension still uses five seconds. This run is not production
acceptance and does not erase the three production-handshake failures.
A system-counter sample during this investigation showed total CPU at 100%,
approximately 3.7 GiB available memory and 0.137 ms average disk transfer
latency. CPU contention is a plausible contributor, not a proven root cause.

The follow-up 0.63.20 extension changes the actual startup deadline and adds
startup-specific errors with regression coverage. It passed all nine Edge
workflow checks with the matching host and no diagnostic override; see
[the handshake validation record](native-handshake-20261018.md). Earlier
failures remain recorded above. The media-suite timing failures, final native
capture-contract qualification, auto-detection/WPAD handoff and live IDM
fallback equivalence remain open. This candidate is not installed.
