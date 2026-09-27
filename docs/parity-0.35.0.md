# UDM 0.35.0 / extension 0.25.0: choose alternate audio

UDM previously selected one audio rendition automatically from HLS and static DASH manifests. The panel now exposes available languages and alternate tracks and passes the selected track to the native downloader.

## Using it

For a supported recorded video, open **Download this video** and select a quality/container. If that quality has multiple audio tracks, choose **Audio track**, then **Download video**. **Back** returns to quality choices without creating a download. Single-track videos keep their one-click flow. **Download all (default audio)** uses each quality's default track.

The output contains one selected audio track together with the video. File Properties/Information records its name and original language tag; MP4/TS language metadata uses a three-letter ISO code where Windows can map it. MP4 retains the selected track name. This is video-plus-audio downloading, not audio-only extraction.

## Behavior

- HLS choices use only the quality's declared audio group, including external audio playlists and a single in-band rendition. Selection prefers DEFAULT, then AUTOSELECT, then source order when no default is declared. Missing groups, duplicate names/defaults, multiple indistinguishable in-band tracks and oversized catalogs are rejected explicitly.
- Static MP4 DASH preserves supported audio representations, labels, languages and commentary roles. A main-role audio representation is preferred before bitrate. Reused representation IDs in separate sets do not collapse distinct audio choices.
- Audio choices are private cached records attached to the current video offer. The page receives labels and keys, not playback URLs. Forged/stale keys fail before fetching audio or handing off to the desktop. Navigation invalidates the selection together with the video.
- The downloader receives only the chosen audio plan and fetches its segments. It does not fetch other language renditions. Existing native parallel segment downloading, resume verification, origin-scoped credentials, quality verification and publication checks remain in use.
- Audio metadata is bounded and validated by the desktop. Untrusted labels are displayed as text; ffmpeg receives argument vectors rather than a shell command. Required audio is checked before publication.
- Limits remain explicit: clear recorded HLS/static MP4 DASH, up to 32 audio options per quality, existing native 1200-segment and 200 KB handoff bounds, and a 4 MB browser offer-cache bound. Unsupported DRM, live manifests and multi-period DASH remain rejected.

Reference protocol: [RFC 8216](https://www.rfc-editor.org/rfc/rfc8216.html) defines HLS audio groups, optional URI, languages and defaults. [Windows locale constants](https://learn.microsoft.com/en-us/windows/win32/intl/locale-information-constants) provide the language conversion used in container metadata. This is original UDM implementation; no proprietary code or assets were copied.

## Validation

| Suite | Passed | Evidence |
|---|---:|---|
| Full native regression | 708 | Includes ten new audio-description validation, native metadata and MP4 language/name checks |
| Browser model/capture/package regression | 225 | Thirteen suites, including 18 new audio-group/default/selection/stale-offer checks and shared Chromium/Firefox preparation |
| Real Edge audio downloads | 12 | Trusted picker/Back actions, HLS MP4/TS and DASH MP4, chosen language, decoded audio signal, no unselected-audio requests |
| Real Chrome audio downloads | 12 | Same independent browser acceptance against isolated native app/history |
| Native launch workflows | 7 | Cold/warm handoff, confirmation and queued transfers |
| Native messaging | 7 | Persistent/fragmented framing and rejected malformed requests |

**971 checks passed before installation.** Final audio runs use the button caption Download video; earlier overlapping runs are not counted. Tests generate benign eight-second video with 440 Hz English and 880 Hz Spanish audio, decode the downloaded audio, and verify the actual chosen signal and language metadata. Screenshots were inspected for the new browser picker. These fixtures do not establish coverage of every real site/session.

All four native binaries build. Logs include deprecated-address and test-variable-shadow warnings. Package-preparation needed unrestricted child-process execution after sandbox EPERM; the passing run used a fresh temporary folder. Firefox's bundle matches canonical scripts and keeps its identity, but this release has no new live Firefox runtime test.

## Installation

Installed and running as **UDM 0.35.0**. All **32 installed files** match their deployment hashes, including the Chromium folder used by each browser. All **23 download records, queues and settings are unchanged**, including after browser testing. The native host confirms 0.35.0 and the normal user-history directory. Backups are in `parity-audio-tracks-20260927/backup-before-0.35.0`.

Installed Edge integration passed **13 additional checks** with a separate profile and history: persistent messaging, Download Later, authenticated range transfer, byte-identical MP4, recorded HLS/audio, modifier gestures and stale iframe exclusion. **Final verification: 984 checks passed**, without counting overlapping runs twice.

Chrome's current unpacked extension points to the original C-drive project folder; Edge points to D:/UDM/browser/chromium. Both source locations are included in the guarded update. Existing browser sessions must reload their unpacked UDM extension and refresh video tabs to activate changed scripts. Browser-control initialization currently fails with `failed to write kernel assets: The system cannot find the path specified. (os error 3)`, so activation in the user's existing profiles cannot be verified automatically. Isolated Chrome/Edge profiles loaded and tested 0.25.0 successfully.

## Remaining acceptance

The [103-workflow audit](idm-parity-0.35.0.md) now has **87 implemented, 13 partial, 2 unverified and 1 missing row**. The combined live/subtitle/alternate-track row moved from Missing to Partial. Live recording, subtitles, multiple in-band HLS audio selections, YouTube-specific alternate audio, localization, other remaining proxy modes, signed distribution, physical wake tests and performance/accessibility acceptance remain open. Complete IDM parity is not claimed.

Test evidence, browser screenshots, deployment hashes and backups are retained under `parity-audio-tracks-20260927` in the writable visualization workspace.

The user confirmed reloading the Chrome and Edge extensions and refreshing video tabs after installation. Version 0.25.0 passed isolated live browser tests; visual inspection of the existing user profiles remains unavailable because browser-control initialization fails.
