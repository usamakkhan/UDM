Current follow-up: recovery-io-candidate-20261001.md (IO-ACCEPTANCE.md in the candidate) supersedes the large-file cancellation limitation and adds interrupted-copy/read-only rollback qualification. The report below preserves the preceding test scope.

# Native restore transaction candidate — 2026-10-01

The C++ restore backend is implemented and tested in the isolated native-084-backup candidate. It is not installed or connected to the desktop GUI. Full IDM parity remains unproven.

## Behavior

The restore verifies the recovery image and requires an exact reviewed destination set. It rejects overlapping, non-normalized and linked destination paths. It stages replacement files beside each destination, records a transaction journal, retains originals, installs data before the catalog, and verifies restored hashes before marking the transaction committed.

Cancellation or an ordinary failure initiates rollback. A separate process can use the journal to roll back an interrupted restore. Rollback refuses to discard files that changed after restore began, retaining the original copies for manual recovery. A completed rollback is idempotent; committed transactions are not automatically undone.

Credentials remain DPAPI encrypted. The successful same-account test verifies decryption after reopening the actual native Manager. On opening a catalog, Manager normally adds/resets ConfirmationPending=false; the reopened-catalog assertion accounts for that established startup normalization. Restored on-disk bytes are independently checked exactly against the image.

## Final evidence

- **36 focused restore checks passed, zero failed.** Exact file restoration, native settings/history reopening, encrypted request credentials, preservation of originals, staging/apply failures, cancellation, idempotent rollback, missing-file recreation and removal on rollback, unapproved destinations, locked files, corrupt images, and preservation of later file edits.
- **Two actual transfer checks passed.** A deliberately damaged partial file was restored, reopened in the native engine, resumed through a loopback HTTP range request, and completed with the expected SHA-256.
- **Four abruptly terminated processes recovered in fresh processes.** Termination occurred after staging a file, moving an original, installing a data file, and installing the catalog. Every pre-restore file was recovered.
- The final binary and sources are fingerprinted in control/restore-acceptance.json. Logs and exit codes are in control/restore-final-runs.json. Earlier failed results remain in restore-test-1; the only first-run assertion failure was the expected startup confirmation-flag normalization.
- Personal catalog and all three installed runtime binaries retain their previous hashes. Tests used disposable files on D:; no personal browsers, certificates, drivers or settings were changed.

## Still required before release

The API currently requires an offline caller. Application shutdown/restart coordination and enforcement, a responsive MFC review/progress/cancel flow, startup detection of unfinished restore transactions, and installer integration are not implemented. Do not call this backend against a running app.

Created empty directories may remain after rollback, and restoration of empty directories that contain no files is not implemented. Cross-account credential migration, browser extension storage and Windows registration recovery remain outside this candidate. Original copies remain beside restored files after success; GUI cleanup/retention policy is pending. Large individual file copies are not yet cancellable mid-copy. Physical power-loss durability, disk-full recovery and cross-volume deployment need further qualification.

Run control/build-restore.ps1 to compile. Udm.RestoreTests.exe takes a fresh absolute directory for the main suite; --resume runs the real download test. --crash DIRECTORY PHASE exits with code 73 at a fixture checkpoint; --recover DIRECTORY performs journal rollback in a separate process. These switches exist only in the test executable.

Candidate location: D:/UDM-Workspace/candidates/native-084-backup.
