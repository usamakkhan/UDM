# Browser integration 0.28.0: indexed DASH

27 September 2026. Compatible with the existing native UDM 0.38.0 engine.

## Result

Previously a recorded MPD using SegmentBase was rejected even when its streams were clear MP4. UDM now reads the selected representations' SIDX indexes, validates their absolute byte ranges, and hands those ranges to the existing parallel native adaptive downloader. Video plus the chosen audio produces MP4; audio-only produces M4A without requesting the video track. Formats are listed before indexes are fetched.

The parser was independently implemented from the [DASH-IF timing model and indexed-addressing description](https://dashif.org/Guidelines-TimingModel/Timing-Model.pdf). No IDM code or drivers are involved.

## Verification

- **345 automated checks passed** across 17 test scripts: 40 new DASH index/handoff checks, 32 cross-site, 31 audio, 54 browser, 12 Dailymotion, 7 policy, 20 integration, 28 transport, 18 video capture, 4 capture, 10 audit, 28 POST context, 6 POST handoff, 40 automatic capture, 8 real native protocol, 1 packaging preparation, and 6 UMP.
- **29 isolated live checks passed:** Chrome 10, Edge 10, Firefox 9. Chrome/Edge clicked the real panel and audio selector. Firefox used a temporary test-only observer calling the real page-scoped discovery/handoff, not a visual button test. Every run verified its separate native data directory first.
- A generated 360p, eight-second video with English 440 Hz and Spanish 880 Hz audio produced MP4 and M4A containing one Spanish track. Decoding measured **880 Hz** and **8.021333 seconds**.
- Video used **10 exact segments**; audio-only used **5**. All three browsers observed **4 simultaneous media requests**, with native completed/total segment counts matching.
- No unselected English media or audio-only video ranges were requested. A server returning HTTP 200 instead of the requested 206 index response created no native job.
- Test evidence: [unit/regression log](../benchmarks/browser-0.28.0/regressions.log), [native protocol rerun](../benchmarks/browser-0.28.0/native-protocol-retry.log), [preparation rerun](../benchmarks/browser-0.28.0/prepare-retry.log), [Chrome](../benchmarks/browser-0.28.0/chrome-results.json), [Edge](../benchmarks/browser-0.28.0/edge-results.json), [Firefox](../benchmarks/browser-0.28.0/firefox-results.json). The initial combined run hit process-launch restrictions and an incorrect preparation-fixture path; both checks passed after correcting their environment.

## Bounds and error handling

Version 0/1 SIDX and extended box headers are supported. References must be direct, within the returned resource length and exactly representable. Index reads are capped at 512 KiB and 12 seconds; media parts retain the native 256 MiB cap and total selected plans the 1,200-segment cap. Malformed, nested, duplicated or truncated indexes are rejected. Index requests require current host permission and exact Content-Range/length; redirects are refused. Cookie/auth transfer retains the existing explicit opt-in and origin restrictions. The document/player identity is checked again before native handoff.

## Scope still open

This is tested support for clear, recorded, single-period MP4 DASH with direct SIDX references. External RepresentationIndex, nested indexes, live recording, DRM, multiplexed in-band alternate tracks, redirected index URLs and general manifest coverage remain open. Native byte-range checks do not add an ETag snapshot binding between the index fetch and later media requests. The synthetic test verifies transfer correctness and concurrency, not an IDM speed comparison or arbitrary commercial video-site compatibility. Parity rows F075/F077 remain Partial; overall inventory counts are unchanged.

## Activation and package

Chrome and Edge were reloaded through their existing extension-management pages. Both display **0.28.0**, remain enabled and report **“UDM is connected and ready”** after an explicit connection check. Firefox 0.28.0 is prepared and verified in its isolated test profile; no persistent Firefox installation is claimed.

Native executables remain 0.38.0. The existing 23-record user catalog stayed byte-identical, SHA-256 AAE5B9BF965F9850CC771232D988052FF5083FD809EA711B1520FB10A0ECC851. Changed project files and the prior setup are backed up in the local staging folder.

The rebuilt [local setup](../installer-out/UDM-0.38.0-Setup-x64.exe) bundles extension 0.28.0 and unchanged native 0.38.0 executables. Inno Setup compilation succeeded; SHA-256 ED4C5F989B3E97C51E353E675C882231AA771DB1754DBC162D593CEF26608F76. The installer was copied and hash-verified, not run over the active development installation. No release was published.

[Deployment receipt](../benchmarks/browser-0.28.0/deployment.json) and [activation/package receipt](../benchmarks/browser-0.28.0/activation.json).

