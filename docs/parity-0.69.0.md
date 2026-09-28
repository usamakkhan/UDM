# UDM 0.69.0 / browser 0.45.0 — direct video recovery

Unfinished direct YouTube video/audio downloads now support **Refresh download address**. It opens the original page, waits for the same quality in the UDM panel, and applies the reviewed capture to the existing download. Filename, history and saved data are preserved. Resume validates retained partial and completed tracks before reuse; changed streams are rejected.

Native player readiness now checks the beginning, middle and end of both tracks. This fixes a false-ready result observed on a public video: its first byte was accessible while later offsets returned 403. Unusable native pairs leave browser capture available. Public YouTube acceptance is still incomplete; a panel timeout was also observed.

**1,785 passing checks:** 1,770 native regressions, 7 isolated Edge recovery checks and eight real native-protocol checks. The Edge fixture recovered partial video plus completed audio on one history record and verified decoded output. These are controlled tests, not an IDM speed comparison. [Evidence](evidence-0.69.0/summary.json), [test scope and limits](evidence-0.69.0/DEVELOPMENT.md).

Browser integration stays at **0.45.0**, with no new reload requirement. Personal Edge activation and rendered Windows dialogs remain unverified. Full IDM parity is not established. [Workflow inventory](idm-parity-0.69.0.md).

## Current-PC deployment

Native 0.69.0 is installed and running. The bridge reports direct-media and adaptive session recovery support. All 23 records, queues and preferences are unchanged, and installed file hashes were verified. The signed network runtime is unchanged. Browser 0.45.0 files are unchanged by this update. No new reload is required for an already active 0.45.0 session.

[Installer](../installer-out/UDM-0.69.0-Browser-0.45.0-Setup-x64.exe), SHA-256 `2e53146bb905fb2d10cbacbe2e6dce3b67b9b4844b691205d9abe87515830cc2`. [Deployment receipt](evidence-0.69.0/deployment.json).
