# Cross-site video in UDM 0.8

The compact 168 × 24 CSS-pixel panel follows the upper-right edge of each visible HTML5 video. Small players use an icon. Drag the handle to adjust its position; the menu provides reset, compact mode and hide-until-reload. Scrolling, resizing, embedded frames, open shadow roots and container fullscreen are handled. Native video fullscreen and picture-in-picture cannot contain this DOM panel.

Use the extension popup to enable panels on the current site or all websites. This requests Chrome's optional host permission. Only granted sites receive the content script. A separately hosted iframe/media playlist may need permission for its host too. Cookies and automatic browser-download takeover remain separate opt-ins.

## Supported paths

| Source | Processing | Boundary |
| --- | --- | --- |
| Direct HTML5 video | Current player's file URL; native range engine | HTTP(S), server access required |
| Recorded clear HLS | Actual master variants/audio groups; media playlist segments/ranges/init | ENDLIST required; no encryption, discontinuities or live updates |
| Static MP4 DASH | Representations, SegmentTemplate/Timeline/List and BaseURL | One period; no DRM, SegmentBase or multiple BaseURLs |
| YouTube watch/Shorts/embed | Current video identity and captured formats; original SABR path where available | Capture-only; live compatibility remains experimental |

The extension sends real segment URLs to the C++ engine. It does not run yt-dlp. Up to 16 native workers fetch segments, verify explicit byte ranges and preserve completed parts with SHA-256 for resume. FFmpeg muxes local files only; FFprobe verifies required tracks and selected video height. A failed quality/hash check leaves no published output. Original and partially assembled files are retained on failure.

Bounds: 2 MB playlist, 200 KB plan, 1,200 segments total in the native plan, 256 MB per segment and at most one selected video plus one audio track. Large single-file DASH representations can exceed the segment bound. Not all codecs can be copied into MP4. Authenticated playlists, multi-period/ad-spliced presentations, live streams, DRM, closed shadow players and arbitrary JavaScript streams remain unsupported.

## Stream association

Direct sources belong to the clicked video. Offers are tied to tab, frame, page, player token, source and load epoch and expire after three minutes. A source replacement invalidates the previous choices. For blob players, observed HLS/DASH requests are considered only when that frame has one visible video. The menu identifies playlist filenames and warns that this heuristic cannot universally distinguish embedded advertising. Multiple blob players in one frame are rejected instead of guessing.

YouTube has its separate video/resource identity checks. Visible known ad-player states hide the panel. This is not a claim of universal ad exclusion.

## Checks and reproduction

`tests/media.test.cjs` checks manifests, bounds, encryption rejection, real quality metadata, panel geometry, frame isolation, stale offers and native plan handoff. `native/AdaptiveChecks.hpp` exercises native HTTP segment transfer, byte ranges, resume integrity, MP4 assembly, output height/audio, hash validation and collision handling. These controlled checks do not prove compatibility with every commercial platform.

`node native/browser-fixture.cjs` generates a synthetic clip and serves direct, embedded and HLS players at `http://127.0.0.1:43821`. It serves only fixture files on loopback and stops accepting connections after 40 minutes. Generated media is excluded from source packages.

## Reference basis

Placement follows the observed player-edge workflow described by [IDM's video panel help](https://www.internetdownloadmanager.com/register/new_faq/video3.html). Implementation is original UDM code. Host permission/content-script behavior follows [Chrome's content-script documentation](https://developer.chrome.com/docs/extensions/develop/concepts/content-scripts) and [permission documentation](https://developer.chrome.com/docs/extensions/develop/concepts/declare-permissions). Playlist parsing follows [Apple's multivariant HLS guidance](https://developer.apple.com/documentation/http-live-streaming/creating-a-multivariant-playlist) and the supported subset of [DASH-IF interoperability guidance](https://dashif.org/docs/DASH-IF-IOP-v4.2-clean.htm).
