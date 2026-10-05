Update: the companion restore backend is now implemented and tested; see restore-transaction-candidate-20261001.md (or RESTORE.md in the candidate). The original backup-stage report below is historical. GUI and lifecycle integration remain pending.

# Native recovery-image candidate — 2026-10-01

Staged backend work only. The installed UDM app and previously qualified source remain unchanged. No parity percentage is established. Restore and the native UI are not implemented in this candidate; this is not a released backup/restore feature.

IDM documents copying temporary files and completed downloads alongside its registry settings, then restoring the same paths. [Official guidance](https://www.internetdownloadmanager.com/register/new_faq/functions17.html), checked 2026-10-01. UDM's existing catalog export omits settings and resume data, so it cannot fulfill that recovery workflow.

## Implemented

The new native/Backup.hpp captures an exact in-memory native catalog snapshot, including settings, queue definitions, histories, projects, encrypted request data, child media tracks and native browser receipts. It inventories the data directory, explicitly recorded external parts/media directories, current completed outputs and recorded previous paths. Missing completed files are listed rather than silently treated as backed up. Empty directories are recorded.

The writer holds the manager lock and refuses active downloads/live captures. It acquires read-only source leases that deny writing/replacement, copies and hashes each file, compares the inventory again, and verifies the image before publishing it through a non-replacing directory rename. Cancellation and failures leave an explicitly named .incomplete-* folder. Existing backup directories cannot be replaced. A child junction/reparse point is rejected rather than traversed.

The format records original absolute paths for eventual same-path recovery. DPAPI-protected fields remain encrypted for the original Windows account. The image also contains private history and any other data already present in the native data directory; it is not a password-encrypted portable archive. No decrypted credential is placed in the manifest.

## Evidence

- Focused native suite: **29 checks passed, zero failed**, including exact snapshot equality, external and child-track resume data, encrypted request headers, output inclusion, missing output reporting, no source mutation, locked-source refusal, actual native active-transfer refusal, cancellation before/during/just before publication, changed inventory, damaged payload, and malformed manifest paths.
- Separate actual Windows directory junction test: rejected; its external fixture target remained unchanged.
- All 267 fingerprinted native83 base-source files match. The personal catalog and three installed runtime binaries match their previous hashes.
- Built with the cached MSVC C++17 toolchain and the fingerprinted final native83 core objects. This is a focused component fixture, not a rerun of the full app suite.
- Tests used disposable data only and made no browser, driver, certificate or system setting changes.

Reproduce using control/build.ps1, then run build/Udm.BackupTests.exe with a fresh absolute test-directory path. Test logs, source/object hashes and preservation checks are in control/acceptance.json. The test-1 directory records the earlier 24-check run; test-2 is the final expanded run. Repeats are not additional coverage.

## Remaining work

Implement an offline restore transaction with preflight, conflict handling, rollback and interrupted-restore recovery, then connect creation/verification/restore to a responsive native GUI. The future restore must validate path mappings and explicit user intent; an intact manifest is not proof that a backup is trusted.

No cross-account DPAPI recovery, browser extension storage, Windows/native-host registration or external helper/application backup is provided here. Recovery after physical power loss has not been tested. Existing file-access leases and inventory checks are not a filesystem snapshot service.

Candidate: D:/UDM-Workspace/candidates/native-084-backup.
