# Release history

## Unreleased

- [Detailed changes from 0.84.0 RC1 through the 8 October source push](docs/changelog-rc1-to-20261008-push.md). The latest browser package and native GUI candidates have not been installed or published.

- Honor Windows static-proxy bypasses and configured PAC URLs for FTP; unresolved WPAD and unsupported static proxy routes remain fail-closed.
- Expand captured browser POST bodies to 4 MiB with an 8 MiB native request frame; preserve exact-byte and over-limit rejection checks.
- Reject ranged HTTP responses that omit the validator established by the download probe.

- Wait for a saved completion before executing CLI /q exit or /h dial-up hang-up; a failed catalog save retains both pending actions for retry.

- License UDM's original code and documentation under Apache 2.0; retain third-party notices and include the project license/notices in future packages.
- Version progress-dialog reference metadata and sanitized validation/release records, retaining the full-suite failure and its passing focused rerun.

- Make individual Pause transactional: a failed catalog save preserves the job, capture prompt, scheduler state, and running transfer. Cancel the worker only after the pause decision is saved.
- Avoid catalog writes when stopping an idle completed download while retaining pending-synchronization cancellation and scanner-wait handling.

- Keep the completion dialog drag area icon-only, retain its Unicode filename as the control name, and support icon rendering in window previews.
- Add a completion-dialog test runner and protect its optional warning observer flag across threads.

## Native 0.84.0 RC1 / browser 0.62.8

- Consolidate queue retry, synchronization, recovery, backup and restore behavior with focused backend checks.
- Add `/a` queue-only downloads and `/s` queue start, including forwarding to an existing application instance.
- Extend browser capture recovery, multipart handling, file recognition and HLS/DASH media selection.
- Improve native menus, properties, progress controls, keyboard behavior and accessibility.
- Add installer migration, ownership and transaction safeguards, plus pinned HTTP/2 transport dependencies.
- See [candidate acceptance and delivery status](docs/current-staged-candidate.md) for the exact tested, packaged and installed revisions. This entry records source changes; it does not announce a qualified release.

## Native 0.78.0 / browser 0.53.1 — video preparation deadlines

- Stop stalled cross-site video preparation before the panel timeout and prevent late native submission.
- Guard direct-media handoff after asynchronous settings/credential work; cancel HLS/DASH preparation when expired.
- Keep unknown native acceptance distinct from a request never sent, without automatic retry.
- [434 targeted and 35 actual Edge checks, decoded outputs and explicit limits](docs/parity-browser-0.53.1.md).

## Native 0.78.0 / browser 0.53.0 — Firefox handoff reliability

- Keep Firefox's original response running until UDM has durably prepared the request.
- Preserve refused/unavailable downloads without pause/resume or a second request.
- Recover prepared/committed ownership after lost acknowledgements and actual background reloads.
- Preserve browser completion and user pause/cancel decisions during preparation.
- [386 targeted and 47 live checks, baseline failures and remaining limits](docs/parity-browser-0.53.0.md).
- Native 0.78 and earlier browser changes: [release](docs/parity-0.78.0.md), [manual Firefox recovery](docs/parity-browser-0.52.2.md), [current gap register](docs/idm-research-current-status-2026-09-29.md).

## 0.74.0 / browser 0.47.1 — repeated-cancel exclusions

- Offer site or literal-address exclusions after two consecutive cancelled automatic captures.
- Add suppression/re-enable, File Types Add/Delete, replay protection and transactional settings updates.
- Preserve literal asterisks, long exact URLs and IPv6 in both extension bundles.
- Preserve multipart.js and ordered dependencies when regenerating Firefox.
- Correct network-helper signature wording.
- [2,821 checks and qualification limits](docs/parity-0.74.0.md).

## 0.73.0 / browser 0.47.0 — queue startup and export scopes

- Persist per-queue Start download on UDM startup independently of the current Stop state.
- Prepare opted-in queues once at launch, with rollback if saving fails.
- Add all, selected and named/all queue export scopes with retained per-file checkboxes.
- Reject stale/duplicate export records and empty export sets before writing.
- Reject overlong instance tags instead of silently connecting to the main app.
- [1,938 checks and qualification limits](docs/parity-0.73.0.md).

## 0.72.0 / browser 0.47.0 — native toolbar customization

