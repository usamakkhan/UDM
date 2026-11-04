# Relocation verification progress — 1.11.1

Move/Rename now reports byte progress during final destination verification and original-file verification after a failed or cancelled copy. Verification uses a read handle denying concurrent writes, checks its SHA-256 against the move journal, and reports completion only when the checksum matches. Same-volume identity verification retains its existing fast path.

Rollback now uses the Windows progress callback, reporting copied bytes while restoring the original location. This recovery phase deliberately ignores late cancellation so the dialog cannot abandon an already moved file midway through restoration. Cancellation remains available during initial hashing and the forward copy.

The dialog keeps the destination path in its own row while a separate row shows phase byte counts. The progress bar is phase-local, not a promise of overall completion: reaching the end of copying is followed by verification and history saving.

All five native production targets built successfully at 1.11.1; source hashes remained unchanged. Runtime and visual checks, installer creation, installation and publication are deferred by user request. Installed UDM remains 1.6.0 / extension 0.63.82; browser source remains 0.63.84. Full IDM parity is unproven. Build evidence: validation/move-verification-progress-build-20261103.json.
