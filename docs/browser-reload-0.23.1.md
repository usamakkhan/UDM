# UDM extension 0.23.1 — reload recovery

The normal Edge profile reported two startup exceptions (undefined runtime.id and storage.local) and two redundant optional-host-permission warnings on extension 0.23.0. Extension 0.23.1 validates the browser APIs before claiming page-script ownership, safely retires selected-link scripts after replacement/invalidation, and removes optional HTTP/HTTPS permissions already covered by required host permissions. Chromium and generated Firefox scripts match.

All 181 targeted checks passed: 54 browser capture, 32 cross-site media, 19 integration protocol, 1 package preparation, 8 selected-link, 12 panel controls, 19 panel geometry, 4 compact video-menu and 32 reload lifecycle checks (16 each in Edge and Chrome). The lifecycle checks also assert zero uncaught exceptions and replacement of selected-link panels without duplicates. A panel-controls fixture was corrected to distinguish integration-policy requests from format lookups.

Eleven files were deployed to D:\UDM with SHA-256 baseline checks and backups in backup-before-0.23.1. Native UDM remains 0.25.0. Chromium ZIP and unsigned Firefox XPI packages are in packages/ with SHA-256 checksums.

The normal Edge Extensions page confirmed enabled UDM Browser Integration 0.23.1 after reload. On the refreshed YouTube page, the panel hid during the observed pre-roll, returned for the main video and listed six H.264 MP4 qualities from 144p to 1080p. The toolbar connection check reported "UDM is connected and ready." Edge retained two historical runtime error entries; both redundant permission warnings disappeared and no additional entries appeared during this check. No new video download was started. Live verification is recorded separately in live-verification.json.

This patch does not establish 99% IDM parity, every-browser validation, or completed live YouTube downloads. The active Chrome profile was not reloaded during this final check; its shared source folder is updated. Firefox's matching package was regenerated; its full native integration evidence remains from extension 0.23.0.