- Replace individual toolbar buttons with a native toolbar and queue dropdowns.
- Add Available/Current command editing, repeated separators, Move Up/Down and Reset.
- Migrate existing preferences and save customized command layouts.
- Support bounded .tbi skin strips, normal/hot/disabled states and high-DPI image variants.
- Preserve UDM artwork and fall back safely if a saved skin is missing.
- [1,917 passing checks, development corrections and remaining GUI limits](docs/parity-0.72.0.md).

## 0.71.0 / browser 0.47.0 — remote ZIP preview

- Add Preview to Download File Info for ordinary ZIP links before download.
- Read bounded HTTP/HTTPS/FTP ZIP and ZIP64 directories using existing login, browser sessions and proxy routes.
- Handle small no-range responses, cancellation, timeouts and changed archive responses.
- Show names, sizes and encryption flags in a virtual native list; retain local preview.
- Correct toolbar/dial-up scope in the comparison inventory.
- [1,887 passing checks, initial failures and limits](docs/parity-0.71.0.md).

## 0.70.0 / browser 0.47.0 — offline website folders

- Enable original subfolders for offline ZIP projects with an explicit destination folder.
- Preserve nested pages and stylesheets, rewrite relative links, retain a root entry page, and separate colliding/query/external resource names.
- Reuse redirected resources, validate archive names and content, and retain verified cache reuse on pause/resume.
- Correct the About dialog's stale version label.
- [1,848 passing checks and remaining limitations](docs/parity-0.70.0.md).

## Browser 0.47.0 / native 0.69.0 — multipart text-form downloads

- Preserve browser-generated multipart text fields, original order, repeated names, encoding and submitter overrides.
- Match the ephemeral submission snapshot to the observed request and retain verified same-origin POST redirects.
- Keep unsupported file/Blob or oversized bodies in the browser.
- [846 checks and actual Edge byte comparisons](docs/parity-browser-0.47.0.md).

## Browser 0.46.0 / native 0.69.0 — responsive video formats

- Bind panel format reads to the browser-issued document identity, avoiding redundant tab lookups and document-idle waits.
- Preserve strict current-tab checks for download actions and legacy browser paths.
- Complete a public Edge 360p video pause, link refresh, resume and full decode on one history record.
- [807 current checks, measured results and limits](docs/parity-browser-0.46.0.md).

## 0.69.0 / browser 0.45.0 — direct video recovery

- Refresh an unfinished direct video/audio pair from the browser panel on its existing record.
- Validate partial-track HTTP identity and completed-track integrity before reuse; retain saved data on rejection or cancellation.
- Open the original page automatically when address refresh begins.
- Check middle and final playback bytes as well as the first byte before accepting a native player pair.
- [1,785 passing checks, public-test failures and remaining limitations](docs/parity-0.69.0.md).

## v0.68.0 — combined 0.58 through 0.68 update (28 September 2026)

This published release includes the previously untagged 0.58.0 through 0.68.0 milestones. The v0.68.0 tag is the complete source snapshot; the entries below preserve the recorded detail for every included version.

- **0.58–0.64:** expanded Site Grabber destinations, filters, link handling, website workspaces, browser sign-in, login persistence, and session updates.
- **0.65–0.67:** signed-in browser downloads, Edge proxy handoff, encrypted proxy/session handling, and ordered proxy alternatives.
- **0.68:** refresh an interrupted recorded HLS or static DASH session from a browser capture while preserving the original download, history, and verified cache.

See [0.58](docs/parity-0.58.0.md), [0.59](docs/parity-0.59.0.md), [0.60](docs/parity-0.60.0.md), [0.61](docs/parity-0.61.0.md), [0.62](docs/parity-0.62.0.md), [0.63](docs/parity-0.63.0.md), [0.64](docs/parity-0.64.0.md), [0.65](docs/parity-0.65.0.md), [0.66](docs/parity-0.66.0.md), [0.67](docs/parity-0.67.0.md), and [0.68](docs/parity-0.68.0.md) for behavior and stated limits.

## 0.68.0 / browser 0.45.0 — recorded-video link recovery

- Refresh interrupted HLS/static DASH sessions from the browser panel without creating another download.
- Preserve filename, history and cache; review fresh credentials before applying them.
- Verify retained segment bytes against the fresh source before fetching missing segments; preserve cache on failure, pause or restart.
- Reject changed selections, foreign-origin credentials and incompatible segment layouts.
- [1,739 passing checks and remaining limitations](docs/parity-0.68.0.md).

## 0.67.0 / browser 0.45.0 — extended Edge proxy handoff

