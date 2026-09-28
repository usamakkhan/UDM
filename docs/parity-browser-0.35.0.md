# Native UDM 0.42.0 / browser 0.35.0: captured YouTube audio

The video panel now offers **Audio only (M4A)** when the current YouTube player exposes a valid direct AAC/MP4 audio stream. The selector shows the available track name, codec and bitrate. The highest-rate default track is initially selected; Back returns to the format menu without starting a download. Audio-only selection uses the ordinary native parallel downloader and does not download or mux a video track. The source page is retained for download properties and link-refresh workflows.

The early capture catalog now retains bitrate metadata and separates audio tracks sharing an itag by track ID and stream tags. A changed known format revision cannot inherit an older direct URL, and the current player snapshot takes precedence over the initial page snapshot. Matching observed playback requires the audio language tags and revision to match. The shared audio selector now selects the first preferred track consistently when multiple defaults are declared.

## Verification

**534 browser checks across 22 suites and 46 live fixture checks passed.** [Results, scope and request records](evidence-browser-0.35.0/summary.json) are included. Native binaries were unchanged; no historical native test count is included.

YouTube parser/handoff checks cover same-itag languages, codec filtering, expiry, invalid hosts/ports/userinfo, signed ranges, transformed playback URLs, stream revisions, duplicated ambiguous formats, navigation, advertisements, private tabs, policy rejection and native errors. The real page reader and early capture observer are tested as well as the handoff.

Actual Chrome and Edge tests click the audio panel, language selector, Back and Download controls. Their YouTube page is synthetic. The production Googlevideo handoff URL is asserted, then a test-only boundary adapter substitutes a loopback file for the native transfer. This isolates UI and native correctness without claiming live CDN acceptance. Two 180-second AAC outputs per browser are byte-identical to their selected sources, fully decode, contain no video, and reach four simultaneous native range requests. No other language is fetched. [English selector](evidence-browser-0.35.0/en-selection.png) and [Spanish selector](evidence-browser-0.35.0/es-selection.png) are recorded.

The shared HLS/DASH selector was also tested through real Edge/native downloads: selected English/Spanish audio, M4A, MP4, MPEG-TS, WebVTT subtitle timing/Unicode/language and Back behavior all passed. Firefox receives identical shared sources and passes extension preparation checks; this release does not claim a fresh Firefox UI/native qualification.

## Actual YouTube result and remaining scope

A separate public YouTube test used the production extension and a fresh Edge profile. It detected 144p–1080p video qualities, but **that session exposed no eligible direct AAC file**, so no audio-only download was started. See the [live service result](evidence-browser-0.35.0/youtube-service.json). This test is not counted as a successful audio download.

Audio-only is currently limited to validated captured direct AAC/MP4 streams. Cipher-only, SABR-only, live, protected, WebM/Opus and YouTube subtitle workflows remain outside this addition. No fake audio option or video-to-audio download is substituted. Other existing video transports continue to use their existing path.

The [103-workflow comparison](idm-parity-browser-0.35.0.md) remains 88 implemented, 12 partial, 2 unverified and 1 user-deferred. F077 is improved but still partial. Full IDM parity and matched-source speed equality remain unestablished.

## Package and activation

[UDM-0.42.0-Browser-0.35.0-Setup-x64.exe](../installer-out/UDM-0.42.0-Browser-0.35.0-Setup-x64.exe) contains unchanged native 0.42.0 binaries and browser 0.35.0. SHA-256: `14E0F2C6790959814F2B38C82039777C6C9038B8C359CB5625A39CF4D1FA7438`.

The tested sources and evidence are installed in D:\UDM, with backups and prior installers retained. Chrome and Edge were reloaded to 0.35.0, remain enabled, and each visibly reported "UDM is connected and ready" after its desktop connection check. The running app showed 23 downloads and zero active transfers; all 23 records remain byte-identical. No native binary, driver or Test Mode setting changed. Firefox remains a shared-source update without new persistent-install qualification. See [deployment evidence](evidence-browser-0.35.0/deployment.json).

This is current-PC source/package deployment, not clean-machine installation or publisher-signature qualification.
