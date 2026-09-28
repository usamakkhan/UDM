# UDM 0.56.0 / browser 0.38.0

This update completes the implemented duplicate-choice workflows: remembered overwrite, safe replacement of completed files, and fresh restart of unfinished downloads. It builds on the download-limit, browser settings and transport changes in 0.53–0.55. Full IDM parity remains open.

## Duplicate choices and dialogs

- The duplicate dialog now remembers all three choices: numbered copy, overwrite, or Existing. Options > Downloads exposes all choices and preserves previously saved preferences.
- Overwrite and Existing use the installed reference's control positions and full descriptions. The remember checkbox explains where to change the saved choice.
- Existing completed links open Download complete; unfinished links resume. A genuinely new or restarted browser download still honors File Info, Download Later and skip-dialog settings.
- Active, queued or unconfirmed records are reused without silently restarting a transfer. An in-progress replacement cannot create another nested replacement.

## Completed and unfinished overwrites

- Completed HTTP(S) and FTP replacements download and verify new bytes before publication at the original filename. The prior file and history record are retained under a previous-version name.
- Explicitly overwriting a file edited since its original download is supported. Its current hash and size become the retained copy's metadata; an obsolete scan result is cleared. A subsequent edit during the new transfer prevents replacement.
- Overwriting an unfinished download restarts the same history record at byte zero with a new temporary-parts generation. Filename, destination, description, queue, category and transfer preferences remain attached to it; stale ranges and validators are cleared.
- A missing completed output can be restored by overwrite. An unrelated file occupying an unfinished download's destination is rejected.
- History and a remembered choice commit together. Failed persistence restores the old record and preference. Cleanup receipts survive crashes and retire only unchanged old part files after commit. Locked or changed parts remain for later recovery; unrelated files are preserved.

These behaviors are independently implemented. The installed IDM dialog 298 provides the reference for wording, positions and available choices; its private implementation is not claimed to be reproduced. Retaining previous versions is an additional UDM behavior.

## Verification

**1,185 checks passed:** 1,135 native checks, 8 native-protocol checks, 8 real isolated app/host checks, 30 live browser/native scenarios across Chrome and Edge, and 4 real local FTP overwrite scenarios.

The native suite covers atomic save failure, remembered choices, completed and partial transfers, real HTTP output hashes, browser/CLI routing, changed files, occupied destinations, configured temporary storage, locked parts, recovery receipts and edited-copy scanner metadata. Windows font measurement checks the dialog text at 96, 120, 144 and 192 DPI. Eight selected resource bounds match the installed reference; this is a static layout comparison, not visual acceptance.

Chrome and Edge each captured the same file twice through the real extension/native bridge, verified publication at the original filename and retained the prior bytes. They also exercised MP4, HLS with audio, ordinary file interception, force/bypass gestures, delayed script downloads and iframe navigation. FTP tests downloaded changed server contents in active and passive modes, overwriting both completed files and partial transfers with exact new hashes.

Receipts: [summary](evidence-0.56.0/summary.json), [native log](evidence-0.56.0/build-verified.log), [FTP](evidence-0.56.0/ftp-overwrite.json), [selected layout audit](evidence-0.56.0/layout-audit.json), [workflow inventory](idm-parity-0.56.0.md).

## Remaining acceptance

Native screenshots, clicks, accessibility and mixed-monitor DPI remain unverified: the Windows control tool fails before initialization with a missing kernel-assets path. Browser source remains 0.38.0, with no fresh personal-profile or Firefox acceptance in this update. Local functional tests do not establish public video-site coverage or internet speed equivalence. Driver equivalence, application/installer publisher signing, automatic updates and clean-machine acceptance remain open. No parity percentage is claimed.

## Current-PC deployment

[UDM 0.56.0 installer](../installer-out/UDM-0.56.0-Browser-0.38.0-Setup-x64.exe) is built. 42 files were deployed with SHA-256 verification and original-file backups. Installer SHA-256: `A51CA3CEA3089C5091E405A8D842D0BC9C2BE6B6E56D8140362B7A1C7A9FC23A`.

The running app/native bridge reports 0.56.0 and the correct D:/UDM/user-data catalog. All 23 records, queues and saved preferences are unchanged. Browser 0.38.0 sources and the signed network runtime are unchanged. This desktop update requires no extension-source reload; activation in existing personal browser sessions remains unverified.
