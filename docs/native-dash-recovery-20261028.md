# Native DASH receipt reconciliation

The native DASH module now provides an offline recording reconciliation function. It decrypts the recording index, checks the selected-track digest and each segment's identity, verifies contiguous sequence/timing, and reads the corresponding encrypted resource receipt. A resource is accepted only when its binding, size and SHA-256 match the current file. Pending index entries with valid receipts become committed; invalid or missing receipts clear completion/resource metadata without deleting media files.

The same receipt inspection function is used by the downloader's cache reuse path, so offline recovery and active reuse apply the same byte verification. Recovery resolves files from hashed bindings instead of trusting arbitrary paths stored in the index. Directory ancestry, index, receipt and resource paths are checked for filesystem redirection. Cancellation is checked during parsing loops and hashing. The updated index is written atomically only after reconciliation finishes successfully.

New initialization records retain their URL list and binding key. This allows verification of both already-indexed initialization files and pending initialization downloads that completed before an index checkpoint. Changed initialization bytes within a Period fail explicitly. Older initialization entries lacking the binding inputs remain unverified rather than being promoted from a file-name claim. Pending initialization disappearance is also rejected when an initialization was previously recorded.

The function has no network path and does not erase files. Its result describes verified local data, not whether missing segments can still be obtained or whether the recording is complete. Server-side changes to a logical segment require the separate source-validation policy noted in `native-dash-storage-20261028.md`.

## Remaining integration

The startup/resume job path still needs to invoke reconciliation, choose capture windows and connect the existing native planning/capture functions to final assembly and browser admission. Recovery should run at resume or explicit verification, not rehash an entire growing recording on every playlist poll. Runtime and regression tests remain deferred at the user's request.

Production compilation and packaging are recorded in `validation/native-dash-recovery-build-20261028.json`. No downloads or runtime recovery scenarios were executed. Browser 0.63.61 is unchanged; the candidate remains unsigned, uninstalled and unpublished. Full IDM parity is not established.
