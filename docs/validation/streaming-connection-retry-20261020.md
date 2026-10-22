# Streaming connection recovery — October 20

The October 19 live browser test stopped with Windows error 12030. Manual
Resume reused 215689418 saved bytes and completed successfully. The streaming
entry point had no automatic retry around its validated segment cache.

The candidate now retries typed WinHTTP connection errors, timeouts, connection
failures and name-resolution failures within the configured queue retry limit.
Retries use cancellable bounded backoff and re-enter through the existing cache;
incomplete segment data is discarded. TLS errors, invalid UMP/media responses,
storage errors and exhausted budgets still propagate. It does not manufacture
progress or change measured transfer speed.

The native streaming suite passed all 159 checks, including an interrupted
response followed by byte-identical recovered tracks, a zero retry budget and
terminal TLS failures. Existing parallel, audio-only, compressed-media and
prefetch/publication checks also passed. Evidence is in
`streaming-connection-retry-20261020.json`.

An initial broad run lacked the isolated FFmpeg/FFprobe dependencies. The next
run exposed the harness inheriting the real D: download folder while its output
was on C:, invalidating a same-volume file-identity assertion. The harness now
sets its download folder inside the isolated test root. The final run used that
isolation and the installed media tool copies.

The desktop candidate built successfully.

Installed and hash-verified: 30C26951981F998AF7C14BD77FDE1E6CC5F2F15714AE92B96983B3BC541E3101. Receipt: `streaming-connection-retry-installed-20261020.json`. A newly interrupted live browser transfer has not yet been tested.

## Retry limits and cancellation

The expanded streaming suite passed all 164 checks. Persistent timeouts stop
after the initial request plus the configured two retries. A queue-specific
zero-retry setting overrides the global preference. Stopping after the transfer
enters retry backoff raises cancellation within one second and issues no further
request. Neither exhaustion nor cancellation publishes an incomplete track or
final file. Evidence: `streaming-retry-boundaries-20261020.json`.

These are native tests with injected transport failures, not a live browser
network-interruption test. Only the test harness changed in this follow-up;
the installed desktop hash above was rechecked and no reinstall was necessary.

## Response-read interruption coverage

The suite now injects a WinHTTP connection error from the response reader after
bytes have arrived, rather than only failing while opening the next response.
It interrupts framing, partial media, and a response containing already completed
segments. All three cases make exactly one retry and recover byte-identical
video/audio tracks without publishing an unmerged final file.

All 173 streaming checks passed (`streaming-read-retry-20261020.json`). These
remain injected native transport tests, not a live network-interruption result.
The installed desktop is now the later dialog-notification build, SHA-256
831B071C33FBF9547AFF4752A719A9FC372355EEA57F0F017649A04AE79220E5;
it contains the same retry implementation. This follow-up changes tests only.
