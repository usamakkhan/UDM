# Persist DASH media and initialization ranges

Build complete: all five native production targets and the UDM 1.1.0 / extension 0.63.64 installer compiled successfully. Evidence: validation/native-dash-range-recording-build-20261028.json. The candidate is unsigned, staged, uninstalled and unpublished.

Planned segment references now carry an optional start/length byte range, and resource windows carry an optional initialization range. Capture includes segment ranges in the protected resource identity and records them before requests. Refresh checks compare the range along with number, timestamp and duration, so an existing segment cannot silently switch between whole-resource and ranged data or change its slice.

Initialization ranges are persisted alongside pending initialization URLs before download, then retained with the verified initialization receipt. Completed and pending initialization range changes within a Period are rejected. Missing resource URLs, malformed coordinates and dangling pending range fields fail validation. Whole-resource records omit the optional fields and retain their existing receipt bindings.

Offline reconciliation reconstructs the same media and initialization bindings, verifies range lengths and resource hashes through the resource store, and preserves range fields when promoting a saved receipt. Stop and sealed assembly use this reconciliation path, so ranges remain attached to captured data across restart, cancellation, finalization and session-output binding. The previous signed/unsigned status-check warning was also corrected by matching the HTTP status type.

This connects range transfer to the recording layer; it does not yet add live SegmentList/SegmentBase planning or browser admission. Shared resource-version consistency for index-based files remains necessary before enabling those paths. Current template planners continue to emit whole resources. Runtime range, restart and playback tests remain deferred at user request. Extension 0.63.64 is unchanged, and installation/publication are not performed.
