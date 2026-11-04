# Responsive replacement publication — 1.2.9

Final replacement verification now snapshots the destination and replacement identifiers under the manager lock, releases that lock, opens the original file with writes denied, and hashes its held Windows handle with cancellation between blocks. It then reacquires the lock, checks cancellation again, and revalidates catalog membership, destination, replacement identifiers and file identity before entering the existing journal/replace/save transaction.

The held handle survives until ReplaceFileW completes. Failure cleanup can confirm that the destination still identifies that unchanged, write-protected original without another full-file hash under the lock. The previous-version backup, durable journal, metadata update and catalog-save rollback remain in the commit section. The earlier synchronization preparation also uses held-handle hashing, rather than reopening a path.

Direct transfer, shared media publication, live HLS and offline archive callers now pass their cancellation object. Live HLS no longer wraps publication in an outer manager lock; other production call sites were inspected for outer locks. External callers must honor the documented no-outer-lock contract. Existing three-argument callers retain the optional cancellation default.

This reduces blocking during replacement verification; it does not make atomic file replacement, catalog writes, startup journal recovery or ordinary duplicate-dialog preparation nonblocking. Runtime timing, cancellation, edit races and rollback acceptance remain deferred under the user's testing instruction. Existing DuplicateChecks.hpp scenarios were inspected but not executed.

Native version is 1.2.9, extension 0.63.82 unchanged. All five production binaries compiled as 1.2.9 and installer packaging passed. Source hashes remained unchanged during the build. See [build evidence](validation/publication-responsive-build-20261030.json). Installed UDM remains 1.2.8.
