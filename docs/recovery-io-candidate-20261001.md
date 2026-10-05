# Recovery I/O and interrupted-copy qualification — 2026-10-01

The isolated C++ backup/restore candidate now supports cancellation during a large file copy and during SHA-256 verification. This supersedes the earlier limitation that cancellation only took effect between files. The candidate remains uninstalled; GUI and lifecycle integration are still pending.

## Changes

RecoveryIo.hpp supplies cancellable copy and hash operations to backup and forward restore. The Windows copy callback catches C++ exceptions, requests cancellation, then rethrows only after the Windows API returns. Existing destination files cannot be overwritten by the copy helper.

Implementation follows Microsoft's [CopyFileExW documentation](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-copyfileexw) and [progress callback contract](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nc-winbase-lpprogress_routine). PROGRESS_CANCEL removes the incomplete copy. Hashing checks cancellation between 64 KiB reads.

Rollback now restores originals even when abrupt termination leaves an incomplete staging file. Unverified staging bytes are retained and listed in RetainedStaging. Later edits to restored destinations still block automatic rollback and preserve originals for manual recovery. Rollback can also remove verified read-only replacement files and put the originals back.

## Final evidence

- **10 copy/hash checks passed.** A real 16 MiB file copy canceled at 1,048,576 bytes; its destination was absent and source hash unchanged. Hashing canceled after 65,536 bytes. Callback exceptions, empty hashes, existing-file protection, successful copying and actual backup mid-copy cancellation passed.
- **29 backup checks passed.**
- **41 restore checks passed**, including in-copy cancellation and read-only replacement rollback.
- **2 actual transfer checks passed:** restored partial bytes resumed through the loopback HTTP server and finished with the expected SHA-256.
- **5 process-crash cases passed**, after a mid-copy callback, staging, original relocation, file installation and catalog installation. Each fresh recovery process recovered every pre-restore file. The mid-copy case retained its incomplete staging file and recorded its path.
- Installed app/host/monitor binaries and personal catalog still match the recorded hashes.

Source/binary hashes, results and preservation checks are in control/io-acceptance.json. Detailed runs are in control/io-test-runs.json and control/io-crash-runs.json. Earlier evidence remains historical; repeated suites do not add feature coverage. BackupTests' old scope-label text predates the companion restore implementation; its assertions still cover only backup creation/verification.

## Remaining before release

Enforce offline restore through app lifecycle coordination, add native review/progress/cancel dialogs, and detect unfinished transactions on startup. Empty-directory restoration/cleanup, original-copy retention UI, cross-account credentials, disk-full and physical power-loss qualification remain open. Rollback intentionally finishes recovery rather than honoring the canceled forward-operation token; its own hashing is not cancellable. This is not complete IDM parity.

Candidate: D:/UDM-Workspace/candidates/native-084-backup.
