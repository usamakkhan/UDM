# UDM browser integration 0.20.1

25 September 2026. Browser patch for UDM native 0.20.0; no native engine rebuild, driver, permission or identity change.

## Fixed

**Ctrl-click could be ignored on fast responses.** A real Edge test showed a bypass message arriving at 13:02:41.332 UTC, followed by the download event at .333 and completed source validation at .334. The old handler looked only for an already-validated gesture, so it paused and handed off the file anyway. Pending gestures are now registered before asynchronous validation. The matching download waits for validation and checks the eight-second expiry; invalid or changed source pages cannot force a handoff.

**Eligible downloads now pause before desktop preference lookup.** This prevents native startup or a slow preference reply from letting a small file finish first. If desktop capture is disabled, a desktop rule excludes the file or the native host is unavailable, the browser transfer resumes. Local exclusions, disabled browser capture and a validated bypass do not pause the file or contact UDM.

Before native handoff, UDM checks whether the user canceled, resumed or removed the paused browser transfer. Those decisions are retained without creating a native job.

## Verification

- 50 browser logic checks passed, including delayed preferences, pending force/bypass validation, invalid source rejection, desktop rejection and user cancel/resume/removal.
- Final real Edge and Chrome tests each passed five checks: native connection to an explicitly isolated history, Alt-click capture outside both file-type filters, Ctrl-click staying in the browser, capture-off remaining off, and no page-script errors. The forced native file was 8 MiB and matched the synthetic fixture hash.
- The full handoff sequence was also exercised: direct MP4 matched its source bytes, recorded HLS assembled H.264 video plus AAC audio at 360p with 8.021-second duration, normal capture canceled the browser transfer only after native acceptance, and iframe navigation excluded old playlists. Edge passed this sequence on 0.20.0; Chrome passed it on 0.20.1 before the final user-cancel guard. The final guard is covered by the 50 logic checks and both final native shortcut suites.
- All 15 original UDM records and queue data remain unchanged. The paused ISO counts remain 546,308,096 and 571,867,136 bytes.
- Firefox sources were regenerated and compared with Chromium. Firefox itself was not tested live.

One test initially used a `.bin` endpoint. Edge received an empty HTTP 204 response, although the local fixture returned a valid byte range when requested directly. BIN is in IDM's configured interception list; the exact component generating the 204 was not established. A dedicated `.udmforce` fixture extension removed that interference and exposed the separate, confirmed Ctrl-click race. The 204 failure is not evidence of UDM's transfer engine rejecting a valid response.

## Status and evidence

The updated unpacked extension is in `D:\UDM\browser\chromium`; the native desktop remains 0.20.0. Activation in the regular browser profiles still needs verification after the PC is unlocked. Windows returned `GetCursorPos failed: Access is denied (0x80070005)` and black screenshots during this turn. These successful browser tests used separate test profiles and separate native history.

[Verification and preserved history](../benchmarks/gui-0.20.1/verification.json), [final Edge results](../benchmarks/gui-0.20.1/edge-final/extension-results.json), and [final Chrome results](../benchmarks/gui-0.20.1/chrome-final/extension-results.json).

Rollback browser files: `D:\UDM\backups\gui-0.20.1-20260925\browser`. Previous YouTube layout repair and remaining GUI differences are documented in [0.20.0](gui-0.20.0.md). These local tests do not establish Internet speed parity or support for every video site.
