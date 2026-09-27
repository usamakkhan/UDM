# UDM audit — 26 September 2026

Browser extension 0.23.2 fixes five confirmed problem areas; the C++ desktop stays at 0.25.0.

| Problem reproduced | Correction |
|---|---|
| A failed or stalled badge/status write could prevent a successful native handoff from being acknowledged, or delay browser-download recovery. | Status reporting is best effort and cannot block either path. |
| Browser cancellation failure after native acceptance resumed the browser transfer, creating duplicate transfers. | After acceptance, UDM owns the job; cancellation failure reports the uncertain browser state without automatically resuming it. |
| Selected-link partial failure omitted the number already accepted. | Reports the accepted count and warns that the final request may also have been accepted before its acknowledgement was lost. |
| Link-selection lists existed only in worker memory. Restart lost them; submitted state was not durable. | Bounded lists live in extension session storage, with submitted state saved before the first handoff. An interrupted submitted list cannot be replayed after restart. Expired lists still expire after ten minutes. |
| A fetch begun on one SPA page could return after navigation and attach the previous page's playlist to the new video. | Playlist observation retains the page identity from request start and rejects responses after navigation. |

Also corrected an outdated site-permission fixture, made the Firefox test host configurable without changing registry entries, and disabled the link-selection submit button until a valid list has loaded. No driver, browser registration, capture opt-in or credential preference was changed.

## Validation

- 453 C++ native checks passed using the native test binary from the verified 0.25.0 build; the retained C# legacy test executable was identified and excluded from native results.
- 30 real extension-to-desktop checks passed: Chrome 13, Edge 13, Firefox 4. Tests use separate profiles and histories and include MP4 SHA-256 identity, HLS assembly with audio, browser capture, scoped request headers, per-tab controls and Download Later.
- 191 unit/component regression checks passed, including 10 new audit checks. Seven fault-injection cases failed before the fixes, as did the delayed-playlist navigation regression.
- 62 browser UI/lifecycle checks passed, including an actual service-worker stop/restart with an open link-selection tab, and rejection of a persisted uncertain submission. Package preparation passed, and seven real native-protocol checks passed.
- The existing Edge profile successfully handed the selected public YouTube video to UDM and downloaded it using the captured SABR session. Output: 265,982,413 bytes, 1920x1080 H.264 video, AAC audio, 1072.994104 seconds. Five-second video/audio decode checks passed at the start, middle and end. SHA-256: `6446732df2fa58d59e66a1bcf79f14bea1c0c4370373de83ac502ad9ba8f7bfc`. This live download ran before the 0.23.2 fixes were installed; the corrected extension was then verified against native downloads in isolated browsers.

Automatic approval review rejected temporary native-host registration as an access-boundary change. Read-only checks confirmed the existing host registrations, and the approved tests used those unchanged registrations instead. No temporary registration was installed.

## Remaining limits

This audit does not establish complete IDM parity or speed parity. Live YouTube success is session-dependent; this test establishes one completed video, not universal site support. Arbitrary POST downloads, every site-specific player, partitioned cookie containers and signed browser-store distribution remain incomplete. Other Chromium browsers have matching source packages but were not exercised individually. Firefox remains a development add-on. The current clear recorded HLS/static DASH restrictions remain.

Evidence and backups are under the local `deep-audit-20260926` artifact directory. Test-browser failures in the first permission/worker harness runs are preserved alongside corrected passing results. Reload installed unpacked extensions and refresh video pages to run the new document-start observer.
