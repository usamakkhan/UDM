# UDM 0.64.0 / browser 0.41.0

UDM now follows website session-cookie updates after manual Site Grabber browser sign-in. A website can replace or delete its cookie during exploration, metadata checks or downloads, and the next eligible request uses the updated value. Browser 0.41.0 and the signed network runtime are unchanged.

## Behavior

- Capture sign-in through **Site Grabber → Name and starting page → Advanced** and **Use this signed-in session** in the extension. Cookie updates thereafter are automatic.
- Read repeated Set-Cookie response fields independently, before following redirects or starting dependent requests. Preserve cookie scope, expiration, replacement order, security prefixes and partitioned identities.
- Use one synchronized cookie jar per request pool, including its parallel range/metadata requests and proxy routes. A redirect to another origin cannot read or change this session; returning to its original origin can use it again.
- Save updated job cookies with Windows account encryption before committing the in-memory update. File Info, file transfers, redownload, synchronization and offline ZIP workflows retain the session. Catalog write failures restore the prior encrypted value, and a newer browser capture wins over a late response.
- Store exploration cookies with its continuation checkpoint so increasing a page limit or resuming an interrupted scan keeps both visited pages and current authentication.

IDM documents manual authorization for its Site Grabber in its [starting-page guide](https://www.internetdownloadmanager.com/support/idm-grabber/starting.html). The independent UDM implementation follows the cookie processing rules in [RFC 6265](https://www.rfc-editor.org/rfc/rfc6265), with an additional exact-origin boundary for captured sessions. No claim is made about IDM's internal implementation.

## Verification

**1,779 fresh checks passed:** 1,648 native (50 new), 8 native protocol, 8 app/host, 40 Chrome/Edge media, 33 offline website and 42 session-rotation checks across Chrome, Edge and Firefox. The focused 50 checks are included in the native total, not counted twice. Unchanged browser-unit results from 0.63 are not counted as new acceptance.

The same actual Chrome handoff fails on the previous 0.63 engine with HTTP 401 after cookie rotation, then passes on 0.64. Recovery testing pauses after real bytes arrive, reopens the manager, requires the replacement cookie and checks the resumed range offset and exact output. Browser fixtures also require a second rotation and cookie deletion before saving an offline image.

[Evidence](evidence-0.64.0/summary.json) · [Development failures and fixes](evidence-0.64.0/DEVELOPMENT.md). Existing compiler warnings are retained in the build logs. The installer is built; current-PC deployment is recorded separately.

## Remaining limits

- Browser-session cookie handling applies to signed-in Site Grabber projects and downloads carrying that protected session. Ordinary captured static Cookie headers do not become managed cookie jars automatically.
- Cookie sharing is confined to one exploration/transfer request pool. Independently created jobs keep their own snapshots; a site that invalidates every concurrent client's token may still require renewed sign-in or sequential work.
- Exploration saves cookies at its normal checkpoints; a process crash before the next checkpoint can require renewed sign-in.
- Sessions stay bound to their captured scheme, host and port. Rendered-page traversal, localStorage tokens, arbitrary authorization schemes and browser-cache reuse remain open.
- Native visual/DPI/accessibility acceptance, public video-site compatibility, matched-route IDM speed comparisons, driver equivalence and distribution signing remain unverified or incomplete. Full IDM parity is not established.

See the [workflow inventory](idm-parity-0.64.0.md) for the unchanged full scope.

## Current-PC deployment

[UDM 0.64.0 installer](../installer-out/UDM-0.64.0-Browser-0.41.0-Setup-x64.exe) is built. 69 files were deployed with SHA-256 verification and original-file backups. Installer SHA-256: `A140C50C7015A3B8FB1DB94B21446A3F14E7DC82171C9AF314805EFEC05E124B`.

The running app/native bridge reports 0.64.0 and the correct D:/UDM/user-data catalog. All 23 records, queues and saved preferences are unchanged. Browser 0.41.0 sources and the signed network runtime are unchanged. Both unpacked Chromium folders retain their verified files; no extension reload is needed for this native update. Personal-profile visual acceptance remains unverified.
