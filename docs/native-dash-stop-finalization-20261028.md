# DASH recording stop finalization

The native `sealDashRecording` operation reconciles resource receipts and freezes a recording's complete common interval. For each selected track it uses the contiguous committed prefix, stopping at the first unfinished segment. It intersects track coverage, respects a saved capture start, and requires full coverage of an explicitly requested end. A track with no complete prefix or unresolved initialization cannot be sealed.

Segments wholly outside the chosen interval move to `excludedSegments` in the protected index. Their records, receipt files and media bytes remain available; the operation deletes no resource files. This handles a pending tail left when Stop interrupts a request without representing that tail as downloaded. Missing media inside the chosen interval still fails. Assembly's selected tracks retain the segment overlap needed for packet-granular trimming.

The chosen start and end and the sealed flag are saved atomically. Capture rejects a sealed recording, and repeated sealing reconciles the saved receipts without changing the chosen interval. File corruption can still make later assembly fail; sealing is not a substitute for media validation.

This resolves a prerequisite found while reviewing job stop integration. The running job dispatcher still supports live HLS only. Live DASH polling, job controls, browser admission, Period transitions and publication remain unfinished. The job must call sealing only for finalization, not for Pause, and must use its parent cancellation token after a user Stop cancels the capture token.

All five production targets and the installer compiled successfully; see `validation/native-dash-stop-finalization-build-20261028.json`. Runtime testing remains deferred at the user's request. Browser 0.63.61 is unchanged. The candidate is unsigned, uninstalled and unpublished.