- Preserve encrypted HTTPS proxies and Edge SOCKS4 local DNS through browser capture and native download.
- Keep proxy credentials separated by encryption protocol; preserve existing native compatibility.
- Preserve ordered PAC alternatives for connection failures without replaying POST or bypassing TLS/authentication errors.
- [2,586 passing checks and remaining limitations](docs/parity-0.67.0.md).

## 0.66.0 / browser 0.44.0 — Edge proxy handoff

- Captured fixed/direct Edge routes, per-protocol endpoints, bypass rules, and explicit-link proxy selection.
- Corrected explicit HTTP/SOCKS proxy routing for loopback downloads with a bounded native Schannel transport.
- Added native compatibility checks; retained browser ownership for unresolved PAC routes.
- [2,509 checks and remaining limitations](docs/parity-0.66.0.md).

## 0.65.0 / browser 0.42.0 — Signed-in browser downloads

- Managed, exact-origin cookie sessions for ordinary browser file captures with consent, store/partition checks, encrypted updates, duplicate handling and link refresh.
- Removed default-profile cookie guessing and fixed UTF-8 cookie-header budget accounting.
- [Acceptance and remaining limitations](docs/parity-0.65.0.md).

## 0.64.0 / browser integration 0.41.0 — Website session updates

- Apply website Set-Cookie responses to signed-in Site Grabber sessions, including redirects, expiry, deletion, domain/path scopes and separate partitioned identities.
- Share updates across a transfer's parallel requests and protect saved sessions with Windows account encryption.
- Retain updated cookies in exploration checkpoints, File Info lookups, queued file transfers, redownload, synchronization and offline ZIP requests. Reject late updates after browser recapture; roll back failed catalog saves.
- Reproduce the previous release's HTTP 401 failure with an actual Chrome sign-in and a server that rotates its session. Verify the replacement-cookie workflow in Chrome, Edge and Firefox.
- Browser source and signed network runtime remain unchanged. Public-site coverage, rendered exploration, cross-job session sharing and full IDM parity remain open.

## 0.63.0 / browser integration 0.41.0 — Site Grabber browser sign-in

- Add manual browser sign-in, a login-page field, logout exclusions and an explicit project selector in the extension popup.
- Save scoped browser cookie snapshots with Windows protection; carry them into exploration, metadata, queued files, redownload, synchronization and offline ZIP requests.
- Respect tab stores/containers, matching partitions, cookie paths, Secure and expiration; recalculate scope through redirects.
- Handle stale wizard saves, corrupt sessions, expiring tickets, replay protection and catalog/template credential boundaries.
- 2,381 checks pass, including real Chrome, Edge and Firefox sign-in/popup/native workflows. Rendered crawling, cookie rotation and full IDM parity remain open.

## 0.62.0 / browser integration 0.40.0 — Site Grabber login and descriptions

- Add project-specific HTTP Basic/Digest login with Windows-protected persistence, exact-origin request scoping, editable credential recovery and save rollback.
- Carry login details into exploration, parallel metadata checks, queued downloads and offline ZIP archives.
- Extract static link text and image alt text into collected-file properties, a Description column and new download records; retain bounded Unicode descriptions across scan continuation.
- Preserve edited metadata on existing downloads; templates exclude project credentials. Browser integration remains 0.40.0.
- 2,276 checks pass, including 45 new native checks, 40 real Chrome/Edge integration scenarios and 33 offline website checks. Native visual acceptance and full IDM parity remain open.

## Browser integration 0.40.0 / native 0.61.0 — Capture lifecycle

- Fix a reproducible late-navigation cleanup race that erased newly detected playlists.
- Retire previous document/frame captures, authenticated request context and SABR sessions while preserving fresh captures; serialize menu-offer storage with cleanup.
- Persist bounded document identity across service-worker restarts and reject delayed work after tab closure.
- Add webNavigation permission; reload the extension and refresh video tabs to activate.
- 687 fresh checks pass, including 25 new lifecycle checks and 20 real extension/native scenarios in each of Chrome and Edge. Native engine and signed network runtime unchanged; full IDM parity remains unestablished.

## 0.61.0 / browser integration 0.39.0 — Offline website links

