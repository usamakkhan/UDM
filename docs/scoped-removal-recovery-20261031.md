# Scoped removal recovery — 1.6.2

Catalog removal previously triggered global recovery only when the selected parent had a publication journal. This missed private audio/video track journals and replacement journals, potentially removing a record still needed for recovery. A triggered global pass could also process unrelated downloads before checking whether the selected transfer was active.

Removal now rejects missing/stale selections and active transfers first, then calls scoped recovery for the selected download. The existing scope traversal includes private tracks, rejects active tracks and replacement originals, and filters out unrelated journals. Removal proceeds only after recovery succeeds; existing pending-replacement dependency checks and catalog-save rollback remain in place. Recovery errors leave the selected catalog record available.

The completed-file Recycle Bin path also recovers the selected download before filesystem deletion. It checks completion again after recovery because an interrupted operation may restore a paused record. A changed status produces an error before invoking Windows, allowing the user to review the recovered state. This avoids recycling a file still required to verify its publication journal.

The change does not delete staging data or bypass recovery validation. Recovery may hash files while holding the manager lock, as before, so this is not a responsiveness claim. A successfully committed recovery remains committed even if subsequent removal fails. Tests and runtime fault-injection remain deferred by user request.

Native build: passed for all five production targets at version 1.6.2. Build-start source hashes remained unchanged; binary sizes and SHA-256 hashes are recorded. No installer, installation or publication requested. Installed UDM remains 1.6.0; browser code remains 0.63.82. Evidence: [build record](validation/scoped-removal-recovery-build-20261031.json).
