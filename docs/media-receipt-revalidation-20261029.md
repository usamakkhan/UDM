# Accepted media receipt revalidation

The recovery UI's Check receipt action previously returned the cached accepted record immediately. It never queried media-status for an accepted receipt, even though the native endpoint reports whether the accepted job is still present. Re-selecting that media also reused the cached present flag.

Accepted receipts now query the running desktop before both explicit review and duplicate-handoff recovery. When the original receipt is still available, its current history-presence flag is saved. A removed history record therefore follows the existing review message instead of being returned as a current download. No automatic replacement download is created.

A reply that no longer proves the same accepted job ID cannot erase or downgrade historical acceptance. Missing, expired, released or conflicting answers preserve the saved receipt and ask for review of the original desktop profile. Closed released receipts retain their immutable local outcome. If a notice is cleared concurrently while a status request is in flight, the check reports that fact instead of returning an empty success.

These checks use preferences-if-running and media-status. They do not replay media requests or start a download. The first handoff path is unchanged; the additional status query occurs only for review or recovery of an already accepted receipt.

Chromium and Firefox version 0.63.75 passed syntax checks. Native sources are unchanged. Installer packaging passed. See [build evidence](validation/media-receipt-revalidation-build-20261029.json); the production bundle manifest contains all 147 payload hashes. Runtime, browser and receipt-recovery tests remain deferred at user request. No installation or publication is performed.
