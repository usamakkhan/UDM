# Browser integration 0.20.2 — panel recovery and verified Edge download

25 September 2026. Browser integration 0.20.2 is loaded in the regular Edge profile. Native UDM remains 0.20.0; no native rebuild, driver change or browser permission change was needed.

## Fixed and deployed

- Media reads and capture preparation now have 12-second deadlines instead of leaving the panel indefinitely at “Sending to UDM”. Native acknowledgement has a separate 40-second limit. Late page results cannot create a job; an uncertain native response advises checking UDM's list and does not submit an automatic duplicate.
- Successful native acceptance no longer waits for badge/error-storage updates.
- Replacing or invalidating an extension context disposes its panel, timers, observers and owned player marker. This addresses the three overlapping panels seen in the regular-profile diagnostics after old reloads. A one-time page refresh cleared the older contexts; the final source is reloaded and the refreshed page shows one panel.
- The extension popup now links directly to Capture diagnostics. Both the link and “UDM is connected and ready” were verified in regular Edge.

53 browser logic checks, 7 panel-control checks and 6 isolated-world lifecycle checks passed. Firefox sources match the Chromium changes but Firefox was not tested live. Native code is unchanged from the previously verified 342-check build.

## Real regular-profile test

The existing YouTube video `BewnzhHlQuk` showed actual choices from 144p through 1080p. The panel was hidden during the observed advertisements and appeared for the main video. Selecting 1080p opened native Download File Info; Start Download completed the transfer and merge.

- Output: **265,982,413 bytes**, 1920 × 1080 H.264 video and AAC audio.
- Duration: **1,072.994 seconds** (17 minutes 53 seconds).
- Ten-second audio/video sections at the beginning (0s), middle (530s) and end (1060s) decoded successfully without errors. The separate full-file decode attempt exceeded its five-minute time limit; full-file decoding remains unverified.
- This was a functional download test, not a controlled speed comparison with IDM.

The first test capture expired while its confirmation dialog was held open. UDM reported the expiry. Refreshing playback and selecting 1080p again produced a new capture; starting that immediately succeeded. Long-held captured sessions still require refreshing playback and recapturing.

## Remaining limits

Fresh isolated Chrome testing reached native confirmation. Two isolated Edge transfer attempts, including one after 15 seconds of playback with attestation data present, failed with browser-attestation rejection. The regular Edge transfer above succeeded; that does not establish the exact cause of the isolated failures or universal YouTube/site compatibility. UDM's parser rejects UMP stream-protection status 3, also interpreted as attestation rejection by the independent [GoogleVideo implementation](https://github.com/LuanRT/googlevideo/blob/main/src/core/SabrStream.ts). No protection checks were removed and no alternate resolver was added.

All **15 original download records** and queue data remain unchanged. The two paused ISO byte counts remain 546,308,096 and 571,867,136. The regular history now has **17 records**, including the expired test attempt and the successful test download. Other live tests used isolated histories.

## Evidence

- [Aggregate verification](../benchmarks/gui-0.20.2/verification.json)
- [Regular Edge transfer and file validation](../benchmarks/gui-0.20.2/regular-edge-download/result.json)
- [Panel controls](../benchmarks/gui-0.20.2/edge-panel-controls/controls-results.json)
- [Panel lifecycle](../benchmarks/gui-0.20.2/edge-panel-lifecycle/results.json)
- [Isolated Chrome handoff](../benchmarks/gui-0.20.2/chrome-youtube-handoff/result.json)
- [Isolated Edge transfer](../benchmarks/gui-0.20.2/edge-youtube-download/result.json)
- [Isolated Edge settled-playback transfer](../benchmarks/gui-0.20.2/edge-youtube-settled-download/result.json)
- [Reusable live runner](../tests/youtube-handoff.browser.cjs): `node tests/youtube-handoff.browser.cjs <fresh-output-directory> msedge download warm`. This creates a fresh browser profile and separate native state; it requires Playwright, installed browsers and native-host registration.

Browser-source rollback: `D:\UDM\backups\browser-0.20.2-20260925\browser`.
