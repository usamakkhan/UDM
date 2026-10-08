# Video-panel source-change fixes — 2026-10-08

Browser source version: 0.62.10. Native binaries are unchanged.

Reviewed and corrected two reproduced races: a pending format lookup prevented a new video from loading its formats, and a source change during a download handoff left the reopened menu empty. Per-player generations now isolate replies and control state. When an older handoff settles, the open panel requests the new video's formats without submitting another download. Uncertain earlier acknowledgments remain visible. Rapid source events have distinct identities.

Final verification:
- 16 passing race checks: candidates/panel-source-race-20261008/review-final/results.json. Both Chromium and Firefox content bundles execute in isolated Edge with controlled extension replies; this is not a real Firefox run.
- 12 passing panel-control checks: candidates/panel-source-race-20261008/review-controls/results.json. Includes refresh, reset, compact mode, close, hide and uncertain-acknowledgment recovery.
- Earlier real isolated Firefox/native flow: 13 passed, 0 failed, temporary registration removed; candidates/panel-source-race-20261008/firefox/results.json. This ran after the functional fixes but before final status-text/UTF-8 and line-ending cleanup. It does not establish the cause of historical intermittent Firefox timeouts.
- git diff --check passed. Both content bundles decode strictly as UTF-8; unintended line-ending churn was removed.

Baseline reproductions are retained under baseline and handoff-baseline in the same candidate directory. The browser/native fixture now records Firefox panel lookup state to help diagnose future timeouts.

Delivery: the new 0.62.10 installer includes these fixes. All 144 package inputs were verified unchanged after compilation. See validation/panel-source-race-release-20261008.json. Personal browser activation and installed native replacement were not performed. Full IDM parity and public-site coverage remain unestablished.
Fresh final-source Edge/native acceptance: 14 passed, 0 failed, isolated registration removed. Evidence: candidates/panel-source-race-20261008/edge-final/results.json. Covers selected audio, parallel indexed downloads, ignored-Range rejection and live subtitles. Both staged browser bundles match reviewed source.
