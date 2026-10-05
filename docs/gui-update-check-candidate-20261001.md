# Help: Check for updates — staged candidate

Help now has Check for updates at the installed reference menu's update-command position. The command checks the public GitHub latest stable release endpoint for usamakkhan/UDM asynchronously, compares numeric version components, and offers the validated repository release page. The dialog shows the running version, result/error, Check again, Open release page and Close.

The request has a 15-second deadline and 256 KiB response bound. Redirects are disabled. The parser rejects draft/prerelease responses, unsupported version syntax and non-HTTPS/foreign-repository release pages. Failed requests do not claim that UDM is current. Duplicate checks are disabled while a request runs; closing cancels and joins the worker. Checks do not write download records or settings, and no installer is downloaded or executed automatically.

## Verification

- 15 deterministic metadata/version checks pass.
- The rebuilt main native suite passes 271 checks, including Help placement and command availability.
- 21 actual-dialog checks pass for available/current results, failure, recovery after retry, duplicate-request prevention and closing a pending worker.
- Five main-app checks pass using the real GitHub endpoint. It returned latest stable v0.78.0; the running candidate is 0.83.0, so the dialog reported no newer stable release. The release-page button was enabled, but the test did not open a browser or download an installer.
- The isolated download records and protected installed UDM/catalog hashes remained unchanged.

[Acceptance](D:/UDM-Workspace/candidates/native-084-backup/control/update-check-acceptance.json) · [Model checks](D:/UDM-Workspace/candidates/native-084-backup/control/update-model-results.json) · [Dialog checks](D:/UDM-Workspace/candidates/native-084-backup/control/update-ui-app-results.json) · [Real response](D:/UDM-Workspace/candidates/native-084-backup/control/update-live-response.json).

This candidate is uninstalled. Automatic update scheduling, signed installer verification/replacement, upgrade rollback, complete Help-menu equivalence and full IDM parity remain unfinished. The check does not promote this candidate to a released version or establish clean-machine installation.
