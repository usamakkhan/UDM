# Awesome Downloader comparison — 2026-10-26

Reviewed public source at [f19ae6d2c944d920cbbb23fdf539d395419caa6d](https://github.com/Ahmadkhattak1/awesome-downloader/tree/f19ae6d2c944d920cbbb23fdf539d395419caa6d), against UDM HEAD 7209abe plus existing local edits. This is a source review, not a comparative throughput benchmark or a full audit of either project.

## Claim and license

The public README describes an original independent implementation and explicitly says it is not a complete feature-for-feature IDM replacement. The reviewed source does not substantiate a claim of complete IDM reverse engineering. Its core uses Node HTTP, Electron, FFmpeg and the GoogleVideo SABR library; its optional legacy watch-URL path uses yt-dlp. None of those establishes recovered IDM internals.

Original upstream source is MIT licensed. The adapted helper retains upstream copyright and the complete MIT license in both browser distributions, referenced by NOTICE. No upstream executable, proprietary IDM material, dependency tree or UI artwork was imported.

## Findings

| Area | Upstream implementation | UDM comparison / action |
| --- | --- | --- |
| Feed video association | `extension/media-choices.js` associates twimg media URLs with poster asset IDs and filters other tweet assets. | Useful missing behavior: UDM previously rejected blob sources whenever multiple visible players shared a frame. Adapted the asset matching rule. |
| HTTP segments | `core/engine.mjs` allocates fixed ranges and validates saved parts against probe identity. | UDM already has range validation, dynamic splitting and retry controls. No reason found to replace its engine with this implementation. No speed ranking measured. |
| Streaming formats | `core/engine.mjs` rejects DASH in the ordinary download path; HLS supports separate audio. | UDM already has native DASH planning, HLS rendition selection, subtitles and live support. Copying this path would lose capabilities. |
| YouTube | `core/sabr.mjs` uses GoogleVideo with captured browser context; totals derive from declared track content lengths. | Does not demonstrate IDM's internal algorithm or a faster exact final muxed-size solution. No replacement made. |
| HLS bitrate | `extension/hls.js` prefers average bandwidth when available. | Potential display refinement; not implemented here. Peak bandwidth also has a distinct meaning, so it should not be silently replaced throughout UDM. |
| Audio conversion | `extension/media-choices.js` offers MP3 output for paired external audio. | A potential separate transcoding feature. Not imported as a substitute for UDM's existing audio-only/remux path. |

## Implemented change

Both Chromium and Firefox extension sources are now version 0.63.38. `sites.js` reads the selected video's poster and includes it in the offer identity. For blob players with a recognized twimg asset, observed manifests and direct files must match that exact asset family and ID. Existing frame, document, age and page checks still apply. Unrecognized multi-player sources remain rejected; a missing matching asset never falls back to another feed video. Changing the poster invalidates previously offered downloads.

The adapted `twitterAsset` helper is stricter than upstream: HTTPS only, no URL credentials, an anchored path, bounded numeric IDs and separate asset families. It does not fetch posters or introduce new browser permissions.

## Validation

- `node tests/media.test.cjs`: 37 passing checks on Chromium.
- `UDM_TEST_BROWSER=firefox node tests/media.test.cjs`: 37 passing checks on Firefox sources (environment-variable syntax depends on shell).
- `node tests/hls-catalog-sites.test.cjs`: 57 passing checks.
- `node tests/integration-parity.test.cjs`: 33 passing checks, including five native handshake cases.
- `git diff --check`: passed.

New checks cover spoofed hosts, credentials, asset families, multi-player selection, wrong document/frame exclusion, poster-change invalidation, direct-file handoff and no-match behavior. These are local harness tests; no live X/Twitter download or performance benchmark was performed. Version 0.63.38 has not been installed or reloaded in the user's browser in this task. No desktop executable changes were required.

Existing edits to `.gitignore`, `native/GrabberUi.hpp` and `native/GrabberResultUi.hpp` were preserved.

## Follow-up: complete the feed-quality adaptation

Upstream HEAD was rechecked and remains `f19ae6d2c944d920cbbb23fdf539d395419caa6d`.
The additional adapted behavior comes from `extension/background.js:isFragment` and
`extension/media-choices.js`: omit segment/init requests from complete-video choices,
and prefer a successfully parsed richer catalog over redundant raw feed entries.
Both extension sources are now **0.63.39**. The original 0.63.38 result above is historical.

A new regression failed before the correction: four entries were offered where only
the complete video should remain (TS fragment, AAC fragment, initialization MP4,
complete MP4). It now passes. Filtering applies only to inferred feed requests;
explicit TS/AAC file selections are not globally disabled. A failed master keeps the
complete direct fallback. Successful catalogs are order independent.

### IDM evidence boundary

IDM's official [video guide](https://www.internetdownloadmanager.com/register/new_faq/video5.html)
describes choosing video downloads from the player, and its
[release history](https://www.internetdownloadmanager.com/news.html) documents TS/MP4
assembly and separate audio/video handling. These support the product behavior of
presenting complete video choices. They do **not** prove that IDM uses twimg poster IDs,
the upstream filtering algorithm, or identical behavior on a current X feed. No such
implementation equivalence is claimed. This change completes a useful licensed
adaptation in that workflow; it is not a newly proven full IDM feature gap.

Additional candidates were not blindly imported: MP3 transcoding lacks established IDM
equivalence; average HLS bandwidth has different semantics from peak bandwidth;
segmentation/resume, audio/video merging, HLS renditions, duplicate handling and progress
windows already have UDM paths. Upstream's narrower HLS engine rejects byte ranges,
changing maps and live streams that UDM already handles.

### New validation

- Chromium source media suite: 42 passed.
- Firefox source media suite: 42 passed.
- HLS site integration: 57 passed.
- Native messaging/integration harness: 33 passed (nested handshake cases counted once).
- Site deadline: 28 passed.
- Audio tracks: 31 passed.
- New `tests/feed-capture.browser.cjs`: 10 passed in real headless Chrome, five per
  extension source variant. It uses actual video elements and the production
  `readPlayer` function, checks the selected player among two videos, complete-file
  handoff, MP4/TS quality choices, poster-change invalidation and failed-master fallback.
- Total: **243 passing checks**. `git diff --check` passed.

The browser fixture mocks browser-extension APIs, HTTP playlist responses and native
handoff; it is not a Firefox-engine test or an installed-extension/end-to-end download.
At the end of automated validation, no live X session had been accessed and 0.63.39
was source-only. Required MIT notices remain in both extension distributions and NOTICE.

### Installed browser acceptance follow-up

Version 0.63.39 is now installed for Chromium and Firefox. All eight changed files
match source SHA-256 hashes; the installer verified backups. The user confirmed
reloading Chrome. The desktop executable was not changed.

The live test of NASA's public post `2113776971603689874` did **not** pass capture.
Its 59.893-second video uses a custom `pbs.twimg.com/media/` thumbnail, which has
no matching video asset ID. With a separate 0.84-second reply GIF on the page,
UDM displayed its ambiguous-player message and offered no download. This exposes
a coverage gap in the poster-ID approach; the automated fixtures do not establish
general X compatibility. No incorrect reply video was selected in this attempt.

Browser control subsequently disconnected and repeated navigation timed out.
Chrome restart was requested. Complete-file, duration and full-decode verification
remain pending; this installed live attempt is not an end-to-end success.

### Custom-thumbnail fallback candidate (0.63.40)

The selected X player's ancestor React props can supply a media record even when
its thumbnail has no video asset ID. The new fallback searches a bounded number
of objects and returns only video variants whose record matches the canonical
thumbnail URL and loaded playback duration. It rejects conflicting matching
records, non-Twitter media hosts and stale selections. It does not copy account
data or infer ownership from network request order. This is a new UDM fallback,
not an assertion about IDM's implementation.

Chromium and Firefox sources passed 20 real-browser fixture assertions, 84 media
checks, 57 HLS site checks and 28 deadline checks. Both installed extension variants
were updated to 0.63.40 and all four changed files were hash-verified. Chrome reload
was requested. The live X metadata layout and completed-file download remain
unverified; see `validation/x-custom-thumbnail-20261026.json`.

### Live correction (0.63.41)

The installed 0.63.40 live test still displayed the ambiguous-player message.
Public NASA page HTML exposed the concrete mismatch: the player thumbnail uses
an extensionless URL with a WebP query, but the media record uses the same image
ID ending in `.jpg`. X's logged-out page also embeds the record in serialized
router data. Version 0.63.41 normalizes image extensions and safely JSON-decodes
matching string literals from bounded `data-tsr-stream-part` scripts. It never
executes those scripts. Exhausting the object-search budget rejects the match.

The JPG regression failed before the correction. Afterward, 24 browser fixture
checks and 42 media checks passed. An isolated DOM fixture using NASA's actual
public serialized page data identified all five variants belonging to video
asset `2113776813121941504`, excluding the reply GIF. This validates extraction
against actual site data, but does not prove the installed download workflow.
Version 0.63.41 is installed with four files hash-verified; reload and complete-file
verification remain pending.

### Installed 0.63.41 acceptance result

After the user reloaded 0.63.41, the NASA post offered its MP4/TS quality choices.
Selecting **1080p MP4 with audio** opened UDM File Info with the correct NASA
asset ID. Start Download completed job `bae1286347c24ae0b0f069fb80ef1943`.
The resulting file contains 21,169,896 bytes, 1920×1080 H.264 video lasting
59.893167 seconds and AAC audio lasting 59.935833 seconds. The video duration
matches the page's 59.893 seconds. FFmpeg decoded every audio/video stream to
completion with `-xerror` and exit code 0. A frame at 10 seconds matches NASA's
rocket hardware footage, not the 0.84-second reply GIF.

The installed Chrome-to-UDM complete-download test now passes for this post and
selection. File Info still showed **Unknown** before Start; pre-download size,
other qualities, other X layouts and Firefox-engine behavior are not established
by this acceptance test. Sanitized details are in
`validation/x-custom-thumbnail-20261026.json`.

### Original MP4 exact-size verification (0.63.42)

X supplies complete MP4 variants in addition to HLS. The extension now labels
these with their resolution and `Original file`, retaining them ahead of HLS
at the same resolution. UDM's existing direct-file probe can obtain their exact
server length without estimating remux overhead. HLS choices remain available;
this does not add general exact-size calculation for HLS.

The user selected NASA's original 1080p MP4 in Chrome and left File Info open.
Before Start, the dialog displayed **20.16 MB** and the pending job's exact Size
was **21,138,063 bytes**. ConfirmationPending was true and background prefetch
had received all 21,138,063 bytes. After Start, job
`a5b7a68dec2e42ef80350b00ae1cfb86` completed with a file of exactly
**21,138,063 bytes**, a zero-byte difference. FFprobe confirms 1920×1080 H.264
video and AAC audio, both approximately 59.893 seconds. The UI's MB label is
rounded; the comparison uses its backing byte count recorded before confirmation.
See `validation/x-original-size-20261026.json` for decode status and test limits.
