# Browser capture presentation before history refresh

The immediate capture notification still invoked the full main-window refresh:
history traversal and sorting, row painting, clipboard checks and completion
dialog handling all preceded File Info. A large history or unrelated dialog
could therefore delay an accepted browser download.

The notification now uses a capture-only refresh. It retains the modal reentry
guard, durable pending-capture lookup, duplicate and replacement handling, and
normal File Info/prefetch flow. History refresh, progress/completion events,
authentication prompts and recovery reviews remain on the ordinary refresh path.
The normal timer still discovers captures deferred while another modal is open.

The isolated native presentation suite passed 129 checks. Its new regression
changes the requested history filter before the presentation message, verifies
that dialogs are handled without rebuilding that history view, and then lets
ordinary refresh resume. Existing checks cover independent ownership, visible
File Info, duplicate choices, durable recovery and final-size updates during
modal loops. Results: `capture-priority-20261020.json`.

This proves removal of history refresh from the notification path; it does not
measure live Chrome click-to-dialog latency. Browser-side metadata and native
admission still precede the notification. The build is now installed and hash-verified (capture-priority-installed-20261020.json). The native host round trip passed; live browser timing remains pending.


## Live desktop verification

Chrome's 1080p H.264 selection opened Download File Info while the main window
was absent from the native window inventory. Before Start, File Info displayed
56731539 exact final bytes and reported that the file was prefetched and verified.
After Start, the completed file contained exactly 56731539 bytes, with
ContinuedMediaPrefetch=true. Its SHA-256 matched the earlier fully decoded sample.
Evidence: `browser-capture-priority-20261020.json`.

The first native observation occurred 8651 ms after the browser click began;
this includes tool and agent latency and is not a measured click-to-paint delay.
The extension runtime version was not verified, so this establishes the installed
desktop's focus/prefetch/size behavior, not the 0.63.23 browser timing improvement.
