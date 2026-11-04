# Background file-type icons

Development version 1.21.0; browser source remains 0.63.85.

The list still called SHGetFileInfoW synchronously after the 1.20.0 manager-lock change. The [Microsoft API documentation](https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shgetfileinfow) recommends a background thread, requires COM initialization and assigns ownership of returned icons to the caller.

A dedicated loader now performs the association lookup in a COM-initialized STA with a message pump. It receives extension strings only and retains no window, manager or image-list reference. The UI immediately uses a cached association icon or the current row's category fallback. It requests new associations only for the viewport and a small look-ahead; regular refreshes and completed-scroll notifications apply results and retry requests when the bounded queue has capacity. Hidden lists do not enqueue more work.

Current-generation pending/in-flight/completed requests share a 128-extension budget (one obsolete lookup may still be returning after invalidation) and are deduplicated. Returned HICON values have explicit move-only ownership and are destroyed after the UI copies them into its image list or discards them. Failed lookups cache a failure marker, allowing each row to retain its own category fallback rather than reusing another category's icon.

Image-list rebuilds invalidate queued/completed work by generation, including appearance/DPI rebuilds. A lookup already in progress may return later, but its old result is discarded. Loader shutdown clears queued/results state and wakes an idle worker. It does not join a potentially blocked Shell call; shared state remains valid until that worker and COM cleanup finish. The running marker is retained through COM cleanup, preventing a failed worker from being replaced while it is still exiting. Existing app shutdown work retains its own behavior.

A blocked Shell extension can leave uncached associations on category icons until the call returns; it no longer holds the main window's list refresh in that call. Startup, first-open timing, large-catalog scrolling, DPI/appearance changes, Shell failures and shutdown interaction require runtime acceptance. No measured responsiveness or exact IDM timing claim is made.

Build status: passed for all five production targets. Embedded versions and SHA-256 hashes are recorded; feature source hashes matched the pre-build snapshot. Runtime tests, installer creation, installation and publication remain deferred. Installed UDM remains 1.6.0 by the last direct check. See [build receipt](validation/background-file-icons-build-20261104.json).
