# UDM 0.59.0 / browser 0.39.0

This update expands Site Grabber from extension-only collection to separate exploration and file filters, retained scan results, continuation and file metadata. It does not establish full IDM parity.

## New workflow

- Five pages separate project setup, destination, explorer rules, file rules and review/download. Advanced controls expose domain/path patterns and size limits. Review includes destination, address, added state, size, content type and referring page; collected-file Properties lets you inspect and copy its addresses.
- Include/exclude comma-separated filename patterns, `*` wildcards and `<start page>`. Exclusions take priority. Page rules and file rules are independent, including starting-host scope, external page depth, main-domain exploration and parent-directory restrictions.
- Whole-domain mode uses ICANN and private rules from the Public Suffix List, with international-domain normalization, so sibling hosted tenants are not mistaken for one site. The bundled snapshot and MPL-2.0 license ship with the app.
- Minimum and maximum file sizes are inclusive. Unknown sizes are retained unless a size limit is enabled, in which case the scan explains the exclusion. Duplicate hiding keeps the first discovered file with the same name and size; original website subfolders disable it.
- Up to four metadata lookups overlap. HEAD falls back to a single-byte Range request where needed; the crawler does not read ordinary file payloads. Server-supplied filenames survive category routing and downloads. Page/referrer context is preserved for refresh-link workflows and scoped for cross-origin requests.
- Stop preserves files already found and unfinished work. Resume continues it across saved projects and app restarts. Raising the page limit permits further exploration; changing filters starts a fresh scan. Malformed saved state is rejected before requests. Hard file/address caps are reported with a request to narrow the filters.
- Older projects retain their backend behavior, destination choices and selected-link addition. Selection controls are disabled during exploration so edits cannot be silently replaced when the worker finishes.

The independently implemented filter behavior follows IDM's documented [Explorer filters](https://www.internetdownloadmanager.com/support/idm-grabber/wheretosearch.html) and [file filters](https://www.internetdownloadmanager.com/support/idm-grabber/filters.html). The review workflow is informed by its [action dialog guide](https://www.internetdownloadmanager.com/support/idm-grabber/action.html), which is explicitly an older-version reference. No proprietary code or graphics were copied.

## Verification

**2,056 checks passed:** 1,394 native, 614 browser, 8 native-protocol, 8 isolated desktop/host, and 16 real extension/native scenarios each in Chrome and Edge. The native suite adds 69 filter/transport checks over 0.58.0. [Machine-readable evidence](evidence-0.59.0/summary.json), [native log](evidence-0.59.0/build-release-candidate.log), [workflow inventory](idm-parity-0.59.0.md).

The new HTTP fixture records incoming requests and proves file exclusions and exploration exclusions are enforced before the relevant requests, including redirects; verifies metadata overlap and HEAD/Range fallback; checks actual server-filename downloads and referrer handling; exercises duplicate order, unknown lengths, scopes, stop/continue, changed filters, malformed resume state and the 2,000-file boundary. Model checks cover wildcard precedence, IDNA, ICANN/private suffixes, traversal and numeric bounds. Existing transfer, media, proxy, recovery, queues and destination tests also run.

The first focused pass passed 116 checks. Subsequent source review found a moved-from result count used for the capacity reason, and added explicit hard-limit checks before final qualification. The first browser-unit run hit a sandbox child-process EPERM; the authorized rerun passed. The first Chrome run also stopped before its scenarios because the disposable fixture lacked its FFmpeg copy; after supplying the existing dependency, a fresh run passed all 16 scenarios. These setup failures are not represented as live application failures.

## Remaining limits

- Native visual, keyboard/click, screen-reader and mixed-monitor acceptance unavailable: the computer-control kernel failed before initialization with a missing assets path.
- Chrome/Edge acceptance uses isolated profiles and localhost generated media, not public-site video or same-server internet speed comparison.
- Browser 0.39.0 and signed network runtime unchanged. No new Firefox, driver equivalence, publisher-signing, updater or clean-machine qualification.
- Advanced Grabber filters apply to collected files. Offline ZIP retains its prior static mirroring scope and bounds.
- Grabber has no JavaScript execution/manual browser login, custom saved templates, site-structure tree or immediate matched-file downloading while exploring. Native appearance is independently implemented, not a proven pixel-identical IDM replica.
- Scanning is HTTP/HTTPS only, with 2,000-file and 10,000-address safety caps, 2 MiB pages, four simultaneous metadata requests and request deadlines. On hard capacity limits, narrow filters and explore again. Increase MaxPages to continue a page-limited scan.
- Filename prefiltering uses the linked URL before metadata for ordinary file links; a server renaming an otherwise excluded URL may not be discovered. Dynamic page-like download endpoints are inspected.
- Domain boundaries use the bundled Public Suffix List snapshot; automatic list updates are not implemented.

## Current-PC deployment

[UDM 0.59.0 installer](../installer-out/UDM-0.59.0-Browser-0.39.0-Setup-x64.exe) is built. 48 files were deployed with SHA-256 verification and original-file backups. Installer SHA-256: `537E18171711F51473F9ACC9CBA8994A371B45B78091710FDDE038D8141780D8`.

The running app/native bridge reports 0.59.0 and the correct D:/UDM/user-data catalog. All 23 records, queues and saved preferences are unchanged. Browser 0.39.0 sources are installed; the signed network runtime is unchanged. This desktop update does not change extension sources; no new extension reload is required. Personal-session activation remains unverified.
