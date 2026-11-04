# Live DASH caption integration audit — 2026-11-04

Current build baseline: native 1.12.1, extension source 0.63.84. This audit changes implementation evidence and the next work item; it adds no runtime feature and does not advance the application version.

The missing capability is live MP4 WebVTT caption capture through discovery, recording, recovery and final assembly. Simply exposing a subtitle option would fail native validation or assemble it as audio.

| Boundary | Current source evidence | Required implementation |
|---|---|---|
| Browser selection | browser/chromium/media.js::dashLiveCatalog emits empty subtitleOptions | Discover application/mp4 + wvtt tracks, expose language/role choices, preserve selection across MPD refreshes |
| Handoff | native/LiveDash.hpp::validateLiveDashPlan permits at most two tracks and requires video/audio order | Add one explicitly selected caption track, validate metadata and require a new desktop capability |
| Recording identity | LiveDash continuation compares selected plan and track identities | Bind caption representation, initialization version, media ranges and clocks into saved/recovered selection |
| Decoder reuse | native/Mp4WebVttAssembly.hpp::assembleMp4WebVttTrack consumes AdaptiveTransferLayout | Adapt retained live initialization/fragments into the decoder without guessing timestamps or dropping edit-list handling |
| Period assembly | native/DashManifest.cpp live assembly recognizes only video/audio and maps every non-video input as audio | Decode caption fragments, clip to the actual capture interval, shift Period clocks, join cues, then map an explicit subtitle stream |
| Final validation | Recorded Period assembly already verifies selected subtitle count | Extend live output validation and receipt identity to include caption codec/language and coverage |

DASH-IF IOP v4.3 requires ISOBMFF packaging for segmented subtitles; Part 9 identifies application/mp4 with wvtt for the WebVTT CMAF profile. Raw segmented VTT and TTML are separate contracts and should not be enabled by deleting the current browser guard.

Primary references:
- https://dashif.org/docs/DASH-IF-IOP-v4.3.pdf
- https://dashif.org/docs/IOP-Guidelines/DASH-IF-IOP-Part9-v5.0.0.pdf

Next implementation should start with the caption selection/identity contract and carry it through recording and assembly before enabling the browser choice. Required future acceptance includes language selection, nonzero presentation offsets, Period transitions, cancellation, continuation, gaps and decoded final subtitle timing. Runtime tests remain deferred under the standing user instruction. Existing source presence and successful builds do not establish these behaviors or full IDM parity.
