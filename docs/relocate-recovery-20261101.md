# Move/Rename recovery — 1.6.3

Move/Rename now requires the selected record to remain in the catalog, reconciles its pending publication and replacement journals (including private tracks), and rechecks completion before using its path. A pending replacement that depends on the original file blocks relocation, matching the existing removal protection. This prevents moving the original away from the location saved by the replacement workflow.

Existing file-operation recovery still runs first. Its catalog update now rolls back in memory if saving fails, leaving the journal available for another recovery attempt. File verification and the existing move journal remain in place.

This is a source-level recovery correction, not a newly measured IDM behavior or proof of full parity. Runtime tests are deferred by request. No installer, installation or publication is requested; installed UDM remains 1.6.0 and the browser remains 0.63.82.

Build record: [native receipt](validation/relocate-recovery-build-20261101.json).

All five production targets built successfully at 1.6.3. Build-start source hashes remained unchanged; executable versions, sizes and SHA-256 hashes are recorded in the receipt.
