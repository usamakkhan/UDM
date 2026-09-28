# UDM 0.63.0 / browser 0.41.0

Site Grabber now supports manual browser sign-in and saved, scoped website cookies. This advances the manual authorization workflow described in [IDM's starting-page guide](https://www.internetdownloadmanager.com/support/idm-grabber/starting.html). Full IDM parity remains unestablished.

## Use browser sign-in

1. In **Site Grabber → Name and starting page → Advanced**, select **Enter login and password manually in the browser**.
2. Set the login page and any logout addresses to avoid. Choose **Sign in now** (or proceed to exploration).
3. Sign in normally in Chrome, Edge or Firefox. Return to a page on the project's starting website.
4. Open **UDM Browser Integration**, select the project and click **Use this signed-in session**. Grant cookie access if prompted.
5. Return to Site Grabber. A requested exploration starts after successful handoff; otherwise select **Explore**.

A sign-in request lasts 15 minutes. Login pages may use a different origin, but session handoff must occur on the project's exact starting origin. Reopen Advanced to sign in again or clear the saved session. HTTP Basic/Digest remains available as the separate username/password option.

## Behavior and recovery

The desktop encrypts the cookie snapshot with Windows account protection. Cookies are selected from the current tab's store/container and matching partition, and are scoped again for each native request and redirect by origin, path, Secure flag and expiration. Logout patterns exclude requests during exploration and downloads. Password form fields are not sent to UDM.

New file downloads, metadata checks, redownloads, synchronization and offline ZIP requests retain the project's session. Ordinary catalog exports omit cookie secrets and mark affected records for browser recapture through Refresh download address; explicitly protected exports preserve them for the same Windows account. Templates omit sessions and pending tickets. Corrupt saved sessions require sign-in again.

Failed saves roll back. A wizard save racing with a completed browser sign-in preserves that exact ticket's receipt. Expired tickets cannot complete, replayed tickets cannot replace cookies, and oversized cookie headers are rejected before creating downloads. The extension does not persist transferred cookie values. Disabled-tab video-panel state is now returned independently of desktop policy refresh, so a delayed desktop reply cannot restore a disabled panel after reload.

## Verification

**2,381 checks passed:** 1,598 native, 658 browser units, 8 native-protocol, 8 app/host, 40 Chrome/Edge capture/media scenarios, 33 offline-website checks and 36 sign-in scenarios across Chrome, Edge and Firefox. [Evidence](evidence-0.63.0/summary.json) · [Workflow inventory](idm-parity-0.63.0.md).

The sign-in acceptance uses an actual login form, HttpOnly session and persistent cookies, the actual popup button and native messaging. It closes the browser and app, then reopens the saved project and verifies protected downloads byte for byte, authenticated metadata/range requests, logout exclusion and redirect cookie paths. It also creates an offline ZIP and reads back the authenticated starting page. Firefox also exercises the browser's own cookie store API. Personal accounts and profiles are not used.

The first real Chrome test exposed a milliseconds/seconds conversion defect: tickets expired in 900 milliseconds instead of 15 minutes. Both ticket duration and persistent-cookie expiry now have regression checks and real-browser coverage. Development failures and fixes are retained in [the log](evidence-0.63.0/DEVELOPMENT.md). Compiler warnings and native visual limitations are recorded rather than treated as acceptance.

## Remaining limits

- This release saves cookie-session snapshots. It does not apply subsequent server Set-Cookie rotations, localStorage tokens or arbitrary custom authorization mechanisms.
- Site Grabber still parses static HTML. Rendered-page traversal, browser-cache reuse and some offline ZIP hierarchy/import cases remain open.
- New sign-in changes future project downloads. Existing downloads retain independent session snapshots; Refresh download address can explicitly replace a file snapshot from a new browser capture. An offline archive with omitted credentials must be recreated through Site Grabber.
- Native visual, DPI and accessibility acceptance remains unverified because the prior Windows control initialization failed to write kernel assets.
- Chrome, Edge and Firefox sign-in acceptance uses real isolated browsers, the actual extension popup/native host, generated localhost credentials and byte-exact files. It does not establish public-site or internet-speed parity.
- The user confirms reloading the updated Chrome and Edge extensions and video tabs; personal-profile runtime/visual activation remains independently unverified. Driver equivalence, publisher signing, updater and clean-machine installation remain open.
- Full IDM feature/design parity remains unestablished; check counts are not a parity percentage.

## Current-PC deployment

[UDM 0.63.0 installer](../installer-out/UDM-0.63.0-Browser-0.41.0-Setup-x64.exe) is built. 89 files were deployed with SHA-256 verification and original-file backups. Installer SHA-256: `DFD073090B37461C7560EEDFD45F7675A4A96FAF81AF0395F8B7E714ED524EA5`.

The running app/native bridge reports 0.63.0 and the correct D:/UDM/user-data catalog. All 23 records, queues and saved preferences are unchanged. Browser 0.41.0 sources are installed; the signed network runtime is unchanged. Both unpacked Chromium folders are updated. The user confirms reloading UDM Browser Integration in Chrome and Edge and refreshing video tabs. This is recorded as user confirmation; personal-profile runtime and visual acceptance remain independently unverified. The [installation log](evidence-0.63.0/installation.log) is retained with the release evidence.
