# UDM 0.62.0 / browser 0.40.0

Site Grabber now saves a website-specific HTTP login and carries a link's text into collected-file and download descriptions. This follows the workflows described in IDM's [starting-page guide](https://www.internetdownloadmanager.com/support/idm-grabber/starting.html) and [Grabber settings guide (old version)](https://www.internetdownloadmanager.com/support/idm-grabber/settings.html). Full IDM parity remains unestablished.

## Using the update

Open **Site Grabber**. On **Name and starting page**, enable **This website requires a user name and password** for a site that uses HTTP authentication. The password field is masked. Save or explore the project as usual. The login follows page requests, parallel metadata checks, range fallback, newly added downloads and offline ZIP creation.

On the review page, **Settings → Use link text as the download description** controls description collection. It is enabled by default. The new Description column and **Collected file properties** show the extracted text. Newly added downloads carry it into their ordinary Download properties. Existing downloads retain their own edited descriptions and login details.

## Reliability and credential handling

- Windows protects the saved project login. Templates exclude the login and starting address. Passwords and Authorization values are not written as plaintext project fields.
- Credentials apply to the exact starting origin, including scheme and port. Requests to other origins do not inherit that project password. Changing the starting origin requires clearing and re-entering the login.
- Unreadable saved credentials leave the wizard editable and require re-entry or clearing. They do not silently become an anonymous scan. Failed project saves restore the previous project.
- An unchanged login preserves scan continuation; a changed login invalidates the old checkpoint. Descriptions survive stopped scans and restart. Malformed or oversized saved descriptions are rejected.
- Text extraction retains Unicode, decodes supported entities once, collapses whitespace and uses image alt text for image links. Comments, scripts and stylesheets are excluded, including unclosed raw-text elements. The first nonempty label is retained for repeated links.

## Verification

**2,276 checks passed:** 1,548 native (45 new), 639 browser regression, 8 real native-protocol, 8 isolated app/host, 20 extension/native scenarios in each of Chrome and Edge, and 33 offline website checks. [Evidence](evidence-0.62.0/summary.json) · [Workflow inventory](idm-parity-0.62.0.md).

New local HTTP tests cover valid and wrong credentials, server-verified Digest responses, authenticated parallel metadata and range fallback, real queued file bytes, protected persistence, save rollback, cross-origin redirects and offline ZIP creation. The whole native suite also covers existing transfer, queue, recovery, proxy and media workflows.

The offline acceptance test downloads a small site through the native Grabber, stops its server, and opens the saved files in Chrome and Edge. Both browsers load local styles, images, scripts and navigation without HTTP requests. Browser capture tests use generated localhost media and isolated profiles; these results do not establish public-site or internet speed parity.

The initial 269-check focused run passed. Review added four boundary/authentication checks before the full run. An overlapping development compile failed with C1041; a sequential rebuild succeeded. Final compilation reports shadowing/sign warnings. Windows UI control again failed during initialization with `failed to write kernel assets: The system cannot find the path specified. (os error 3)`, so no visual acceptance is claimed.

## Remaining limits

- Native visual, DPI, keyboard and screen-reader acceptance remains unverified: Windows computer-use initialization fails with a missing kernel-assets path.
- Chrome and Edge acceptance uses isolated profiles and localhost files/media. Public-site coverage, current YouTube compatibility and matched-server internet speed parity are not established by these tests.
- Project login supports HTTP Basic and native Digest negotiation, not manual browser sign-in, cookies, OAuth or rendered-page exploration. Browser cache reuse remains open.
- Changing project login affects future collected downloads. Previously added downloads keep their independent login and description; edit their Download properties as needed.
- Link text is extracted from static HTML, with common/numeric entities, inline formatting and image alt text. It is not a full browser-rendered accessibility name or arbitrary HTML/CSS visibility computation. Descriptions are capped at 1,024 UTF-8 bytes.
- Offline ZIP hierarchy, JavaScript-created navigation, module imports and escaped CSS references remain open. Existing crawl and file-size limits still apply.
- Browser sources remain 0.40.0, and the signed network runtime is unchanged. Personal browser-session activation, Firefox live acceptance for this candidate, driver equivalence, publisher signing, updater and clean-machine installation remain unverified.
- Full IDM feature and design parity remains unestablished. Test counts are not a parity percentage.

## Current-PC deployment

[UDM 0.62.0 installer](../installer-out/UDM-0.62.0-Browser-0.40.0-Setup-x64.exe) is built. 47 files were deployed with SHA-256 verification and original-file backups. Installer SHA-256: `9AB3D4B70C0119EAE5D857A53FCFB8F9F40E87D0B2FF2E7CAF6085F5DFFFE6F1`.

The running app/native bridge reports 0.62.0 and the correct D:/UDM/user-data catalog. All 23 records, queues and saved preferences are unchanged. Browser 0.40.0 sources are installed; the signed network runtime is unchanged. This desktop update does not change extension sources; no new extension reload is required. Personal-session activation remains unverified.
