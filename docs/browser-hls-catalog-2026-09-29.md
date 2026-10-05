# UDM HLS quality catalog: tested browser 0.50 candidate

**The staged extension can now follow nested HLS video playlists and offer their actual leaf qualities with the correct audio groups.** This finishes the pending implementation and controlled Edge acceptance. It remains uninstalled; the personal installation is still native 0.74.0 / browser 0.47.1. This browser candidate is built on the [durable handoff/dialog candidate](browser-handoff-presentations-2026-09-29.md), separately from COM 0.75.

Previously, selecting a quality whose playlist led to another master caused UDM to reject the download. Catalog discovery now follows those branches before presenting choices. Child quality/codec metadata overrides inherited metadata when supplied. Audio and subtitle groups resolve in the playlist that declares them; a missing child group cannot silently borrow a same-named parent group. The chosen video leaf goes directly into the existing download path, paired with the selected rendition.

Parent and child playlists observed by the browser no longer create duplicate quality offers, regardless of their arrival order. A broken or cyclic branch can leave independent valid qualities available. A redirected playlist still needs permission for its destination host, and a navigation change invalidates the catalog before offers are saved.

Discovery is bounded: four concurrent playlist reads, a shared 64-playlist/eight MiB budget across observed roots, a 15-second cancellation deadline, six nested levels and 256 references per traversal. It reuses recent captured bodies and does not fetch audio bodies or media segments during discovery. It does read video media playlists, so this is not a claim of zero added startup work. Nested masters are compatibility behavior; the research has not established them as an IDM requirement.

## Tests completed

| Verification | Result |
|---|---|
| Extension/native-host regression suite | **1,163 checks across 44 scripts passed.** Includes 16 HLS traversal checks and seven production `sites.js` integration checks. |
| Actual isolated Edge and native app | **25 assertions across five completed downloads passed.** Real panel clicks, output decoding, language/tone verification and request traces. |
| Generated browser consistency | 35 shared Chromium/Firefox files match; both manifests are 0.50.0. Actual Firefox was not run for this change. |
| Native preservation | 242 staged native-tree files and five compiled binaries match the preceding presentation candidate. No C++ change, rebuild or new native-suite result is claimed. |

The five Edge downloads were:

1. Nested 720p MP4 with Spanish audio.
2. Nested audio-only M4A with Spanish audio and no video segment transfer.
3. Nested 360p TS with English audio.
4. Ordinary 360p MP4 with Spanish subtitles, exercising Back before download.
5. A valid 720p MP4 branch alongside a cyclic playlist reference.

All five outputs fully decoded. The harness checked actual dimensions, exactly one audio track, 440/880 Hz fixture tones, language tags and file types; subtitle checks covered Unicode text, timing and duplicate-cue removal. No unselected audio or overridden parent audio group was fetched. Collected file hashes match the completed native records. Screenshots were inspected for readable quality/audio/subtitle controls.

[Final Edge results](evidence-hls-catalog-20260929/hls-catalog-edge-2/results.json) · [Output hashes and streams](evidence-hls-catalog-20260929/outputs.json) · [Quality panel](evidence-hls-catalog-20260929/hls-catalog-edge-2/nested-MP4-720-catalog.png) · [Audio/subtitle selection](evidence-hls-catalog-20260929/hls-catalog-edge-2/flat-MP4-es-selection.png)

The first live run stopped at a stale fixture assertion that still expected 360p for the new 720p case. That failed run is retained; the corrected run above passed. An earlier unit assertion expected version 0.49, and another regression run could not launch three subprocess tests under the sandbox (`EPERM`). The final run with execution permission passed all 44 scripts. These failures and retries are not counted as additional coverage.

## Cleanup and remaining work

Independent read-only cleanup found both fixture native-host registry entries absent and no matching test processes. Product/browser/network files match their deployment receipts, accounting for the authorized research report update. The 25-record personal catalog is byte-identical to the fresh pre-test research audit. No certificate, driver or installed extension was changed.

The synthetic localhost tests do not establish public-site coverage, ad attribution or IDM speed equivalence. Nested audio rendition masters, broader live formats, wider refreshed-link transitions and unsupported encrypted streams remain outside this accepted scope. COM integration, controlled recognition, versioned paired packaging and activation in the user's Edge session are still required. Full IDM parity remains unestablished.

[Summary and source hashes](evidence-hls-catalog-20260929/summary.json) · [Source diff](evidence-hls-catalog-20260929/source-review.diff) · [Regression results](evidence-hls-catalog-20260929/unit-results-final.json) · [Cleanup](evidence-hls-catalog-20260929/cleanup.json)
