# Directory recovery candidate — 2026-10-01

The staged native recovery candidate restores empty folders and includes folder destinations in the native restore review. The exact reviewed file/folder set must match before any restore starts. Older images without a directory inventory remain supported.

Rollback removes only recorded, newly created, still-empty directories using a handle whose filesystem identity matches the creation receipt. Existing folders, replacement folders and new external files survive. A process interruption between folder creation and recording its identity conservatively leaves that folder in place; the journal reports retained directories. This is intentional preservation, not proof that all crash debris is removed.

Verification on this revision:
- 59 restore assertions passed, including 18 new directory checks, invalid directory manifests and legacy images.
- 23 actual MFC dialog assertions passed, including folder review and empty-folder restoration.
- Two real process terminations before/after the directory receipt recovered successfully.
- Two actual HTTP range-resume assertions passed with an exact final hash.
- Five rebuilt-app handoff checks passed: menu, separate recovery process, exclusive data lease, return to original profile and normal closure.
- Restore-review screenshot inspected for legible folder rows and controls.
- Personal catalog and installed app/host/monitor hashes remain unchanged.

Evidence: D:/UDM-Workspace/candidates/native-084-backup/control/directory-acceptance.json.
This candidate is not installed, versioned or packaged. Older acceptance records refer to earlier source hashes. Physical power loss, disk-full behavior, cross-account migration, retained-file cleanup UI, browser requests during recovery and installer integration remain unqualified. Complete IDM parity is not established.
