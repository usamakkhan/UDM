# UDM Download Manager

UDM is a native Windows download manager for reliable HTTP/HTTPS transfers, browser-captured media, queues, and organized download history. It has an independent C++17/MFC desktop application and companion extensions for Chrome, Edge, and Firefox.

## What UDM does

- Downloads files with parallel connections, dynamic range splitting, pause/resume, retries, changed-resource detection, and sequential fallback.
- Verifies completed downloads with SHA-256, handles naming collisions, and safely publishes completed files.
- Organizes downloads through categories, queues, schedules, search, history, import/export, and folder rules.
- Captures supported browser links and media, including direct media addresses and available quality choices.
- Supports authenticated sites, proxy credentials, speed limits, quota windows, and an optional external scan hook.

## Quick start

Open `release/UDM.exe` and keep the `assets` directory beside it. UDM targets Windows 10/11 x64 and ordinary downloads do not require .NET or Node.js.

Download and run the x64 setup executable from the [GitHub Releases page](https://github.com/usamakkhan/UDM/releases). Setup installs UDM, the media helpers, browser-host registration and the UDM WFP development driver. It enables Windows Test Mode and requires a restart; Secure Boot can prevent Test Mode from being enabled.

## Release history and comparisons

Every archived version from `v0.7.0` through `v0.16.1`, plus the current `v0.33.0` release, is available as an ordered source snapshot in Git. Current releases use the x64 setup executable rather than a portable ZIP.

Use [CHANGELOG.md](CHANGELOG.md) for direct, one-click comparisons between consecutive versions, or browse the [GitHub Releases](https://github.com/usamakkhan/UDM/releases) page.

## UDM and IDM

IDM is a useful comparison point for established download-manager workflows, but UDM is its own application with an independent codebase, UI assets, browser integration, and transfer implementation. No IDM binaries, extensions, drivers, or artwork are included. Mentions of IDM in technical documentation are compatibility observations, not claims of complete feature or performance parity.

## Project layout

- `native/` — current C++17/MFC application, native host, and transfer engine.
- `browser/` — Chrome, Edge, and Firefox extension sources.
- `src/` — earlier C# implementation retained for reference.
- `tests/` — native, browser, protocol, and fixture-driven checks.
- `docs/` — architecture notes, validation evidence, and technical references.

## Technical validation history

Native **0.25.0** / browser **0.23.2**: fixes duplicate browser transfers after accepted downloads, stalled error reporting, interrupted link-selection batches and stale playlist capture after navigation. The audit passed 453 C++ checks and actual Chrome, Edge and Firefox downloads. A live 1080p YouTube download also completed with verified audio/video. See [audit fixes, validation and remaining limits](docs/audit-0.23.2.md) and [broader coverage](docs/browser-parity-0.25.0.md).

Native 0.23.0 and browser integration 0.21.0 add native YouTube player retrieval, validated direct video/audio links, bounded prefetch, and document-bound handoff. The live TV-client probe returned `LOGIN_REQUIRED`, so **IDM transport and speed parity are not established**; browser capture remains available. See [implementation, observed IDM requests, tests and remaining work](docs/player-retrieval-0.23.0.md).


Version 0.22.0 displays actual captured media addresses separately from the web page, adds a Links dialog, and adds visible username/password, login-and-retry, and saved HTTPS site controls. Installed host verification passed; all **19 download records and settings are unchanged**. Live Edge UI verification remains blocked by Windows desktop access. [Changes, IDM inspection, validation, and remaining gaps](docs/links-login-0.22.0.md).

Version 0.21.2 adds up to eight parallel YouTube timeline workers, actual connection rows and verified overlap assembly. [Implementation and live-service limits](docs/parallel-video-0.21.2.md). Isolated live tests hit browser attestation; real-world completion and speed gains for this new path remain unverified. All 19 existing download records are preserved.

Version 0.21.1 and browser integration 0.20.3: [live IDM/UDM video comparison and media-storage repair](docs/video-comparison-0.21.1.md). Both apps completed the same 1080p video; encoded video/audio hashes match. UDM now honors its temporary-folder preference for captured media and reports storage failures clearly. Full IDM feature parity and speed parity are not established.

Previous browser integration deployment: [0.20.2 — panel recovery and verified Edge download](docs/browser-0.20.2.md). The regular Edge profile completed a 266 MB YouTube download at 1080p with audio. Start/middle/end audio-video decoding checks passed; the full scan timed out. 66 browser checks passed; isolated Edge profiles still encountered attestation rejection. Native desktop remains 0.20.0. The original 15 history records are preserved; two new test records bring the total to 17.

Browser integration update: [UDM 0.20.0 - YouTube panel repair and desktop browser controls](docs/gui-0.20.0.md). The repaired button and actual format menu were verified in Chrome; panel controls also passed in isolated Edge tests. All 15 download records and both paused ISO byte counts are preserved.

Desktop workflow update: [UDM 0.19.0 - scheduler automation, drop basket, catalog, ZIP preview and Grabber wizard](docs/gui-0.19.0.md). All 15 records and both paused ISO byte counts are preserved.

Previous desktop GUI update: [UDM 0.18.0 — Properties, dialogs, menus and remaining gaps](docs/gui-0.18.0.md).
UDM now has a native x64 C++17/MFC desktop, download engine, browser messaging host and network monitor. It uses original UDM source and artwork. This is a development release with an IDM-like workflow; complete IDM feature parity is still outstanding.

Version 0.17.0 adds steadier payload speed reporting, reusable streaming connections, real MP4/TS video choices with bitrate labels, Download all, and improved progress-window controls. The update passed 251 native and 107 browser checks. See [measured speed graph, changes and verification limits](docs/speed-menu-0.17.0.md).

Version 0.16.1 fixes rearming one-time schedules and supports an explicit shared history directory; all 14 existing records are preserved on this PC. See [feature audit, remaining gaps and live video result](docs/features-0.16.1.md).

Version 0.16.0 adds browser-level streaming request capture, including worker requests missed by page observers, with document/video/ad checks and usable-codec preference. Native HTTP reads now use the tested 16 KiB buffer, and unchanged periodic state saves are skipped. One live 1080p YouTube completion has since been verified; broader compatibility remains unverified. See [implementation, IDM observations and validation](docs/video-capture-0.16.0.md).

Version 0.15.0 adds bounded server-aware HTTP retries, fixes save-folder and pause/silent command-line handoffs, and excludes playlists from previous embedded documents. See [implementation and validation](docs/reliability-0.15.0.md).

Version 0.14.1 fixes a real-browser HLS discovery race and progress-button states. Live Microsoft ISO throughput averaged 4.23 MiB/s for UDM and 3.97 MiB/s for IDM in two short samples; different CDN endpoints and test conditions prevent a general speed-win claim. See [live validation, limits and reproduction](docs/live-validation-0.14.1.md).

Version 0.14.0 adds duplicate-link choices across manual entry, browser capture, batches and imports: show/resume the existing download, create a numbered copy, or replace a completed HTTP file while retaining its previous version. Signed query strings and account credentials remain distinct. Replacement includes saved-state rollback and interrupted-operation recovery. See [implementation details](docs/duplicates-0.14.0.md).

Version 0.13.0 implements the live-review workflows: completed-file Move/Rename, Open with and Redownload; editable properties; configurable double-click; saved columns, Find Next and appearance preferences; background File Info downloading; 32 connections with server overrides; separate temporary storage, configurable quota periods and server timestamps; pending queue membership; and browser panels for selected text containing links. See [implementation and validation](docs/workflows-0.13.0.md).

Version 0.12.2 fixed list sorting, download action enablement, and keyboard context menus. The [103-check audit](docs/idm-feature-audit-2026-09-23.md) remains the historical baseline; the 0.13.0 and 0.14.0 reports record subsequent implemented changes.

Version 0.12.1 makes double-clicking a completed download open File Properties. The same dialog is available from File > Properties and the download context menu, with separate Open and Open folder buttons. Version 0.13.0 adds editable metadata and a separate Move/Rename operation.

Version 0.12.0 adds Refresh download address: capture or paste a fresh link, verify it identifies the same file, and resume preserved parts. The failed Microsoft ISO was recovered through this flow, completed, and independently verified against Microsoft's SHA-256. See [implementation details](docs/recovery-0.12.0.md).

Version 0.11.0 starts larger contiguous download ranges to reduce request overhead, tracks clipped/hidden/resized video players promptly, and preserves cross-site integration when rebuilding the extension. See [0.11.0 implementation and validation](docs/improvements-0.11.0.md).

Version 0.10.1 fixed Pause on stalled HTTP requests and restored native menu commands. See [stress-testing results and limits](docs/stress-testing-0.10.1.md).

See [0.10.0 IDM analysis and measured results](docs/reverse-engineering-0.10.md). Earlier help for slow connections reduced local fixture completion time by 20.3% versus 0.9.0. Live IDM comparisons and their limits are included; Internet performance parity remains unproven.

## Run

Open `release/UDM.exe`. Keep its `assets` directory beside the executable. Windows 10/11 x64 is required. The MFC and C/C++ runtimes are linked statically; ordinary downloads need neither .NET nor Node.js. The browser extension remains JavaScript, as browser extensions require.

UDM downloads the bytes itself and invokes FFmpeg/FFprobe only for local media processing. There is no yt-dlp dependency. The setup executable includes the media helpers.

The setup executable registers the browser host system-wide. Browser extension installation is described in [browser/README.md](browser/README.md).

## Refresh an expired download link

Select a paused or failed HTTP/HTTPS file, then choose **File > Refresh download address** (also available by right-clicking the download or from its progress dialog). Click **Open page** and send a fresh link for the same file through the UDM browser extension, or paste its direct URL. Choose **Save and resume**. UDM verifies the replacement before reusing saved parts. Completed downloads do not need this command; captured video streams use the browser video panel.

## Native features

- HTTP/HTTPS parallel downloads with up to 32 workers, shared transfer sessions and dynamic splitting of slow remaining ranges; validated ranges, pause/resume, retries, changed-resource detection, sequential fallback, unknown-length and empty responses.
- Expired-link recovery with an original-page shortcut, encrypted pending browser replacements, and validation before saved bytes are reused.
- SHA-256 verification, destination collision protection, atomic publication and Internet-zone marking.
- MFC category tree, file list, original icons, search, toolbar, File Info, progress, range map, speed limiter and completion dialogs.
- Multiple queues, per-queue ordering/concurrency/retries, daily/overnight and dated schedules, manual starts/stops.
- History, custom categories and folder rules, URL batches, import/export, drag/drop, optional clipboard offers, tray notifications and completion sound.
- Global and per-download speed limits, quota over a configurable 1�168 hour period, HTTP authorization, encrypted saved site logins and proxy credentials, external scanner hook.
- Bounded same-origin static HTML grabber with filters and saved projects.
- Current-user native messaging, explicit extension identities, durable browser confirmation and duplicate handoff handling.
- Browser-captured direct media URLs, detected quality choices, local audio/video merging and selected-height verification. The original SABR/UMP transport is experimental.
- Compact draggable panels on permitted HTML5 players, including embedded frames; clear recorded HLS and static MP4 DASH with native segment downloads and verified resume. See [cross-site video](docs/cross-site-video.md) for supported formats and limits.
- Native TCP/UDP endpoint diagnostics and an optional client for UDM's original C WFP driver.

The UI and engine were rewritten in C++; `src/` and `build-legacy.ps1` retain the previous C# implementation for reference. The native binaries do not launch it. There is no trial expiry or activation system.

## Build and check

Use Visual Studio 2022's x64 developer environment with Desktop C++, MFC/ATL and a Windows SDK installed. The build also supports the verified Microsoft toolchain cache on this development PC.

```powershell
.\setup-media.ps1
.\build.ps1 -Test
# Explicitly prefer the installed Visual Studio environment:
.\build.ps1 -Test -UseInstalledToolchain
node .\tests\browser.test.cjs
node .\tests\capture.test.cjs
node .\tests\ump.test.cjs
node .\tests\streaming-capture.test.cjs
node .\tests\prepare.test.cjs
node .\tests\media.test.cjs
# With UDM running:
node .\tests\native-host.cjs
.\package.ps1
```

Close the corresponding UDM executable before building. `native/build.ps1` produces an isolated `release-native` build; the root build script copies the three application binaries and icons into `release`. Tests use local fixtures and separate state. Full media tests require FFmpeg/FFprobe in `release-native/tools`; Node.js is needed only for extension and handoff tests. See [native/README.md](native/README.md).

## Data and compatibility

History and settings default to `%LOCALAPPDATA%\UDM`. An optional `udm-data.json` beside UDM.exe can select an absolute or installation-relative `dataDirectory`; for example, `{"dataDirectory":"..\\user-data"}`. This development installation uses `D:\UDM\user-data` so desktop and browser launches share the same 15-record history. Exit UDM before changing this path and migrate the existing state/parts together. The private configuration and history are excluded from the portable ZIP. The native implementation reads the earlier JSON schema, dates and dictionaries and uses the same current-user DPAPI protection. It preserves a `state.before-native.json` copy on migration and uses atomic saves with `state.json.bak`. Downloaded files and existing partial directories are retained. Exit UDM before restoring a backup.

Captured URLs can contain private query tokens; review history and exported lists before sharing them. Downloads are not executed automatically by default. Queue automation can open an explicitly selected file after its user-configured, cancelable completion countdown.

## Current limits

One live browser-captured 1080p YouTube video completed with verified H.264 video and AAC audio (130,762,165 bytes in 34.383 seconds), using original UDM SABR transport. This establishes that one session, not universal live compatibility. Short live Microsoft ISO comparisons are documented above; a controlled full-file completion comparison and general speed superiority remain unverified. Controlled stream tests check identity matching, foreign-video/ad rejection, fragmented UMP responses, gaps and encrypted-media rejection; they do not establish compatibility with every live browser session. Expired or rejected playback URLs require fresh browser capture. No external resolver is used.

FTP is fresh sequential transfer and has not been tested against a live server. HLS/DASH support is bounded to the recorded clear formats documented above. Live/DRM video, subtitles, full website mirroring, system-wide interception, localization, skins, signed updates and production distribution remain incomplete. Edge/Firefox live integration and broad proxy/authentication matrices also remain unverified.

The setup executable installs the WFP development driver and its development certificate, enables Windows Test Mode, and requires a restart. It is not Microsoft production-signed; a traffic-monitoring driver cannot reveal encrypted HTTPS video URLs.

See [native conversion](docs/native-conversion.md), [architecture](docs/architecture.md), [driver report](drivers/README.md) and [language/signing evidence](docs/language-and-signing.md). Older dated benchmark reports describe earlier builds and are not current native performance claims. No IDM binary, extension, driver or artwork is bundled.
