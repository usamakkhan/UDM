# UDM 0.60.0 / browser 0.39.0

This update makes Site Grabber an interactive scan-and-download workspace, adding reusable templates, site trees and project transfer controls. Full IDM parity remains unestablished.

## New workflow

- Enable **Download matched files while exploring** to start files as they are found. Project downloads respect the global limit plus the project's own simultaneous-file limit; they do not enable or drain another queue. Progress is saved incrementally. Stop exploration retains results, pending pages and job identities.
- Use **Start checked** and **Stop checked** while exploration continues. Explicitly stopped files stay stopped when later automatic checkpoints revisit them. Existing queue-based **Add selected / Download later** remains available.
- Browse results under **All files**, **By folder** or **By referring page**. The page tree follows captured exploration ancestry. Check/uncheck visible or highlighted rows, inspect properties, edit a not-yet-added filename, open completed files/folders, and copy download/referrer addresses.
- The result list shows size, status, bytes received and transfer rate. Updates preserve highlight/focus and update stable rows in place. An always-on-top statistics dialog shows found/checked/completed/active/queued/paused/failed counts, pages, errors, bytes and aggregate rate.
- Save reusable templates, replace their settings, delete them, or apply them to new/existing projects. Templates contain destination/filter/concurrency choices and omit captured URLs, identities, credentials and continuation state. Applying one preserves the project name and starting page.
- Category renames/deletions update saved template destinations transactionally. Live category renames, selection changes, filename edits and automatic-download toggles survive later crawler updates. Failed catalog writes stop exploration and roll back staged download batches.
- Set independent metadata-lookup and project-download concurrency. Newly discovered file sizes remain visible before the first download request. Statistics avoid integer overflow and distinguish completed files from workers finishing cleanup.

The workflow follows IDM's documented [Grabber templates](https://www.internetdownloadmanager.com/support/idm-grabber/starting.html), [action dialog](https://www.internetdownloadmanager.com/support/idm-grabber/action.html) and [Grabber settings](https://www.internetdownloadmanager.com/support/idm-grabber/settings.html). The latter two are older-version references. Implementation and graphics are UDM's own.

## Verification

**2,112 checks passed:** 1,450 native, 614 browser, 8 native-protocol, 8 isolated app/host, and 16 real extension/native scenarios each in Chrome and Edge. The native suite adds 56 checks over 0.59.0. [Machine-readable evidence](evidence-0.60.0/summary.json), [native log](evidence-0.60.0/build-release-candidate.log), [workflow inventory](idm-parity-0.60.0.md).

Local HTTP fixtures verify actual file completion while a later page is still scanning, cancellation and durable continuation, stopped queues remaining stopped, per-project concurrency, exact downloaded content and edited filenames. Tests also cover live UI-model edits during scan checkpoints, category/template persistence rollback, failed download-batch rollback, and checkpoint failures stopping further requests.

The initial focused run found a completed file temporarily counted as active during worker cleanup. This was corrected before release qualification. The final full suite, protocol tests and browser integration were run on the release candidate.

## Remaining limits

- Native visual, keyboard/click, screen-reader and mixed-monitor acceptance remains unavailable: the computer-control kernel failed before initialization with a missing assets path.
- Chrome/Edge acceptance uses isolated profiles and localhost generated media, not public-site video or same-server internet speed comparison.
- Browser 0.39.0 and the signed network runtime are unchanged. No new Firefox, driver-equivalence, publisher-signing, updater or clean-machine qualification.
- Grabber does not execute JavaScript or inherit a manually authenticated browser session. Offline ZIP remains static mirroring; folder hierarchy in those archives is still incomplete.
- The independently implemented native interface is not a proven pixel-identical IDM replica. IDM documentation used here includes an explicitly older-version action/settings guide.
- HTTP/HTTPS scanning retains the 2,000-file and 10,000-address caps, 2 MiB page limit and request deadlines. Lookup and download concurrency are each bounded at 1-16; project downloads also obey the global simultaneous-download limit.
- URL filename prefiltering may miss a server-renamed file whose original link does not match. Rendered/authenticated crawling, link-text descriptions and browser-cache reuse remain outside this update.
- The bundled Public Suffix List is a fixed snapshot; automatic updates are not implemented.

## Current-PC deployment

[UDM 0.60.0 installer](../installer-out/UDM-0.60.0-Browser-0.39.0-Setup-x64.exe) is built. 45 files were deployed with SHA-256 verification and original-file backups. Installer SHA-256: `8713EEC898B96E1BA90243C9520AFD7FAE979975DF4313090B80D545D7645B2C`.

The running app/native bridge reports 0.60.0 and the correct D:/UDM/user-data catalog. All 23 records, queues and saved preferences are unchanged. Browser 0.39.0 sources are installed; the signed network runtime is unchanged. This desktop update does not change extension sources; no new extension reload is required. Personal-session activation remains unverified.
