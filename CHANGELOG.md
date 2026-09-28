# Release history

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
