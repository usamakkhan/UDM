# DASH recording version persistence

All five native production targets and the UDM 1.1.0 / extension 0.63.64 installer built successfully. Logs, source hashes and the installer checksum are recorded in [the build record](validation/native-dash-version-recording-build-20261029.json). The candidate is unsigned, staged, uninstalled and unpublished.

Media references and initialization windows now carry optional resource-version descriptors. Capture includes them in the existing protected segment and initialization identities, checkpoints them before resource requests, and passes the same descriptors to the conditional resource store. A refresh cannot replace an indexed segment's version or change completed or pending initialization versions within the Period.

Offline reconciliation reconstructs the original version-bound cache keys before accepting a saved resource. It preserves initialization versions when promoting receipts and clears the pending version only when the initialization is promoted or legitimately absent. Orphaned pending version metadata and changed initialization versions are rejected. Legacy recordings without version descriptors retain their previous identities.

Before capture starts and before reconciled state is committed, a recording-wide check rejects conflicting strong ETag or length descriptors for the same exact resource URL across initialization and media in every selected track. This prevents two explicitly versioned slices of one file from being combined under different versions. It does not infer identity across different URLs or supply missing versions automatically.

Live list admission remains gated. Version discovery and assignment to planned windows still need implementation, and SegmentBase needs native index parsing. This batch completes persistence and consistency checks for supplied descriptors; it does not claim end-to-end live SegmentList support.

Runtime tests remain deferred at user request. Extension 0.63.64 is unchanged. No installation or publication is performed.
