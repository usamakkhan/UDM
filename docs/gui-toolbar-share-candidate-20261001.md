# UDM toolbar action completion — staged candidate, 1 October 2026

The default toolbar now includes Tell a friend after Grabber, matching the twelve-action order in [IDM’s published toolbar specification](https://www.internetdownloadmanager.com/support/make_toolbar.html). UDM uses its existing link icon; external skins use their twelfth frame. The action opens UDM’s existing sharing dialog. It does not automatically send mail.

Explicit command/index mapping prevents the twelfth slot from invoking Add batch download. Enablement, tooltips, native customization and saved layout conversion use that mapping. Existing explicit layouts, including eleven-button and empty layouts, remain unchanged. Reset restores twelve actions. Icon-and-text minimum button width is 55 logical pixels, preserving image size while fitting the twelve actions at the tested 950-pixel client width/display scale.

## Verification

- All 23 native engine objects rebuilt consistently with the shared toolbar model; staged application and test executables rebuilt.
- 87 toolbar model/actual GUI checks passed in each of three fresh isolated profiles: command dispatch, tooltip, default order, legacy settings, add/remove/move/reset, persistence, three sizes by three caption modes, visibility, skin loading/fallback, dark arrows and narrow-window wrapping.
- 274 accumulated native GUI/transfer regression checks passed.
- Reviewed own-window light, dark and independent test-skin renders. Labels remain readable; default toolbar stays on one row. Synthetic skin rectangles are intentional fixture images.
- Installed binaries and the personal catalog retain their protected SHA-256 values.

## Retained failures and limits

The first run correctly failed because the last action wrapped; padding was corrected. The second run timed out after 180 seconds without a completed result. Persistent assertion checkpoints were added to the fixture, then three fresh runs passed without another application change. The timeout cause is not established; these runs do not prove it impossible. Failure evidence is retained, not counted as a pass.

This candidate is not installed. No personal browser session, certificate, driver or download changed. This closes the missing toolbar action, not all GUI or full IDM parity. Full accessibility, arbitrary fonts/DPI and broader product gaps remain unverified.

Evidence: `D:\UDM-Workspace\candidates\native-084-backup\control\toolbar-share-acceptance.json` and sibling `toolbar-share-evidence` directory. Source: `D:\UDM-Workspace\candidates\native-084-backup\native`. Binary: `build\UDM.RecoveryCandidate.exe` in that candidate.
