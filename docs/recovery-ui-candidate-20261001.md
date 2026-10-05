# Native backup and recovery UI candidate — 2026-10-01

The staged MFC app now exposes **Tasks → Backup and recovery…**. It closes the main app after saving, opens a recovery process for the same data directory, and offers Create backup, Verify, Restore and Open UDM. This is implemented and tested, but not installed or packaged for release. Full IDM parity remains unproven.

## User workflow

Active downloads must be paused before entering recovery. The recovery process waits for the old app to exit and holds the shared data-folder lease. Backup and restore then operate without a live main-window timer or download Manager competing with the worker's catalog lock.

Backup offers a new folder and reports verified file and missing-file counts. Verify checks the image on a worker thread. Restore displays original destinations and sizes, requires an explicit Restore click, rechecks that the reviewed manifest is unchanged, and uses the protected transaction/session backend. Cancelling the review leaves existing files untouched.

Progress dialogs remain responsive during worker operations. Cancel requests cancellation and waits for the worker to finish preserving existing data. Mandatory automatic rollback disables Cancel and Close until recovery finishes. Open UDM exits recovery and restarts the same data directory and instance tag. A normal recovery-window close returns process success.

## Final verification

- **20 actual MFC dialog checks passed**: backup creation, verification, restore review/cancel, exact-byte restore, progress display, worker cancellation, disabled cancellation during mandatory rollback, and orderly dialog closure.
- **5 actual app handoff checks passed** on the final binary: native menu command, main app exit/recovery process opening, data lease, Open UDM returning to the same profile, and normal app close.
- **8 launch-option checks passed**, including ordinary download compatibility and invalid/missing process identifiers.
- An actual app recovery-mode run showed pending-restore progress, restored the original file, cleared its marker, opened the recovery center and exited with code zero.
- Rendered recovery center, review and progress images were inspected for legible text and controls at the test desktop's scale. This is not full DPI/accessibility qualification or a claim of visual equivalence to an IDM backup screen.
- Personal catalog and installed app/host/monitor hashes are unchanged. All work used private profiles. Owned fixture processes exited.

Final evidence: control/ui-acceptance.json, control/ui-app-flow-release.json, control/recovery-ui-pending-final.json and recovery-ui-final/results.json. Source and executable hashes are recorded. Screenshots are in recovery-ui-final and recovery-ui-2.

## Retained failures

The first dialog fixture tried its cancellation scenario after destroying its designated main dialog, so the queued quit ended the test. Moving cancellation inside the live recovery center corrected the harness and passed.

The first pending-recovery close check expected process zero but received the normal dialog Cancel code 2; recovery itself succeeded. The app now normalizes a completed recovery-center close to process zero. The final actual-app check passes.

Earlier startup failures caused by missing toolbar assets remain documented in SESSION-ACCEPTANCE.md. The app build copies the required assets and preserves the established manifest-linker setting.

## Remaining release work

Disk-full and physical power-loss qualification, cross-account migration, retained-original cleanup UI, browser requests during recovery, and close/save-failure paths need further work. Ordinary app startup still runs pending rollback before showing the main window; recovery mode provides the progress dialog. Installer integration, versioning, source promotion and activation are pending. Existing installed releases do not implement the new shared recovery lock and must be closed for deployment.

Candidate: D:/UDM-Workspace/candidates/native-084-backup.


Directory restoration and cancellation cleanup now have separate verification: [directory recovery candidate](recovery-directories-candidate-20261001.md). This supersedes the empty-directory gap above; packaging remains pending.