- Add a Complete website preset and conversion of HTML/CSS references to actual downloaded destinations, including original subfolders, numbered filenames, redirects and CSS imports.
- Retain original pages for repeat conversion; preserve external edits and unsupported encodings. Recover interrupted page/catalog publication with verified journals and rollback.
- Support conversion while queues are disabled, Stop/retry, and synchronization using original server lengths. Preserve project association on updated files.
- 2,198 checks pass, including 53 new native checks and 33 real offline-site checks in Chrome/Edge. Native visual acceptance, public-site coverage and full IDM parity remain open.

## 0.60.0 / browser integration 0.39.0 — Live Site Grabber workspace

- Download matching files during exploration with bounded concurrency and incremental persistence. Start/stop checked downloads without enabling unrelated queues.
- Add reusable templates, folder/referring-page trees, editable collected filenames, file/folder actions, address copying and live statistics.
- Preserve edits across scan checkpoints, transactionally remap category/template destinations, and stop on persistence failures.
- 2,112 checks pass, including 56 new native checks and 32 Chrome/Edge integration scenarios. Native visual acceptance and full IDM parity remain open.

## 0.59.0 / browser integration 0.39.0 — Advanced Site Grabber

- Add separate Explorer and File Filters pages with include/exclude patterns, independent site/path rules, depth, main-domain scope, size bounds and deterministic duplicate hiding.
- Add concurrent metadata lookup, server filenames, result properties, referring-page context, Stop with retained results and saved-scan continuation.
- Use an included MPL-2.0 Public Suffix List snapshot for domain boundaries; preserve legacy saved projects and category/destination behavior.
- 2,056 checks pass, including 69 new native checks and 32 Chrome/Edge integration scenarios. Native visual acceptance, broader video/driver equivalence and full IDM parity remain open.

## 0.58.0 / browser integration 0.39.0 — Grabber destinations and reliable batches

- Add category-based, selected-category and explicit-folder destinations, with optional original URL subfolders for collected files.
- Add a dedicated Save to step and per-file destination/added-state previews; retain old project behavior and category choices across category renames/deletion.
- Save each selected batch and project atomically, with rollback on invalid paths or storage failure; reject traversal and redirected subfolders and preserve pause/restart/resume.
- 1,987 checks pass, including real HTTP folder downloads and 32 Chrome/Edge extension/native scenarios. Native visual acceptance and full IDM parity remain open.

## v0.57.0 — combined 0.41 through 0.57 update (28 September 2026)

This published release includes all previously untagged milestones from 0.41.0 through 0.57.0. The v0.57.0 tag is the complete source snapshot; the linked notes retain the recorded details for each included milestone.

- **0.41–0.43:** Firefox proxy-route handoff, indexed DASH resource binding, and selected YouTube audio-only streaming.
- **0.50–0.53:** media-session recovery, download and completion dialogs, configuration/queue controls, plus PAC and transport configuration.
- **0.54–0.56:** browser customization, quota workflows, and safer duplicate/overwrite behavior.
- **0.57:** exposed clear live HLS recording with Stop and save, pause/recovery, and native parallel capture.

See [0.41](docs/parity-0.41.0.md), [0.42](docs/parity-0.42.0.md), [0.43](docs/parity-0.43.0.md), [0.50](docs/parity-0.50.0.md), [0.51](docs/parity-0.51.0.md), [0.52](docs/parity-0.52.0.md), [0.53](docs/parity-0.53.0.md), [0.54](docs/parity-0.54.0.md), [0.55](docs/parity-0.55.0.md), [0.56](docs/parity-0.56.0.md), and [0.57](docs/parity-0.57.0.md) for detailed behavior and limits.

## 0.57.0 / browser integration 0.39.0 — live HLS recording

- Capture exposed clear live HLS through native parallel workers and playlist refreshes; Stop and save the captured portion or pause separately.
- Persist encrypted segment receipts, verify resumed media, recover offline after expiration/publication conflicts, and clean only owned cache files after successful publication.
- Respect retry budgets and Retry-After; cancel stalled HTTP promptly; preserve separate-track timing, discontinuities and changed initialization sections.
- 1,461 checks passed, including generated TS/fMP4 media and 32 actual Chrome/Edge extension/native scenarios. Native visual acceptance, broad public-site support and full parity remain open.

## 0.56.0 / browser integration 0.38.0 — duplicate overwrite workflows

