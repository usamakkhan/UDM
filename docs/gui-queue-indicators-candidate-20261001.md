# Queue state indicators — staged candidate, 1 October 2026

UDM now shows queue-state glyphs in the Q column and queue tree. This follows the semantic distinctions documented in [IDM’s queue guide](https://www.internetdownloadmanager.com/support/idm-scheduler/idm_queues.html): gold sheets for download queues, green sheets for synchronization, three sheets for the main queue and two for additional queues, with a clock for a configured scheduled/one-time start. UDM draws its own artwork in native code; no IDM image assets are used.

Q cells are icon-only. Main queue membership is now visible, rather than blank. Removing membership or referencing a missing queue clears the image. Editing queue type or schedule updates list and tree indicators on refresh. Native row infotip handling supplies the file name, queue name/type, configured schedule, and stopped state. Clock badges describe configured starts, including stopped queues; they do not promise an imminent download. Existing Q sorting continues to use membership names.

## Verification

- 348 native checks passed, including real list-subitem and tree image assignments for all eight states, explicit image clearing, native infotip routing, and the accumulated GUI/transfer regressions.
- Eight glyphs rendered distinctly at 16, 20 and 32 pixels. Reviewed the actual masked image-list contact sheet on light and dark backgrounds.
- 15 separate actual-app checks passed: ordinary/nonmember/scheduled synchronization records, icon-only cells, removal, Add/Move dialog actions, cancellation, close and restart.
- First app-harness run asserted row count before asynchronous population completed. The corrected harness waits for the identical three-record invariant, then passes in a fresh profile with unchanged application binaries. The failed result is retained.
- Installed binaries and personal catalog retain their protected hashes.

Evidence: `D:\UDM-Workspace\candidates\native-084-backup\control\queue-indicators-acceptance.json`, `queue-indicators-evidence`, `queue-indicators-app-1-results.json`, and `queue-indicators-app-2-results.json`. Reproducible app harness: `queue-indicators-app-2.py`. Implementation: `native\QueueIndicators.hpp` and `native\App.cpp`. Candidate executable: `build\UDM.RecoveryCandidate.exe`.

## Remaining scope

Staged, not installed. Full IDM parity remains unestablished. Native notification routing is tested, not a full screen-reader or physical-hover audit. Arbitrary display scaling, exact live IDM pixels, default synchronization-queue provisioning, and the complete context-menu layout remain unqualified. Main-versus-additional classification currently follows UDM’s reserved Main queue identity; this change does not create a separate default synchronization queue. No driver, certificate, browser, or personal download changed.
