# UDM 0.68.0 / browser 0.45.0 — recorded-video link recovery

“Refresh download address” now works for unfinished recorded HLS and static DASH downloads. A fresh Edge video-panel capture returns to the existing download for review. Applying it keeps the filename, destination and history, replaces session credentials, and checks retained segments before reuse.

Pause the download, choose **Refresh download address**, open its original video, and choose the same quality/audio/subtitles in UDM's browser panel. Return to **Refresh media session** and choose **Save and resume**. Changed selections or segment layouts are rejected. Live recordings remain outside this recovery path.

Fresh-source verification fetches saved segments again to compare their hashes. This avoids silently mixing changed content; it adds recovery traffic. Ordinary resume behavior is unchanged.

**1,739 passing checks:** 1,721 native regressions (including 41 new recovery checks), ten isolated Edge checks and eight native-protocol checks. Both Edge HLS and DASH tests recovered the original record after HTTP 403 and produced matching decoded video with audio. [Evidence](evidence-0.68.0/summary.json), [test scope and limits](evidence-0.68.0/DEVELOPMENT.md).

Browser integration remains **0.45.0**. No extension reload is introduced by this native update. Personal browser activation and the Windows recovery dialog still need visual acceptance; the desktop control tool cannot initialize. Full IDM parity, generic direct-video session recapture and public-site coverage remain incomplete. [Workflow inventory](idm-parity-0.68.0.md).

## Current-PC deployment

Native 0.68.0 is installed and running. The bridge reports adaptive session recovery support. All 23 records, queues and preferences are unchanged, and installed file hashes were verified. The signed network runtime is unchanged. Browser 0.45.0 files are unchanged by this update. No new reload is required for an already active 0.45.0 session.

[Installer](../installer-out/UDM-0.68.0-Browser-0.45.0-Setup-x64.exe), SHA-256 `473c1e0014ec1e46df1d1c27cc2415a09259085dfa3041ed1d5e82d206ca0741`. [Deployment receipt](evidence-0.68.0/deployment.json).
