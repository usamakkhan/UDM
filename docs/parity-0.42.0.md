# UDM 0.42.0 / browser 0.32.0: indexed video consistency

The browser previously checked a recorded DASH file while reading its index, then discarded the validator before the desktop fetched media. A changed file could therefore supply unrelated ranges. The new handoff binds each selected indexed resource to its exact URL, observed size and available strong ETag or Last-Modified.

## Download and resume behavior

- The desktop capability is checked before any index fetch, so an older app cannot silently ignore the binding.
- Every bound native range uses If-Match for a strong ETag, or If-Unmodified-Since for Last-Modified. Conflicting captured conditional headers are replaced. The request keeps its normal scoped authentication.
- Bound requests refuse redirects and check returned validators, exact range and captured total size before accepting body bytes. HTTP 412 and identity/size mismatches fail without repeated requests against the stale index.
- Saved part records contain a protected copy of the exact part/resource binding as well as the existing SHA-256. Only matching strong-ETag records are reused. Missing, old, mismatched, date-only or size-only cache bindings trigger fresh part downloads.
- Existing HLS and non-indexed DASH plans retain their supported behavior. Existing saved plans without resource bindings are not retroactively rewritten; recapture an indexed video to use the new checks.

## Validation

**854 native checks, 404 browser checks, 8 native-protocol checks and 81 live-browser checks passed.** Live totals: Chrome hierarchy 24, Edge hierarchy 24, Firefox hierarchy 23, Chrome flat 10.

Fresh results and request records are in [evidence-0.42.0](evidence-0.42.0/summary.json). The native suite covers successful assembly, protected persistence, resume reuse, old cache rejection, changed size/version, redirects, date/size-only cache policy and malformed bindings. Browser unit tests include capability negotiation and the actual handoff payload.

Real isolated Chrome and Edge tests use the extension panel and audio chooser. A temporary Firefox observer invokes its real page-scoped discovery and handoff; this is not a Firefox visual-button qualification. Generated clear 360p MP4 and audio-only M4A use selected Spanish audio, with exact ranges and concurrent native media requests. Five browser-to-native fault cases change the ETag, remove it, change total size, return HTTP 412 or redirect only after index capture. No case publishes an output or accepts media bytes.

The fixture deliberately controls responses and uses separate profiles/catalogs. It is not a same-server IDM performance comparison or proof of universal real-site support. The first native build exposed a missing WinINet declaration include; the corrected build is the one tested and packaged.

## Limits and remaining parity

Only strong server ETags provide the tested version identity. Last-Modified has weaker time granularity, and length alone cannot detect a same-size mutation. Such sources never reuse saved parts in this path, but this does not make their identity guarantees equivalent to a strong ETag.

External DASH indexes, multi-SIDX reference ranges, live recording, protected media and arbitrary site/session variants remain outside this implementation. The existing 103-workflow inventory still has 88 implemented, 12 partial, 2 unverified and 1 deferred by the user. “Implemented” is not exhaustive acceptance. Driver handoff/compatibility, several proxy/site workflows, publisher signing/updater/clean-machine acceptance, accessibility and matched-route speed comparison remain open. Full IDM parity is not established.

Conditional requests follow [HTTP Semantics, RFC 9110](https://www.rfc-editor.org/rfc/rfc9110.html#name-if-match); strong and weak validator behavior is distinguished in its [validator section](https://www.rfc-editor.org/rfc/rfc9110.html#name-validator-fields). This is independent UDM code.

## Package

UDM-0.42.0-Setup-x64.exe

SHA-256: E76C01D7B40D07DD379721DFEADE1613A534CEC036667F45644E0041BEEA44F2.

Installation on the current PC is documented in [deployment evidence](evidence-0.42.0/deployment.json); it does not establish a publisher signature or clean-machine acceptance.

## Current-PC activation

UDM 0.42.0 is running from D:/UDM/release. The native host and desktop both report 0.42.0 and adaptiveResources=1. Existing Chrome and Edge integrations were reloaded to 0.32.0 and both reported connected and ready. The original 23-record catalog remains byte-identical. Prior source and binaries are backed up under the local adaptive-binding-20260927 staging folder; previous installers were retained. No driver or Test Mode setting was changed.
