# UDM 0.57.0 / browser 0.39.0

This update adds native recording of exposed clear live HLS playlists, with Stop and save, pause/recovery and browser handoff. Full IDM parity remains open.

## Recording workflow

- Select an exposed live HLS stream in the browser panel. The extension verifies desktop capability and sends the selected playlist and origin-scoped request context. The desktop refreshes the playlist and downloads newly available full segments through parallel native HTTP workers.
- Download progress displays captured duration without a fabricated percentage or ETA. Stop and save publishes the playable captured portion. Its context menu offers Pause/Resume recording separately. Saving becomes available only after every selected track has a playable prefix.
- Server ENDLIST finishes naturally. Stop and save interrupts stalled HTTP headers or reads. Temporary playlist/segment rejections respect Retry-After and the queue retry budget; permanent forbidden responses preserve media without retry loops.
- Encrypted recovery receipts identify each segment and initialization section. Restart and finalization verify hashes. Saved media can be published offline after expiration or a destination conflict; successful publication retires only recorder-owned temporary files.
- Separate video/audio inputs retain either their media clock alignment or a program-date offset. Discontinuities and changed initialization sections survive native capture and local assembly. FFmpeg receives only generated local playlists, with network protocols disabled.

IDM's [release notes](https://www.internetdownloadmanager.com/news.html) document stop-and-save for live broadcasts. Playlist continuity follows [RFC 8216](https://www.rfc-editor.org/rfc/rfc8216). This is an independent implementation of that workflow, not a claim about IDM's private transport.

## Verification

**1,461 checks passed:** 1,273 native, 140 browser unit checks, 8 native protocol checks, 8 isolated app/host checks, and 32 actual extension/native scenarios across Chrome and Edge.

The native fixtures use generated TS and fragmented MP4 media. They cover rolling windows, actual overlapping segment requests, full decoding, Stop and save, expired sources, gaps, restart, Retry-After, stalled reads, destination collisions and cache cleanup. Separate audio/video tests preserve a deliberate 0.5-second offset. A changed-initialization fixture with an encoder timestamp reset produces the full eight-second decodable output. Windows font measurement checks live action captions at 96, 120, 144 and 192 DPI.

The full run also exposed an old scheduling-dependent SABR test assumption: the next request cursor was mistaken for the buffered start. The fixture now deterministically seeds a saved prefix and verifies serial fallback with both video/audio and audio-only sessions. Production SABR behavior was retained.

Chrome and Edge each passed a trusted panel-to-native live fMP4 capture, full output decoding and hashing, plus existing recorded HLS, direct MP4, file capture, duplicate overwrite, request credentials, force/bypass keys and navigation checks. [Evidence summary](evidence-0.57.0/summary.json), [native log](evidence-0.57.0/build-release-candidate.log), [workflow inventory](idm-parity-0.57.0.md).

## Current limits

This release qualifies exposed clear full-segment HLS. Live DASH/SABR/YouTube, encrypted or DRM media, live subtitles, delta playlists, partial-segment capture and Dailymotion's live-player metadata path remain unsupported. Changing signed resource URLs are conservatively rejected; missing live-window media cannot be recovered automatically. Public video-site acceptance and internet speed equivalence remain unestablished.

Native visual and keyboard/click acceptance remain unavailable because Windows control fails before initialization with a kernel-assets path error. Firefox sources are synchronized but no fresh Firefox-profile live acceptance is claimed. The signed network runtime is unchanged; driver equivalence, publisher signing, automatic updates and clean-machine setup remain open.

## Current-PC deployment

[UDM 0.57.0 installer](../installer-out/UDM-0.57.0-Browser-0.39.0-Setup-x64.exe) is built. 59 files were deployed with SHA-256 verification and original-file backups. Installer SHA-256: `960F811A4268E1BC435AD12CAD58C679BA7E54390E4BE844D52076628579AF88`.

The running app/native bridge reports 0.57.0 and the correct D:/UDM/user-data catalog. All 23 records, queues and saved preferences are unchanged. Browser 0.39.0 sources are installed; the signed network runtime is unchanged. Existing browser sessions need an extension reload and video-page refresh; personal-session activation remains unverified.
