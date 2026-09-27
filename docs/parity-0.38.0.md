# UDM 0.38.0 — form-download reliability and activation

27 September 2026. Native **0.38.0** is running locally. Extension **0.27.1** is loaded in the existing Chrome and Edge sessions; both popup connection checks report “UDM is connected and ready.” English is retained.

## Changes

- Removed the unrelated MouseButtonSwap scripts and restored the UDM README. Recovery copies remain in the external staging backup.
- Increased supported browser POST bodies from 64 KiB to 1 MiB. Desktop negotiation retains the older 64 KiB fallback. Native input frames allow 2 MiB; replies remain bounded to 256 KiB.
- Bounded captured request bodies to 8 MiB across at most 256 recent records. Bodies never enter extension persistent/session storage; native request storage uses Windows DPAPI.
- Fixed redirected-body fallback: only an omitted same-origin 307/308 body may inherit prior bytes.
- Prevented corrupt multipart replay after real Chrome/Edge tests showed missing Blob contents. Multipart forms continue in the browser.
- Added a history-save guard matching the 32 MiB read limit; rejected additions preserve the readable catalog.
- Fixed Firefox automatic form downloads. Request details are captured before pause, matching response events are awaited for up to 750 ms, and Firefox's interrupted pause states are recognized. A successful pause before the first payload byte can report paused=false; UDM now handles that observed case.
- Unknown request details remain with the browser instead of being guessed to be GET. Captured upload length is checked against Content-Length when available; unverified empty bodies are rejected. Unsupported forms are declined before pause.
- A pre-pause snapshot still obeys current cookie/authorization permissions. Native rejection or unavailable desktop capture releases the browser transfer; browser cancellation follows native acknowledgement.

Supported raw bodies are replayed byte for byte. URL-encoded dictionaries preserve their provided values but cannot promise the original arbitrary wire ordering or encoding. POST uses one native submission without probes, parallel submissions or automatic retries. Explicit retry is a new submission.

## Verification

The activation follow-up passed **155 automated checks and 39 live browser checks**:

| Check | Passed |
|---|---:|
| Shared request capture and upload-length validation | 28 |
| Automatic handoff, event ordering, pause states and recovery | 40 |
| Desktop body-limit negotiation | 6 |
| Browser/media handoff regression suite | 54 |
| Native connection and request-context integration | 20 |
| Browser integration policy | 7 |
| Real Chrome form downloads | 13 |
| Real Edge form downloads | 13 |
| Real Firefox form downloads | 13 |

Chrome and Edge handed off full 1 MiB forms. Firefox handed off ordinary UTF-8 forms and a 65,537-byte form. This Firefox build exposed incomplete data for the 1 MiB fixture, which correctly completed in Firefox without a native replay. Oversized forms and multipart uploads also stayed in the browser with their file bytes intact.

All completed fixture outputs matched SHA-256. Every supported form caused one browser POST and one native POST; no native range/probe submission occurred. Firefox retained one persistent native connection. The local response fixtures were paced to allow interception under test-machine load.

The preceding native 0.38.0 build passed 743 native checks and 8 framing checks. Native binaries were unchanged during this extension follow-up.

Tests used synthetic loopback data, isolated profiles and separate catalogs. Temporary native-host registrations were cleaned up. Firefox testing used a separate temporary extension/profile, not the user's existing Firefox session.

Evidence: [activation-browser-0.27.1](../benchmarks/parity-0.38.0/activation-browser-0.27.1). Earlier build evidence remains in [parity-0.38.0](../benchmarks/parity-0.38.0).

The existing 23-download catalog remained byte-identical:
SHA-256 AAE5B9BF965F9850CC771232D988052FF5083FD809EA711B1520FB10A0ECC851.

## Installation and remaining limits

The native app, native host and monitor are activated as 0.38.0. Chrome and Edge were reloaded and their connection checks passed. The 0.38.0 setup package was rebuilt with extension 0.27.1; no public release was published.

Bodies above 1 MiB, incomplete browser captures, multipart replay, browser/worker-loss continuity and universal interception remain open. Incomplete captures continue in the browser. No driver or boot-policy change was made; the existing signed network runtime verified successfully with Test Mode off.

Full IDM parity remains incomplete; see [the workflow inventory](idm-parity-0.38.0.md). Localization is deferred while English is sufficient.

## Primary references

Chrome documents form dictionaries and raw upload parts in [webRequest](https://developer.chrome.com/docs/extensions/reference/api/webRequest), and framing limits in [native messaging](https://developer.chrome.com/docs/extensions/develop/concepts/native-messaging). Mozilla describes [requestBody](https://developer.mozilla.org/en-US/docs/Mozilla/Add-ons/WebExtensions/API/webRequest/onBeforeRequest) and records limitations of reconstructing complete bodies in [Bug 1376155](https://bugzilla.mozilla.org/show_bug.cgi?id=1376155). The specific missing-body and pause-state findings above come from the local live tests; they are not universal claims about every browser version.

