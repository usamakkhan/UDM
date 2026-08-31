# UDM Download Manager 0.9.0

UDM now has a native x64 C++17/MFC desktop, download engine, browser messaging host and network monitor. It uses original UDM source and artwork. This is a development release with an IDM-like workflow; complete IDM feature parity is still outstanding.

See [0.9.0 engine improvements and measured results](docs/performance-0.9.md). The local slow-connection benchmark completed about 12% sooner; this is not an IDM or Internet speed guarantee.

## Run

Open `release/UDM.exe`. Keep its `assets` directory beside the executable. Windows 10/11 x64 is required. The MFC and C/C++ runtimes are linked statically; ordinary downloads need neither .NET nor Node.js. The browser extension remains JavaScript, as browser extensions require.

For captured audio/video assembly, run `setup-media.ps1` once to obtain FFmpeg and FFprobe with checksum verification. UDM downloads the bytes itself and invokes these helpers only for local media processing. There is no yt-dlp dependency. The portable ZIP excludes media helper binaries.

The optional current-user installer is `install.ps1 -StartMenu -MediaTools`. Browser registration is described in [browser/README.md](browser/README.md). The existing workspace registration uses `release/Udm.NativeHost.exe`.

## Native features

- HTTP/HTTPS parallel downloads with up to 16 workers, shared transfer sessions and dynamic splitting of slow remaining ranges; validated ranges, pause/resume, retries, changed-resource detection, sequential fallback, unknown-length and empty responses.
- SHA-256 verification, destination collision protection, atomic publication and Internet-zone marking.
- MFC category tree, file list, original icons, search, toolbar, File Info, progress, range map, speed limiter and completion dialogs.
- Multiple queues, per-queue ordering/concurrency/retries, daily/overnight and dated schedules, manual starts/stops.
- History, custom categories and folder rules, URL batches, import/export, drag/drop, optional clipboard offers, tray notifications and completion sound.
- Global and per-download speed limits, hourly quota, HTTP authorization, encrypted saved site logins and proxy credentials, external scanner hook.
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

A successful current capture-only YouTube download and an IDM/UDM speed comparison are still unverified. Controlled stream tests check identity matching, foreign-video/ad rejection, fragmented UMP responses, gaps and encrypted-media rejection; they do not establish compatibility with every live browser session. Expired or rejected playback URLs require fresh browser capture. No external resolver is used.

FTP is fresh sequential transfer and has not been tested against a live server. HLS/DASH support is bounded to the recorded clear formats documented above. Live/DRM video, subtitles, full website mirroring, system-wide interception, localization, skins, signed updates and production distribution remain incomplete. Edge/Firefox live integration and broad proxy/authentication matrices also remain unverified.

The original WFP driver has compiled and passed static/INF checks and has a separate development test signature. It is not installed or kernel-tested, and is not Microsoft production-signed. Windows Test Mode remains off. The ordinary app does not require it; a traffic-monitoring driver cannot reveal encrypted HTTPS video URLs. No automatic restart is performed.

See [native conversion](docs/native-conversion.md), [architecture](docs/architecture.md), [driver report](drivers/README.md) and [language/signing evidence](docs/language-and-signing.md). Older dated benchmark reports describe earlier builds and are not current native performance claims. No IDM binary, extension, driver or artwork is bundled.
