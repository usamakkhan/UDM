# Edge video selection — browser 0.43.0 / native 0.65.0

UDM's video-format selector could prefer a combined 360p player URL containing an unverified `n` parameter. That choice bypassed native player retrieval and immediately failed with HTTP 403. The audio-only selector already excluded such unobserved URLs.

Video and audio selection now use the same readiness check. An `n` URL is usable only after a successful browser playback observation has matched the current media resource. If no direct file pair is ready, the quality selector prefers an MP4 adaptive format over an unavailable combined format at the same actual quality. It preserves the chosen quality, available audio language, and validated browser observations. It does not remove or guess signature/transform parameters. Quality labels also display clean separators.

## Verified in Microsoft Edge

All tests used separate Edge profiles and native download catalogs. The public test video was [the Blender Foundation's Big Buck Bunny](https://www.youtube.com/watch?v=aqz-KE-bpKQ). No personal browser cookies or history were copied.

| Test | Result |
| --- | --- |
| Installed 0.42.0, 360p | Selected format 18; HTTP 403, zero bytes |
| Installed 0.42.0, 1080p | Completed: 268,281,336-byte MP4, 1920×1080 H.264 and AAC, 634.625 seconds |
| Fixed selector, 360p | Selected format 134; completed: 28,864,389-byte MP4, 640×360 H.264 and AAC, 634.625 seconds |
| 0.43.0 actual panel clicks | Opened Download this video, clicked Refresh and 360p; completed with identical SHA-256 to the fixed-selector test |
| Final rendered menu | Panel present; only detected qualities listed; corrected labels verified |
| Regression suites | 223 checks across nine suites, including 17 new readiness/label checks |

The new regression fails against the installed 0.42.0 selector. Two fixed 360p runs took 15.112 and 14.387 seconds from selection to observed completion, using a monotonic clock. These include native handoff, transfer, final assembly and polling; they are not bandwidth measurements or an IDM comparison. The 1080p download was a pre-fix baseline, not a 0.43.0 1080p acceptance run.

The matching completed 360p SHA-256 is `e47894654e205af1af162fc996063c97327bf125e7b0628fcdb8ac0018353af0`.

![Final Edge video menu](evidence-browser-0.43.0/edge-menu.png)

## Deployment and limits

The native executable and driver are unchanged. This update is compatible with the installed native 0.65.0 host. Edge uses `D:/UDM/browser/chromium`; equivalent source changes are mirrored to Firefox and the existing Chrome extension directory. The user confirmed reloading 0.42.0 before this investigation. Activating the new 0.43.0 files in existing browser sessions requires Reload in the extension manager and a page refresh. No new permission is requested.

Personal-window control failed before initialization with `failed to write kernel assets: The system cannot find the path specified. (os error 3)`. This is a control-tool problem, not evidence that the PC is locked. Isolated Edge testing worked.

This confirms the specific public-video workflow above. It does not establish complete IDM parity, every public video/site/quality, personal-session activation, live streaming, browser attestation compatibility, or an IDM speed advantage. Broader inventory remains in [native 0.65.0 parity](idm-parity-0.65.0.md).

Machine-readable results and deployment receipt are in [the evidence directory](evidence-browser-0.43.0/summary.json).