- Remember numbered-copy, overwrite and Existing choices, with full dialog descriptions and all policies in Options > Downloads.
- Replace completed HTTP/FTP files after verifying new bytes; retain prior versions with accurate metadata and invalidate obsolete scan results for edited copies.
- Restart unfinished duplicates at the original filename with a fresh parts generation, preserved preferences and correct browser/CLI confirmation routing.
- Commit history and preferences together; retain old parts on failure and recover cleanup after a crash without deleting unrelated or changed files.
- 1,185 checks passed: 1,135 native, 16 protocol/app-host, 30 Chrome/Edge and 4 real active/passive FTP scenarios. Eight selected resource bounds match. Native visual acceptance and full parity remain open.

## 0.55.0 / browser integration 0.38.0 — quota and download workflows

- Add the download-limit warning preference, native warning dialog, quota countdown and waiting status.
- Handle simultaneous quota reservations, large chunks, cancellation, rollover, restart and malformed/future saved timestamps.
- Open Download complete for existing completed links and resume unfinished duplicates directly; keep automatic completion actions separate from viewing history.
- Preserve new-download confirmation, saved segments, paused records on persistence failure and existing double-click behavior.
- 1,139 checks passed, including 1,095 native checks and real isolated Chrome/Edge downloads. Six selected reference rectangles match. Native visual acceptance and full parity remain open.

## 0.54.0 / browser integration 0.38.0 — browser customization update

- Add separate native Keys, Browser Menus and Video Panels dialogs with functional preferences.
- Support Insert/Delete combinations, request-bound held-key capture, Ctrl-force/Alt-bypass defaults and forced web-resource filtering.
- Apply browser-family context-menu switches and video type/minimum-size/site-exception rules; reject stale disabled offers.
- Add optional verified direct-player capture with opt-in, protected/ad/fragment checks and duplicate prevention.
- Fix Edge menu-rebuild and held-key/download-popup races found in real browser testing.
- 1,720 behavioral checks passed, including real isolated Chrome/Edge downloads and 1,040 native checks. 45 static control rectangles match the reference. Native visual acceptance, public-site coverage, full parity and speed equivalence remain unverified.

## 0.53.0 / browser integration 0.37.0 — transport configuration update

- Add asynchronous custom PAC scripts, per-protocol configuration, redirect reevaluation and ordered proxy alternatives without POST replay.
- Route passive FTP control/data through PAC-selected HTTP CONNECT and SOCKS; retain precise connection failure messages.
- Add a working TLS 1.3 toggle and an opt-in modification-date override. Preserve certificate, ETag, byte-range, length and refreshed-link identity checks.
- Add reference-position transport controls and clear stale PAC settings from captured browser routes.
- Verified: 1,024 native checks, 24 new transport scenarios, 127 existing live transport regressions, 8 native-host checks, 6 isolated app/host checks and 112 static control bounds. Visual acceptance, full parity and speed equivalence remain unestablished.

## 0.52.0 / browser integration 0.37.0 — configuration and queue update

- Rebuild configuration around nine tabs with font-based dialog units, browser lists, category settings, connection exceptions, downloads, proxy routes, logins, dial-up/VPN and sounds.
- Add browser executable enable/disable preferences; the native host derives its actual parent identity. Restart reconnects only UDM native hosts from the active install.
- Add queue selection, optional queue creation/start and persistent prompt preferences for Download Later and batch imports. Queue-only browser capture applies to files and HLS/DASH media.
- Make category extension edits affect routing; preserve folder-memory and type settings across rename/delete and roll back failed persistence.
- Replace the duplicate dropdown with radio choices and compact Add URL with authorization controls. Existing replacement safeguards and HTTPS-only saved logins remain.
- Merge Options drafts without overwriting unrelated live settings and refresh controls after nested editors. Restore global SOCKS selection through Advanced and import Windows proxy routing.
- Verification: 1,008 native checks, 8 host protocol checks, 6 isolated app/host checks and 107 selected resource-bound comparisons. Native visual/click acceptance remains open.

## 0.51.0 / browser integration 0.37.0 — download dialog update

- Rebuild File Info, File Properties, Progress, Completion and progress customization with Windows dialog units and Tahoma 8-point measurements. Use native Windows buttons in the light theme.
- Add category creation and recent destination folders to File Info; keep authentication, queue and preview options under More.
- Match completion button order and add the address field. Keep saved-file drag, Open with, folder and suppression actions connected.
- Apply speed-limit edits immediately; preserve remembered limits and reset temporary overrides on resume. Add persistent Hide tab controls.
- Add separate disconnect, exit, power and force controls on the completion tab. Combined actions run once after a cancellable countdown, with force off by default and explicit confirmation when selected.
- See [verification and remaining gaps](docs/parity-0.51.0.md). Native visual acceptance and full parity remain unverified.

