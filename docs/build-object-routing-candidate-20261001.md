# Current-engine build routing — staged correction

Seventeen candidate build/relink scripts now use select-core-objects.ps1. Older recovery scripts previously linked base83 Core/Bridge/Queue objects; those paths now select the current overlay objects consistently. Missing objects, duplicate names and an omitted required engine component fail explicitly instead of silently falling back.

Five selector checks passed. All 17 scripts parse and reference the selector exactly once. The restore fixture was rebuilt with the selected current objects and passed all 59 restore checks, covering hash-verified restoration, interrupted/canceled rollback, credential decryption, locked/corrupt inputs and directory identity preservation.

Evidence: D:/UDM-Workspace/candidates/native-084-backup/control/build-object-routing-acceptance.json. Current object, script and restore executable hashes are recorded. Protected personal binaries/catalog remain unchanged.

This establishes path selection for these scripts, not automatic source-to-object freshness or a reproducible clean build. Existing objects must still be rebuilt after their sources/dependencies change; build-recovery-host.ps1 rebuilds the three overlay engine objects. Other fixture binaries were not rebuilt merely because their scripts changed. Diagnostic targets remain excluded from packaging. Full IDM parity and release qualification remain unfinished.
