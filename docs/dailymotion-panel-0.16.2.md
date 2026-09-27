# Dailymotion video panel fix — extension 0.16.2

The reported page was https://www.dailymotion.com/video/xbbklbu. In the user's Chrome session, the video player was a cross-origin iframe at https://geo.dailymotion.com/player/xtv3w.html. IDM's panel was visible; UDM's panel was absent.

## Confirmed cause

UDM's optional site permission had been granted only for https://www.dailymotion.com/*. The old popup requested only the address-bar origin. This excluded the actual embedded player and media hosts while reporting that video panels were enabled. This was a limitation in UDM's implementation, not a missing kernel driver.

Dailymotion's documented embed endpoint also uses geo.dailymotion.com: https://developers.dailymotion.com/docs/player-embed-script-web .

## Change

- Dailymotion's per-site permission request now covers its subdomains and dmcdn.net media hosts. Other sites keep their existing scope. No browser profile files are edited and optional permissions are still granted through Chrome.
- The popup shows the requested domains and says when page refresh is needed to observe playback. It does not claim a button or downloadable format was detected just because access was saved.
- Concurrent permission-event and popup activation requests serialize their content-script registration. Existing registrations are updated without an unregister/re-register gap.
- Immediate injection failures are returned to the popup instead of silently reporting successful activation.
- Chromium and Firefox extension sources are synchronized at version 0.16.2. The native desktop remains 0.16.1; no native engine changes were needed for this bug.

## Validation

- Real isolated Chrome extension regression: parent-only permission reproduces zero panels in the embedded player; corrected platform scope produces exactly one panel and the actual synthetic 360p source. Concurrent activation retains one registration/panel; the panel survives navigation; an unrelated iframe remains ungranted. Six checks passed. Fixture permissions were pre-granted only in test profiles; the public site was not substituted in the user's Chrome.
- Existing cross-site media suite: 31 passed.
- Existing browser handoff/identity suite: 30 passed.
- Chromium/Firefox preparation and stable extension identity: passed.
- In the user's Chrome, extension reload was verified as 0.16.2 and the new permission prompt correctly names all dailymotion.com and dmcdn.net sites.

## Live verification status

The exact public video was successfully tested through Dailymotion's documented embed endpoint in an isolated Chrome profile, using UDM's real extension and native backend. The first top-level-page probe did not instantiate its player, so the follow-up used https://geo.dailymotion.com/player/xtv3w.html?video=xbbklbu. No user cookies or profile data were copied.

- One visible UDM panel, 168 x 24 pixels. Hidden auxiliary videos had hidden panels.
- The actual menu offered 288p HLS and its observed media playlist; no higher qualities were invented.
- A trusted panel click downloaded the complete video: 48,520,179 bytes, 40.59 seconds measured from the selection through completed-file verification.
- ffprobe: H.264 video, 512 x 288, AAC audio, duration 837.475 seconds, matching the player's 837.48 seconds.
- Full ffmpeg video/audio decode with `-xerror` succeeded, exit code 0.
- Output: `../benchmarks/dailymotion-panel-20260924/dmlive1790291547694/Dailymotion Video Player.mp4`.
- Evidence: `../benchmarks/dailymotion-panel-20260924/live-download-result.json`, `live-menu.json`, `dailymotion-live-menu-report.json` and `dailymotion-live-menu.png`.
- The normal user history remains 14 records: 9 complete, 3 failed, 2 paused. The test used a separate native instance and data directory.

The user's normal Chrome session still awaits the final click on its Dailymotion/dmcdn.net permission prompt. Extension 0.16.2 is loaded there, but the button and menu on that original tab must still be verified after permission is granted. The independent live embed success does not substitute for that remaining session check.

Known limits observed: this session offered only 288p; other resolutions were not verified. The generic capture menu also lists the observed media playlist alongside the master playlist's quality, and the generic embedded-player title becomes the filename. Universal ad exclusion and full platform parity are not established by this test.

This fix does not establish feature parity with IDM or universal support for every video platform. Clear recorded HLS, supported static DASH and direct media retain the existing limitations described in the extension.

Evidence: ../benchmarks/dailymotion-panel-20260924/regression-results.json and before-parent-only/result.png, after-platform-scope/result.png. Source changes were backed up in ../backups/dailymotion-panel-20260924/.