## 0.50.0 / browser integration 0.37.0 — media recovery update

- Retain completed captured-stream segments across pause, failure and process restart. Verify hashes and exact content/track identity before reuse, including after changing the connection count.
- Add **Refresh media session** for paused captured YouTube streaming jobs: open the original video, recapture the same selection, then save the matching session or save and resume on the existing record.
- Reuse verified complete tracks when retrying assembly, even after the browser session expires. Preserve input tracks on output hash failure or destination conflict.
- Show retained/reused bytes, media phases, **Retry assembly**, and an ETA that excludes previously retained media. Browser panels acknowledge session recovery with a review message.
- See [test evidence and remaining limitations](docs/parity-0.50.0.md). This update does not establish current YouTube compatibility or complete IDM parity.

## v0.40.0 — combined 0.37 through 0.40 update (27 September 2026)

This published release includes the previously untagged 0.37.0, 0.38.0, and 0.39.0 milestones as well as the final 0.40.0 changes. The tag provides one honest source snapshot and file comparison from v0.36.0; the linked notes preserve the recorded detail for each intervening milestone.

- **0.40.0:** separate HTTP/HTTPS/FTP proxy routes, passive FTP through HTTP CONNECT, and Microsoft Defender/ClamAV scanner presets. See [0.40 verification and limits](docs/parity-0.40.0.md).
- **0.39.0:** durable browser-handoff ownership receipts and recovery after an extension worker restart. See [0.39 verification and limits](docs/parity-0.39.0.md).
- **0.38.0:** safer browser form-download capture up to 1 MiB, plus Firefox handoff and saved-history reliability fixes. See [0.38 verification and activation](docs/parity-0.38.0.md).
- **0.37.0:** audio-only M4A output, explicit recorded-media audio language selection, and optional WebVTT subtitles in supported MP4 downloads. See [0.37 verification and limits](docs/parity-0.37.0.md).

## Included milestone: 0.39.0 / browser integration 0.29.0

Adds persistent browser-handoff ownership receipts and recovery after the extension worker stops. Accepted downloads continue in UDM; unsubmitted downloads resume in the browser. A durable release receipt prevents a delayed Add from creating a second download. Ambiguous ownership remains for review, and recovery never replays form bodies. Adds the popup recovery button. Activated locally in UDM, Chrome and Edge; Firefox was verified in an isolated profile. Passed 761 native checks, 366 browser/protocol/preparation checks and 61 live browser checks. The setup package was rebuilt. See [verification and limits](docs/parity-0.39.0.md).

## Included milestone: browser integration 0.28.0

Adds recorded clear DASH SegmentBase/SIDX support: bounded index parsing, exact range validation, parallel native media downloads, selected audio language and audio-only M4A. Compatible with native 0.38.0. Verified with 345 automated checks and 29 isolated Chrome, Edge and Firefox checks. See [implementation and limits](docs/browser-parity-0.28.0.md).

## Included milestone: 0.38.0

Supports browser form downloads up to 1 MiB with desktop negotiation and bounded memory. Fixes incomplete multipart capture, redirected-body fallback and unreloadable history writes. Extension 0.27.1 fixes Firefox request-event ordering and pause states, validates captured upload lengths, and preserves browser downloads when complete request data is unavailable. Native 0.38.0 and Chrome/Edge integration are activated. Removed the unrelated mouse utility and restored UDM documentation. See [verification and activation status](docs/parity-0.38.0.md).

## Included milestone: 0.37.0

Recorded HLS/DASH now supports audio-only M4A output, explicit audio language selection, and optional WebVTT subtitles embedded in MP4. The simple video handoff stays one click. Native, Chrome, Edge and Firefox checks are recorded in [the verification report](docs/parity-0.37.0.md).

