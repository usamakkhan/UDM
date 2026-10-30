# Reconcile saved DASH media before Stop

Build complete: all five native production targets and the UDM 1.1.0 / extension 0.63.63 installer compiled successfully. Evidence: validation/live-dash-stop-reconciliation-build-20261028.json. The candidate is unsigned, staged, uninstalled and unpublished; runtime behavior remains unverified.

Stopping live DASH now reconciles the current Period from its protected recording index and resource receipts before deciding whether to exclude an empty newly started Period. It checkpoints the reconciled state into the session journal and refreshes progress before the existing sealing path.

Capture commits media and recording checkpoints incrementally. If Stop cancels the capture before it returns, the worker's previous in-memory index can still describe an empty Period even though saved resources exist. Using that stale snapshot for the empty-tail decision could leave complete media out of the saved session. The new path uses the parent cancellation token, so Stop can recover committed media while ordinary job cancellation still aborts recovery.

The existing empty-tail restriction remains: only a later unsealed Period with no committed segments can be removed from the assembly list. Partly captured tracks are retained, and an unusable common audio/video interval still produces an error instead of silently discarding recorded segments. No recording files are deleted. Runtime Stop, cancellation and playback tests remain deferred at user request.
