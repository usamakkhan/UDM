# Complete collected browser batches

Source review found a mismatch: context-menu collection retained up to 500 links, while links.html, links.js and the batch-download handler accepted only 100. Selecting all in a larger collected list disabled the submission button, and the handler independently rejected the same selection.

Both Chromium and Firefox now permit all 500 rows retained by that collection path. The review text, button state, submission guard and background validation agree. The initial status includes the number being submitted. Extension version is 0.63.51.

The handoff remains sequential, with validation before admission and the busy marker persisted before the first handoff. There is no new automatic retry: partial failures still report the acknowledged count and warn that the last request may also have been accepted. Native request size limits are unchanged because each link is handed off separately. The existing collection cap, 512 KiB stored-list bound and eight-list retention bound remain intact.

The separate in-page selected-link picker retains its existing 100-link input bound. This change addresses the context-menu collected-list mismatch, not unlimited batch size or proof of IDM-scale throughput.

JavaScript syntax, mirrored-source equality and manifest-version checks were performed. Runtime, browser and regression tests remain deferred at the user's request. The updated extension is staged in the installer; it is not installed or activated in Chrome.
