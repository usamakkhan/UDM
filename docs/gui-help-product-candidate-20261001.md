# Help product actions — staged candidate

The Help menu now has all 13 reference entries/separators in the installed IDM resource's order, with UDM branding. UDM Home Page opens the project repository; Contact UDM Support opens its issue tracker. The repository API confirmed that usamakkhan/UDM is public, unarchived and has issues enabled.

Tell a Friend opens a dialog with the project URL, Copy link, Email and Close. Email passes a recipient-free mailto draft URI to the default email application; the app has no send operation. Clipboard and launch failures report errors rather than announcing success. Merely opening/closing the dialog performs neither action.

The existing Browser integration and Network integration tools moved from Help to Downloads > Options > More. Browser edits still use the existing draft settings flow. Network diagnostics opens without starting its signed monitor.

## Verification

274 native checks pass, including the reference Help entry count, product action positions and availability without download selection. Ten actual share-dialog checks pass using injected clipboard/launch sinks: correct link, no action on close, one copy/draft action per click and unchanged personal clipboard sequence. Seven actual-main-app checks pass for exact branded Help labels/separators, opening Share, both preserved integration routes, canceled-setting/download preservation and normal exit.

The first main-app harness tried a hidden tab's More button. The final fresh-profile run restricts selection to visible controls and passes against the unchanged final binary. That first attempt remains recorded, not counted as successful acceptance.

[Acceptance](D:/UDM-Workspace/candidates/native-084-backup/control/help-product-acceptance.json) · [Native checks](D:/UDM-Workspace/candidates/native-084-backup/control/help-product-native-results.json) · [Share checks](D:/UDM-Workspace/candidates/native-084-backup/control/share-ui-app-results.json) · [Main app checks](D:/UDM-Workspace/candidates/native-084-backup/control/help-product-app-2-results.json).

The installed app/host/monitor and personal catalog hashes remain unchanged. The candidate is uninstalled. Default browser/email launching, real clipboard failure cases, sharing-dialog visual equivalence and full app parity remain unqualified; the tests did not launch the personal email client or start the driver monitor.
