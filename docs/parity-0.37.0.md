# UDM 0.37.0 — recorded-media outputs

The desktop is 0.37.0; Chromium and Firefox extensions are 0.26.0. The signed network backend remains 0.3. This release adds working recorded-media workflows. It does not establish complete IDM parity.

## Implemented

- Audio-only M4A output for supported clear recorded HLS and static MP4 DASH. The browser offers an explicit **Audio only (M4A)…** action when the manifest identifies audio. The user selects the language, then **Download audio (M4A)**. For external HLS/DASH audio, the handoff excludes video segment URLs and the downloader does not fetch them. In-band audio is extracted from its shared media input. The output is remuxed without audio re-encoding; an incompatible audio codec or missing audio fails explicitly.
- Optional WebVTT subtitles in MP4: HLS subtitle groups are bound to the selected video variant; static DASH supports standalone `text/vtt` representations. Choices carry opaque keys, not signed playback URLs. The selection is revalidated against the frame, document, player and capture expiration before any handoff.
- Subtitle downloads use the existing parallel segment workers, retry handling, encrypted capture storage, exact-origin headers/cookies, verified checkpoints and collision-safe output publication. One selected subtitle stream is converted into MP4 text, with its language/name in both metadata and Download Properties.
- A bounded native WebVTT parser validates UTF-8, timestamps and input size, handles HLS `X-TIMESTAMP-MAP` and 33-bit MPEGTS clock wrap, normalizes timestamps to the input media clock, and deduplicates identical cues across segment boundaries. Incorrect/missing tracks block publication.
- Simple video formats retain their one-click handoff. Multiple audio tracks or available subtitles open the track-selection panel. **Download all** retains default audio and no subtitles.

## Validation

- Native build and complete regression suite: **735 passed, 0 failed**.
- Media browser logic: **31 audio/subtitle checks** and **32 cross-site checks** passed.
- Native message framing/protocol: **7 passed**.
- Real isolated Edge and Chrome sessions used synthetic 360p recorded HLS/DASH with English 440 Hz and Spanish 880 Hz audio. Each browser passed **22 checks**: actual extension clicks, native completion, decoded audio frequency, no unselected rendition requests, audio-only requests without video segments, TS regression, language tags, embedded subtitle text/timing and duplicate-cue suppression.
- Firefox uses the same reviewed media code; its isolated native test covers direct video, one-click HLS and M4A audio-only output. See the saved Firefox results for the actual outcome.

Evidence is kept in `benchmarks/parity-0.37.0/`. The first sandboxed native run was blocked replacing its test state; the normal-access rerun and final build passed. No user download state was used by these fixtures.

## Explicit limits

Subtitles currently support self-contained recorded HLS WebVTT segments and standalone static DASH WebVTT. Subtitle initialization maps, TTML/IMSC, fragmented MP4 text, subtitle layout/style parity, multiple simultaneous subtitle tracks and TS subtitles are not implemented. Discontinuous/live HLS and encrypted media remain unsupported. Audio-only selection in this release is for the generic HLS/DASH workflow, not YouTube's separate SABR/player path. Multiple in-band audio tracks remain unqualified.

Other open parity work remains in [the 103-workflow inventory](idm-parity-0.37.0.md): localization, legacy driver protocols and automatic driver handoff, remaining proxy modes, rendered/authenticated site mirroring, broader site coverage, physical wake/disk-exhaustion cases, accessibility/DPI acceptance, publisher signing/update distribution, and controlled same-route speed comparison. No equality percentage or universal speed claim is justified by these tests.

The independently implemented subtitle behavior follows [RFC 8216 §3.5](https://www.rfc-editor.org/rfc/rfc8216.html#section-3.5) and the [W3C WebVTT format](https://www.w3.org/TR/webvtt1/). MP4 subtitle styling can differ from the original WebVTT presentation.
