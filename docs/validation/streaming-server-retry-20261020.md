# Streaming server retry support

SABR previously converted every non-success HTTP response into a generic fatal
error. It now preserves the HTTP status and Retry-After header in HttpRejected,
using the same temporary-error policy as ordinary downloads. Eligible statuses
include 408, 425, 429, 500, 502, 503 and 504. A requested wait over five minutes
is not retried automatically. Authentication and expired-link failures remain
terminal with the existing refresh/authentication guidance.

Connection and server failures share the queue retry budget. The existing
cancellable wait handles the delay, and progress identifies a server retry.
Retry re-enters the validated segment cache instead of publishing partial media.

All 182 streaming checks passed. New cases recover byte-identical audio/video
after 429 and 503, verify a one-second Retry-After is not shortened, reject
401/403/410 and excessive waits without replay, and verify a mixed connection/
server failure cannot exceed the shared retry budget. These use injected native
transport failures; no live YouTube temporary-error response was observed.

The standalone candidate built successfully and was installed after a normal
exit with no active transfers. The installed executable matches SHA-256
8E06D954808E037172AF8F73F2FF910F7478D8DC48C642928B32AC784B1E9E19.
UDM restarted in background mode. The installed native host passed the real
stdio-framing and desktop-pipe round trip (`tests/native-host.cjs`, ping only).
Receipt: `streaming-server-retry-installed-20261020.json`.
Evidence: `streaming-server-retry-20261020.json` and
`streaming-server-retry-build-20261020.json`.

The Chromium/Firefox extension files remain 0.63.22. Reload confirmation is
still pending, and no live temporary HTTP error has been exercised.
