# Desktop row refresh cache

The 500 ms desktop refresh previously recomputed every visible row's text, target path, dates, queue image, and optional metadata, even when the download had not changed. It now skips formatting for a row when its saved data, current speed, quota-wait state, settings, tree/queue state, and optional column visibility are unchanged. List rebuilds and image-list changes invalidate the cache.

Validation on 2026-10-13:

- Rebuilt the native MFC app and menu-state test binary from the current source.
- The menu-state suite passed 377 checks, including repeated refresh of a completed row, a changed description, and clearing that description.
- An isolated app instance opened a 5,000-download catalog; the actual list view reported 5,000 rows. Twenty window-response probes after startup returned in 0.1 ms each on this host.
- The 5,000-download native catalog benchmark loaded in 89, 92, and 92 ms before the GUI edit, helping isolate row rendering from catalog loading.

The response probe is a candidate smoke check, not a matched before/after speed measurement or proof of faster download transfers. This build was not installed or released.
