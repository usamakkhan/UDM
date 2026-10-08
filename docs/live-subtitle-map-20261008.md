# Live subtitle initialization support — 2026-10-08

UDM now records clear live HLS WebVTT captions whose header is stored separately through EXT-X-MAP. Both extension bundles pass the selected playlist to the native recorder only when the desktop advertises live-hls-subtitle-map. Browser source version: 0.62.11.

The previous backend reproduced the rejection with a valid header-and-cues fixture. The recorder now downloads and retains the initialization resource using its existing protected receipts, validates the header independently, combines it with each matching segment, and preserves timestamp-map changes. Input remains bounded to 2 MiB per combined header/segment and 16 MiB per recording's interpreted subtitles. Malformed headers, cue data placed in an initialization header, missing resources and oversized headers cannot publish output.

Validation: 126 focused native checks passed, including selected-track metadata, cue times, header changes and offline catalog-reload recovery. Both extension bundles passed 17 combined handoff checks, including rejection by older native capabilities. Real isolated Edge passed 7 focused checks and Firefox passed 6; each downloaded the header once, preserved the captions and timing in a decoded MP4, and removed its temporary native registration. Edge's fixture-generation log includes an FFmpeg playlist rename warning; the final output and ENDLIST assertions passed. No equivalent warning appeared in Firefox.

Native build scope: Adaptive.cpp and Bridge.cpp were rebuilt and the four desktop programs linked against unchanged objects from the previous live-subtitle candidate. This is not a fresh clean build or full-native-suite rerun. Candidate: candidates/live-subtitle-map-20261008/release-native. [Hashes and evidence](validation/live-subtitle-map-20261008.json).

Not installed, packaged or published. The latest installer still contains browser 0.62.10 and the earlier native backend. All-empty caption tracks, recorded non-live playlists with separate WebVTT headers, public-site coverage and direct IDM comparison remain open. Full parity is not established.

Protocol basis: [RFC 8216 section 3.5](https://www.rfc-editor.org/rfc/rfc8216.html#section-3.5) permits a WebVTT header supplied through EXT-X-MAP and defines synchronization through X-TIMESTAMP-MAP. This implements that permitted arrangement; it does not claim observed IDM behavior for this fixture.
