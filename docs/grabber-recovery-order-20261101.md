# Site Grabber recovery ordering — 1.6.6

Start checked downloads calls resume within addProject's catalog transaction. Resume reconciles publication and replacement journals, but save returns without writing while a transaction is active. Recovery could therefore remove a journal before the final batch save. A later failure or process exit could leave durable history unrecovered without its journal.

Project start now resolves the selected existing download records first, before taking the rollback snapshot or enabling deferred saves. The preflight uses the same project/URL mapping, selection, immediate-start and resume-existing conditions as the existing start path. Active and completed entries are skipped, as they are by startMatched. New-file-only exploration does not recover or restart existing selections. Offline archive project admission is unchanged.

Publication/replacement recovery additionally rejects a matching journal inside an active catalog transaction before changing records or files. Legacy verified-file reconciliation also rejects such a deferred commit. A normal resume with no pending recovery can still run inside the project transaction. Private track recovery retains its existing scope traversal.

Successfully committed recovery survives a subsequent project-start error. The batch rollback snapshot is taken after recovery, so it cannot restore an obsolete record whose journal has already been removed. Unselected jobs are not globally reconciled, although recovery of a selected replacement can update its required original record.

Native version: 1.6.6; browser remains 0.63.82. Tests remain deferred by user instruction. No installer, installation or publication is requested; installed UDM remains 1.6.0. Compilation does not establish full IDM parity or fault-injection acceptance.

Build record: [native receipt](validation/grabber-recovery-order-build-20261101.json).

All five production targets built successfully at 1.6.6. Build-start source hashes remained unchanged; executable versions, sizes and SHA-256 hashes are recorded in the receipt.
