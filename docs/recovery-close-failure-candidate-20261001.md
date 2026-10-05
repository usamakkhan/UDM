# Save failure during recovery handoff — staged candidate

The recovery command now launches its helper only after stopping transfers and successfully saving the catalog. A failed final save keeps the main window open, restores browser integration, resets the engine shutdown state and allows another attempt. No waiting recovery child is created before the save succeeds.

Verified on the rebuilt candidate:
- Nine actual GUI/process checks passed with a Windows handle denying replacement of a disposable catalog: the original app stayed open, saved bytes stayed intact, no recovery child appeared, browser ping worked, and a later retry completed the recovery/open-app handoff.
- Six engine checks passed: failed save is reported, old catalog stays intact, a queued loopback download starts and completes with the expected hash after the failure, and saving succeeds after the lock is released.
- Installed app/host/monitor and the personal catalog are unchanged; no personal browser session was touched.
- The changed Core.cpp and Bridge.cpp are compiled into separate candidate objects. build-app.ps1 builds and links both through build-recovery-host.ps1; base native83 objects remain intact.

The initial engine fixture checked for completion before pumping the scheduler. It failed with the job still Queued. The corrected fixture calls Manager::tick as the actual app does and passed; the initial failure remains in close-failure-engine-1/results.json.

Evidence: D:/UDM-Workspace/candidates/native-084-backup/control/close-failure-acceptance.json.
The complete native suite was not rerun in this continuation; these checks target shutdown behavior and a real post-failure transfer. Actual disk exhaustion, power loss, in-flight browser transitions and helper-launch failure remain unqualified. If helper launch itself fails after saving, the error is shown and the app finishes closing; automatic reopening is not implemented. Versioning, installer integration and personal activation remain pending. Full IDM parity is not established.
