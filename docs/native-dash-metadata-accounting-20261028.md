# Live DASH metadata transfer accounting

The native live worker now accounts for successfully read MPD and HTTP clock-response bodies through the same manager rate/quota path and shared per-job Rate object as its media workers. This covers initial capture, resume bootstrap refreshes, subsequent manifest refreshes and clock resynchronization. Conditional 304 responses, direct clock declarations and HEAD timing responses add no body bytes.

These bytes increment `TransferredBytes` and a separate `AdaptiveMetadataBytes` counter. They do not inflate the received-media total, receipt sizes or final output size. The accounting unit is the bounded response body, consistent with the existing HLS playlist path; this does not count headers, TLS overhead, or partial reads that fail before the bounded body reader returns.

Bandwidth and quota waits use the parent capture cancellation token. Their elapsed time extends the metadata network timeout budget, so a user-configured wait does not itself masquerade as a network timeout. Manifest receipt and clock-response midpoint anchors are recorded before accounting waits. Clock freshness is checked again after waiting, and MPD validity still advances with real elapsed time: waiting does not make old metadata fresh or distort a clock sample's measured network round trip.

All five production targets and the installer compiled successfully, recorded in `validation/native-dash-metadata-accounting-build-20261028.json`. Runtime quota, low-bandwidth and cancellation tests remain deferred at the user's request. Browser 0.63.62 is unchanged. The candidate is unsigned, uninstalled and unpublished.
