# UDM Download Manager 0.15.0

UDM now has a native x64 C++17/MFC desktop, download engine, browser messaging host and network monitor. It uses original UDM source and artwork. This is a development release with an IDM-like workflow; complete IDM feature parity is still outstanding.

Version 0.15.0 adds bounded server-aware HTTP retries, fixes save-folder and pause/silent command-line handoffs, and excludes playlists from previous embedded documents. See [implementation and validation](docs/reliability-0.15.0.md).

Version 0.14.1 fixes a real-browser HLS discovery race and progress-button states. Live Microsoft ISO throughput averaged 4.23 MiB/s for UDM and 3.97 MiB/s for IDM in two short samples; different CDN endpoints and test conditions prevent a general speed-win claim. See [live validation, limits and reproduction](docs/live-validation-0.14.1.md).

Version 0.14.0 adds duplicate-link choices across manual entry, browser capture, batches and imports: show/resume the existing download, create a numbered copy, or replace a completed HTTP file while retaining its previous version. Signed query strings and account credentials remain distinct. Replacement includes saved-state rollback and interrupted-operation recovery. See [implementation and 194 native checks](docs/duplicates-0.14.0.md).

Version 0.13.0 implements the live-review workflows: completed-file Move/Rename, Open with and Redownload; editable properties; configurable double-click; saved columns, Find Next and appearance preferences; background File Info downloading; 32 connections with server overrides; separate temporary storage, configurable quota periods and server timestamps; pending queue membership; and browser panels for selected text containing links. See [implementation and validation](docs/workflows-0.13.0.md).

Version 0.12.2 fixed list sorting, download action enablement, and keyboard context menus. The [103-check audit](docs/idm-feature-audit-2026-09-23.md) remains the historical baseline; the 0.13.0 and 0.14.0 reports record subsequent implemented changes.

Version 0.12.1 makes double-clicking a completed download open File Properties. The same dialog is available from File > Properties and the download context menu, with separate Open and Open folder buttons. Version 0.13.0 adds editable metadata and a separate Move/Rename operation.

Version 0.12.0 adds Refresh download address: capture or paste a fresh link, verify it identifies the same file, and resume preserved parts. The failed Microsoft ISO was recovered through this flow, completed, and independently verified against Microsoft's SHA-256. See [implementation and 125 passing native checks](docs/recovery-0.12.0.md).

Version 0.11.0 starts larger contiguous download ranges to reduce request overhead, tracks clipped/hidden/resized video players promptly, and preserves cross-site integration when rebuilding the extension. See [0.11.0 implementation and validation](docs/improvements-0.11.0.md).

Version 0.10.1 fixed Pause on stalled HTTP requests and restored native menu commands. See [stress-testing results and limits](docs/stress-testing-0.10.1.md): 95 native checks, 61 browser checks, 24 engine stress groups and 11 protocol/storage groups passed.

See [0.10.0 IDM analysis and measured results](docs/reverse-engineering-0.10.md). Earlier help for slow connections reduced local fixture completion time by 20.3% versus 0.9.0. Live IDM comparisons and their limits are included; Internet performance parity remains unproven.

## Run

Open `release/UDM.exe`. Keep its `assets` directory beside the executable. Windows 10/11 x64 is required. The MFC and C/C++ runtimes are linked statically; ordinary downloads need neither .NET nor Node.js. The browser extension remains JavaScript, as browser extensions require.

For captured audio/video assembly, run `setup-media.ps1` once to obtain FFmpeg and FFprobe with checksum verification. UDM downloads the bytes itself and invokes these helpers only for local media processing. There is no yt-dlp dependency. The portable ZIP excludes media helper binaries.

The optional current-user installer is `install.ps1 -StartMenu -MediaTools`. Browser registration is described in [browser/README.md](browser/README.md). The existing workspace registration uses `release/Udm.NativeHost.exe`.

## Refresh an expired download link

Select a paused or failed HTTP/HTTPS file, then choose **File > Refresh download address** (also available by right-clicking the download or from its progress dialog). Click **Open page** and send a fresh link for the same file through the UDM browser extension, or paste its direct URL. Choose **Save and resume**. UDM verifies the replacement before reusing saved parts. Completed downloads do not need this command; captured video streams use the browser video panel.

## Native features

- HTTP/HTTPS parallel downloads with up to 32 workers, shared transfer sessions and dynamic splitting of slow remaining ranges; validated ranges, pause/resume, retries, changed-resource detection, sequential fallback, unknown-length and empty responses.
- Expired-link recovery with an original-page shortcut, encrypted pending browser replacements, and validation before saved bytes are reused.
- SHA-256 verification, destination collision protection, atomic publication and Internet-zone marking.
- MFC category tree, file list, original icons, search, toolbar, File Info, progress, range map, speed limiter and completion dialogs.
- Multiple queues, per-queue ordering/concurrency/retries, daily/overnight and dated schedules, manual starts/stops.
- History, custom categories and folder rules, URL batches, import/export, drag/drop, optional clipboard offers, tray notifications and completion sound.
- Global and per-download speed limits, quota over a configurable 1–168 hour period, HTTP authorization, encrypted saved site logins and proxy credentials, external scanner hook.
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
node .\tests\media.test.cjs
# With UDM running:
node .\tests\native-host.cjs
.\package.ps1
```

Close the corresponding UDM executable before building. `native/build.ps1` produces an isolated `release-native` build; the root build script copies the three application binaries and icons into `release`. Tests use local fixtures and separate state. Full media tests require FFmpeg/FFprobe in `release-native/tools`; Node.js is needed only for extension and handoff tests. See [native/README.md](native/README.md).

## Data and compatibility

History and settings remain in `%LOCALAPPDATA%\UDM`. The native implementation reads the earlier JSON schema, dates and dictionaries and uses the same current-user DPAPI protection. It preserves a `state.before-native.json` copy on migration and uses atomic saves with `state.json.bak`. Downloaded files and existing partial directories are retained. Exit UDM before restoring a backup.

Captured URLs can contain private query tokens; review history and exported lists before sharing them. Downloads are never executed automatically.

## Current limits

A successful current capture-only YouTube download remains unverified. Short live Microsoft ISO comparisons are documented above; a controlled full-file completion comparison and general speed superiority remain unverified. Controlled stream tests check identity matching, foreign-video/ad rejection, fragmented UMP responses, gaps and encrypted-media rejection; they do not establish compatibility with every live browser session. Expired or rejected playback URLs require fresh browser capture. No external resolver is used.

FTP is fresh sequential transfer and has not been tested against a live server. HLS/DASH support is bounded to the recorded clear formats documented above. Live/DRM video, subtitles, full website mirroring, system-wide interception, localization, skins, signed updates and production distribution remain incomplete. Edge/Firefox live integration and broad proxy/authentication matrices also remain unverified.

The original WFP driver has compiled and passed static/INF checks and has a separate development test signature. It is not installed or kernel-tested, and is not Microsoft production-signed. Windows Test Mode remains off. The ordinary app does not require it; a traffic-monitoring driver cannot reveal encrypted HTTPS video URLs. No automatic restart is performed.

See [native conversion](docs/native-conversion.md), [architecture](docs/architecture.md), [driver report](drivers/README.md) and [language/signing evidence](docs/language-and-signing.md). Older dated benchmark reports describe earlier builds and are not current native performance claims. No IDM binary, extension, driver or artwork is bundled.