Each version is represented by a source snapshot tag and its original portable bundle on the [GitHub Releases page](https://github.com/usamakkhan/UDM/releases). Select **Compare** to see the exact file and code changes introduced by that version.

| Version | Historical release date | Changes from previous version |
| --- | --- | --- |
| [v0.7.0](https://github.com/usamakkhan/UDM/releases/tag/v0.7.0) | 26 Aug 2026 | First archived source snapshot |
| [v0.8.0](https://github.com/usamakkhan/UDM/releases/tag/v0.8.0) | 28 Aug 2026 | [Compare v0.7.0...v0.8.0](https://github.com/usamakkhan/UDM/compare/v0.7.0...v0.8.0) |
| [v0.9.0](https://github.com/usamakkhan/UDM/releases/tag/v0.9.0) | 31 Aug 2026 | [Compare v0.8.0...v0.9.0](https://github.com/usamakkhan/UDM/compare/v0.8.0...v0.9.0) |
| [v0.10.0](https://github.com/usamakkhan/UDM/releases/tag/v0.10.0) | 2 Sep 2026 | [Compare v0.9.0...v0.10.0](https://github.com/usamakkhan/UDM/compare/v0.9.0...v0.10.0) |
| [v0.10.1](https://github.com/usamakkhan/UDM/releases/tag/v0.10.1) | 4 Sep 2026 | [Compare v0.10.0...v0.10.1](https://github.com/usamakkhan/UDM/compare/v0.10.0...v0.10.1) |
| [v0.11.0](https://github.com/usamakkhan/UDM/releases/tag/v0.11.0) | 6 Sep 2026 | [Compare v0.10.1...v0.11.0](https://github.com/usamakkhan/UDM/compare/v0.10.1...v0.11.0) |
| [v0.12.0](https://github.com/usamakkhan/UDM/releases/tag/v0.12.0) | 8 Sep 2026 | [Compare v0.11.0...v0.12.0](https://github.com/usamakkhan/UDM/compare/v0.11.0...v0.12.0) |
| [v0.12.1](https://github.com/usamakkhan/UDM/releases/tag/v0.12.1) | 11 Sep 2026 | [Compare v0.12.0...v0.12.1](https://github.com/usamakkhan/UDM/compare/v0.12.0...v0.12.1) |
| [v0.12.2](https://github.com/usamakkhan/UDM/releases/tag/v0.12.2) | 13 Sep 2026 | [Compare v0.12.1...v0.12.2](https://github.com/usamakkhan/UDM/compare/v0.12.1...v0.12.2) |
| [v0.13.0](https://github.com/usamakkhan/UDM/releases/tag/v0.13.0) | 15 Sep 2026 | [Compare v0.12.2...v0.13.0](https://github.com/usamakkhan/UDM/compare/v0.12.2...v0.13.0) |
| [v0.14.0](https://github.com/usamakkhan/UDM/releases/tag/v0.14.0) | 17 Sep 2026 | [Compare v0.13.0...v0.14.0](https://github.com/usamakkhan/UDM/compare/v0.13.0...v0.14.0) |
| [v0.14.1](https://github.com/usamakkhan/UDM/releases/tag/v0.14.1) | 20 Sep 2026 | [Compare v0.14.0...v0.14.1](https://github.com/usamakkhan/UDM/compare/v0.14.0...v0.14.1) |
| [v0.15.0](https://github.com/usamakkhan/UDM/releases/tag/v0.15.0) | 22 Sep 2026 | [Compare v0.14.1...v0.15.0](https://github.com/usamakkhan/UDM/compare/v0.14.1...v0.15.0) |
| [v0.16.0](https://github.com/usamakkhan/UDM/releases/tag/v0.16.0) | 24 Sep 2026 | [Compare v0.15.0...v0.16.0](https://github.com/usamakkhan/UDM/compare/v0.15.0...v0.16.0) |
| [v0.16.1](https://github.com/usamakkhan/UDM/releases/tag/v0.16.1) | 26 Sep 2026 | [Compare v0.16.0...v0.16.1](https://github.com/usamakkhan/UDM/compare/v0.16.0...v0.16.1) |
| [v0.29.0](https://github.com/usamakkhan/UDM/releases/tag/v0.29.0) | 26 Sep 2026 | [Compare v0.16.1...v0.29.0](https://github.com/usamakkhan/UDM/compare/v0.16.1...v0.29.0) |
| [v0.30.0](https://github.com/usamakkhan/UDM/releases/tag/v0.30.0) | 26 Sep 2026 | [Compare v0.29.0...v0.30.0](https://github.com/usamakkhan/UDM/compare/v0.29.0...v0.30.0) |
| [v0.31.0](https://github.com/usamakkhan/UDM/releases/tag/v0.31.0) | 27 Sep 2026 | [Compare v0.30.0...v0.31.0](https://github.com/usamakkhan/UDM/compare/v0.30.0...v0.31.0) |
| [v0.33.0](https://github.com/usamakkhan/UDM/releases/tag/v0.33.0) | 27 Sep 2026 | Includes [0.32 FTP-through-SOCKS](docs/parity-0.32.0.md) and [0.33 scheduler wake timers](docs/parity-0.33.0.md); [compare v0.31.0...v0.33.0](https://github.com/usamakkhan/UDM/compare/v0.31.0...v0.33.0) |
| [v0.34.0](https://github.com/usamakkhan/UDM/releases/tag/v0.34.0) | 27 Sep 2026 | [Compare v0.33.0...v0.34.0](https://github.com/usamakkhan/UDM/compare/v0.33.0...v0.34.0) |
| [v0.36.0](https://github.com/usamakkhan/UDM/releases/tag/v0.36.0) | 27 Sep 2026 | Includes [0.35 alternate audio-track selection](docs/parity-0.35.0.md) and [0.36 signed network backend](docs/parity-0.36.0.md); [compare v0.34.0...v0.36.0](https://github.com/usamakkhan/UDM/compare/v0.34.0...v0.36.0) |
| [v0.40.0](https://github.com/usamakkhan/UDM/releases/tag/v0.40.0) | 27 Sep 2026 | Includes [0.37 recorded-media outputs](docs/parity-0.37.0.md), [0.38 form-download reliability](docs/parity-0.38.0.md), [0.39 handoff recovery](docs/parity-0.39.0.md), and [0.40 proxy/FTP/scanner updates](docs/parity-0.40.0.md); [compare v0.36.0...v0.40.0](https://github.com/usamakkhan/UDM/compare/v0.36.0...v0.40.0) |
| [v0.57.0](https://github.com/usamakkhan/UDM/releases/tag/v0.57.0) | 28 Sep 2026 | Includes milestones 0.41–0.43 and 0.50–0.57; [compare v0.40.0...v0.57.0](https://github.com/usamakkhan/UDM/compare/v0.40.0...v0.57.0) |
| [v0.68.0](https://github.com/usamakkhan/UDM/releases/tag/v0.68.0) | 28 Sep 2026 | Includes milestones 0.58–0.68; [compare v0.57.0...v0.68.0](https://github.com/usamakkhan/UDM/compare/v0.57.0...v0.68.0) |

The commits were restored from the original archived source trees. This makes GitHub's Files changed view useful for every consecutive release pair.

## Documented development milestones

The project also retains the following development notes. They are committed with the current source so their work is visible in Git, but they are **not** backfilled as source tags or GitHub Releases: separate source snapshots and portable builds for these points were not available. Consequently, GitHub cannot show an honest file-by-file comparison for them. The notes describe the recorded work; they do not claim to recreate the exact code at that time.

| Documented version | Recorded change note |
| --- | --- |
| 0.17.0 | [Transfer measurements, video formats and download dialogs](docs/speed-menu-0.17.0.md) |
| 0.18.0 | [Desktop GUI expansion](docs/gui-0.18.0.md) |
| 0.19.0 | [Desktop workflow completion pass](docs/gui-0.19.0.md) |
| 0.20.0 | [YouTube panel repair and browser controls](docs/gui-0.20.0.md) |
| 0.20.1 | [Browser integration](docs/browser-0.20.1.md) |
| 0.20.2 | [Panel recovery and verified Edge download](docs/browser-0.20.2.md) |
| 0.21.0 | [Video comparison and implementation notes](docs/video-comparison-0.21.0.md) |
| 0.21.1 | [Live video comparison and storage repair](docs/video-comparison-0.21.1.md) |
| 0.21.2 | [Parallel YouTube downloads](docs/parallel-video-0.21.2.md) |
| 0.22.0 | [Media addresses and server logins](docs/links-login-0.22.0.md) |
| 0.23.0 | [Native player retrieval and browser integration](docs/player-retrieval-0.23.0.md) |
| 0.23.1 | [Extension reload recovery](docs/browser-reload-0.23.1.md) |
| 0.23.2 | [Audit](docs/audit-0.23.2.md) |
| 0.25.0 | [Browser integration update](docs/browser-parity-0.25.0.md) |
| 0.26.0 | [Native and browser comparison](docs/parity-0.26.0.md) |
| 0.27.0 | [Browser integration 0.24.0](docs/parity-0.27.0.md) |
| 0.28.0 | [Offline websites and SOCKS5](docs/parity-0.28.0.md) |
