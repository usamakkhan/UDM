# UDM 0.61.0 / browser 0.39.0

This update adds normal-file offline website saving to Site Grabber: downloaded pages and stylesheets link to the files UDM actually saved. It follows IDM's documented [Save to and local-link workflow](https://www.internetdownloadmanager.com/support/idm-grabber/saveto.html). Full IDM parity remains unestablished.

## Use it

Open **Site Grabber**, choose **Complete website**, and select a destination folder on **Save to**. Enable **Use original website subfolders** to retain the URL hierarchy. **Convert links to local files for offline browsing** is enabled by the template and can also be used with other file-collection templates.

Explore the site and start the checked files. Once exploration has ended and all added files have finished or stopped, UDM converts completed HTML/CSS files. **Convert links** on the review page retries conversion manually. The review status reports completion or the first error. Additional completed resources or moved/renamed destinations trigger a new conversion from the retained original page.

## Behavior and recovery

- Actual numbered filenames, different destination folders, redirected resources, CSS imports, images, scripts and fragment navigation are mapped to local files. Unavailable resources remain absolute website URLs. Cross-volume destinations use file URLs.
- HTML and CSS endpoints receive browser-readable filename extensions unless a user supplied a saved name. Normal file downloads and their source addresses remain in the download history.
- Original pages are retained with hashes. Repeat conversion starts from those original bytes, preserving UTF-8 byte-order marks and avoiding cumulative rewriting. Pages changed outside UDM are preserved.
- Converted files and their recorded size/hash publish through a journaled replacement. Catalog-write failures roll the page back; interrupted replacements recover matching content and metadata on restart. Inconsistent recovery records preserve both versions for inspection.
- Conversion waits for exploration and pending transfers, supports Stop/retry, and can run while the download queue is disabled. It does not change completed file status into a failed transfer. Unsupported document encodings report a conversion error without altering their bytes.
- Queue synchronization compares converted pages with their original server length and validators, avoiding unnecessary redownloads. A synchronized replacement retains project association and receives its own source cache. Previous/recycled versions are excluded from conversion.

## Verification

**2,198 checks passed:** 1,503 native, 614 browser regression, 8 native-protocol, 8 isolated app/host, 16 extension/native scenarios in each of Chrome and Edge, and 33 offline-website checks. This adds 53 native checks over 0.60.0. [Evidence](evidence-0.61.0/summary.json) · [Workflow inventory](idm-parity-0.61.0.md).

The offline acceptance fixture downloads an HTML/CSS/JS/image site through the native Grabber, with an existing filename collision and a redirected image. It shuts down the server, then opens the actual saved files in Chrome and Edge. Both browsers verify styling, imported CSS, script execution, images, forward/back navigation, preserved anchors, numbered-file references and zero HTTP requests. Downloaded hashes and the existing file are checked separately. This is a development fixture, not a public-site compatibility claim.

Initial tests caught a queue-schedule cancellation bug and legacy continuation signature mismatch. Recovery tests also exposed null-field normalization across catalog reloads. These were corrected before final qualification. An existing rollback check was made atomic against legitimate worker cleanup. A browser navigation comparison was corrected to compare decoded file paths rather than equivalent URL spellings. A sandbox subprocess launch failure was rerun in the authorized test environment. The first Edge extension run passed 14 scenarios, then timed out waiting for an iframe media capture. Its failure record is retained; the fresh-profile rerun passed all 16 scenarios. The intermittent timeout's cause is not established, and broad video-capture reliability remains an open qualification item.

## Remaining limits

- Native visual, keyboard/click, screen-reader and mixed-monitor acceptance remains unverified: computer control failed before initialization with a missing kernel-assets path.
- Chrome/Edge acceptance uses isolated profiles, generated localhost media and a small local website. It does not establish public video-site coverage or same-server internet speed parity.
- Browser 0.39.0 and the signed network runtime are unchanged. No new Firefox, driver-equivalence, publisher-signing, updater or clean-machine qualification.
- Normal Grabber link conversion supports ASCII-compatible HTML/CSS up to 2 MiB per document. Unsupported encodings and externally edited documents remain unchanged and report an error.
- The crawler does not execute JavaScript, inherit a manually authenticated browser session or reuse its cache. Script-created links, JavaScript module imports, escaped CSS URLs and link-text descriptions remain outside this update.
- Complete website preserves scripts, forms and original content-security policies. A saved site can still depend on a server or on browser restrictions; it is not a guaranteed functional mirror of every website. Form destinations remain web addresses and are not submitted by the crawler.
- Offline website (ZIP) remains a separate static archive mode with scripts/forms disabled; archive folder hierarchy remains incomplete.
- HTTP/HTTPS scanning retains the 2,000-file and 10,000-address caps and request deadlines. Metadata and download concurrency remain bounded.
- The interface is independently implemented and not a proven pixel-identical IDM replica. Full IDM parity remains unestablished.

## Current-PC deployment

[UDM 0.61.0 installer](../installer-out/UDM-0.61.0-Browser-0.39.0-Setup-x64.exe) is built. 57 files were deployed with SHA-256 verification and original-file backups. Installer SHA-256: `D9C0C2A17890EF9C02A6A90B9B012F9FC0C233A70B5C84F2505F1297EFD4F45D`.

The running app/native bridge reports 0.61.0 and the correct D:/UDM/user-data catalog. All 23 records, queues and saved preferences are unchanged. Browser 0.39.0 sources are installed; the signed network runtime is unchanged. This desktop update does not change extension sources; no new extension reload is required. Personal-session activation remains unverified.
