# Native startup recovery candidate — 2026-10-01

The candidate desktop app now recovers a pending restore before constructing its download Manager, starting queues or exposing browser IPC. It is compiled and qualified with private catalogs, not installed into the user's runtime. Backup/restore GUI and invocation remain unfinished.

## Implementation

RecoverySession.hpp adds a named mutex derived from the canonical data directory. The candidate app holds it throughout the data session; restore sessions use the same lock. Different instance tags cannot bypass this data-directory lock.

A Windows-account-encrypted marker stores the reviewed destination set and journal location. It is written only after the transaction journal exists and before file staging begins. Startup rolls back an unfinished transaction, preserves an already committed transaction, and then removes the marker. An altered marker, missing journal, or failed rollback prevents catalog opening. The restored image must belong to the selected data directory.

This protects cooperating updated processes. The old installed app does not yet implement this lock; deployment must close it before any recovery operation. The restore GUI must coordinate shutdown before calling restoreWithSession. The new startup path alone is not the full recovery user workflow.

## Verified

- Six separate interrupted session phases recovered: marker creation, mid-copy, staged file, original relocation, catalog installation and post-commit marker cleanup.
- Altered encrypted markers and missing journals blocked recovery and retained the marker.
- A second process could not obtain the same canonical data-folder lock; the lock became available after the owner terminated.
- The real rebuilt MFC app recovered an interrupted restore before opening its private catalog. Its original output was restored and the transaction marked Rolled back.
- The real app also reopened a committed restore, retained the image's output and cleared the marker.
- Both actual app runs stayed alive for three seconds, held the data lease, and were terminated only as owned private fixtures.
- All 41 affected restore regression checks passed.
- Personal catalog and installed app/host/monitor hashes remain unchanged.

Evidence and hashes: control/session-acceptance.json. Final app behavior: control/app-recovery-final.json. The app runtime requires the verified assets directory beside the executable. Build using control/build-app.ps1; it now copies those assets.

## Retained failures and corrections

An initial test tag exceeded the existing 32-character limit and never reached recovery. A later startup test exited with Windows fail-fast code 0xC0000409. The unchanged native83 baseline remained running on the same private data.

Correcting a manifest-linker setting did not resolve the exit. The dedicated private-process debugger then captured the actual C++ exception: Windows could not decode the toolbar image. The test runtime was missing its assets directory. Adding the established runtime assets fixed the startup failure; both final app cases then passed. Early immediate-process observations were too weak and are superseded by the final sustained checks. No product crash fix is claimed for that packaging error.

## Remaining

Add native backup/restore review, progress and cancellation dialogs, restore invocation and orderly shutdown/restart coordination. Startup rollback currently occurs before the main window; its progress presentation is still missing. Empty directories, cleanup/retention UI, cross-account migration, disk-full and physical power-loss qualification remain open. Full IDM parity is not established.

Candidate: D:/UDM-Workspace/candidates/native-084-backup.
