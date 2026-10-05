# Completed download scanner-wait correction — staged candidate

The preceding completed-job Stop guard was too broad: UDM can mark file transfer Complete while its worker still waits for a configured scanner. ProgressUi explicitly routes Stop in that state to Manager::pause.

A real benign scanner helper reproduced the regression: the completed file stayed intact, but Stop did not end UDM's wait. Manager::pause now signals an existing worker's cancellation token before returning for a completed job. It still avoids modifying completed-job queue metadata or saving an idle completed job. The scanner process itself is never terminated by this change.

Verification:
- Corrected baseline: 15 passed / one failed (scanner-wait cancellation).
- Fixed engine: all 16 passed, including completed-file preservation, interrupted scan status and proof that the scanner helper continued to its normal completion marker.
- All 69 existing native menu/transfer checks passed against the new core.
- GUI sources matched their earlier compiled acceptance before relinking the app and fixture.
- Personal catalog and installed binaries remain unchanged.

The first scanner fixture attempt changed settings before its prior worker had fully finished and shared marker names. It observed the wrong worker. The harness now waits for worker completion and uses per-file markers. Its original failed results remain in scanner-stop-before/results.json; the valid baseline is scanner-stop-baseline/results.json.

Evidence: D:/UDM-Workspace/candidates/native-084-backup/control/scanner-stop-acceptance.json.
No real antivirus configuration was changed; all scanner processes were owned disposable helpers. Full scanner/queue concurrency coverage and IDM parity remain unestablished. Candidate is not installed or packaged.

Next reference-backed menu gap: IDM's Downloads > Speed Limiter has Turn on, Turn off and Settings. UDM currently exposes only its global dialog, whose disabled state also loses the remembered rate. This was inspected but not implemented in this continuation.
