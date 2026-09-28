# UDM Download Manager

UDM is a free, source-available download manager for Windows. It is built as an independent, modern alternative to Internet Download Manager (IDM): fast, dependable downloads; useful browser integration; and a desktop workflow that keeps files, queues, and history under control.

The goal is straightforward: give Windows users a capable download manager without a trial period, activation screen, or subscription.

UDM **0.69.0** with browser integration **0.45.0** adds reviewed direct-video session recovery and checks player links beyond their first byte. See [verification and remaining gaps](docs/parity-0.69.0.md). Full IDM parity and current public video-site compatibility remain incomplete.

## Built for downloading

- Parallel HTTP and HTTPS downloads with dynamic range splitting, pause/resume, retries, and sequential fallback.
- Resume support that validates a refreshed link before reusing saved partial data.
- SHA-256 verification, protected file publication, collision handling, and Internet-zone marking.
- Support for ordinary files, unknown-size responses, and server configurations that do not allow ranges.
- Per-download and global speed limits, connection limits, quotas, and configurable retry behavior.

## Browser and video capture

UDM includes companion extensions for Chrome, Edge, and Firefox.

- Send links and selected page content directly to UDM.
- Detect supported recorded media and show available quality, container, and audio-track choices.
- Download supported clear HLS and static MP4 DASH streams with native segment handling and local audio/video merging.
- Preserve browser sign-in context during a supported handoff without exposing saved credentials in the browser UI.
- Recover interrupted automatic browser handoffs using saved ownership receipts, without replaying a request whose outcome is unknown.
- Refresh expired file links from the original page instead of discarding an incomplete download.

Browser media support intentionally has limits: DRM, live streams, and unsupported site-specific formats are not presented as downloadable files.

## Keep downloads organized

- Categories, folder rules, queues, history, search, and import/export.
- Daily, overnight, dated, and repeating schedules with configurable queue behavior.
- Batch links, drag and drop, clipboard offers, duplicate handling, and download properties.
- File Info, progress maps, speed reporting, completed-file actions, and tray notifications.
- Optional external scan command after a completed download.

## Connections and privacy controls

- Authenticated HTTP downloads and encrypted saved site-login data for the current Windows user.
- HTTP, SOCKS, and dial-up connection settings with explicit bypass rules.
- A signed network-monitor component for optional connection diagnostics. Starting it asks for administrator approval; normal downloads do not require elevation.
- Sensitive stored data is protected with Windows account encryption. Review download history and exports before sharing them, because captured URLs can contain private tokens.

## Install UDM

1. Download the x64 setup executable from [GitHub Releases](https://github.com/usamakkhan/UDM/releases).
2. Run the installer on Windows 10 or Windows 11 x64.
3. Install the browser extension using the instructions in [browser/README.md](browser/README.md).
4. Open UDM and add a link, capture one from a browser, or import a list.

The installer includes UDM, the required media helpers, browser-host registration, and the signed network runtime. Ordinary downloads do not require .NET or Node.js.

## UDM's goal

IDM is the benchmark UDM is working to compete with: a polished Windows download workflow, reliable resume behavior, useful browser capture, and strong transfer management. UDM is independently designed and implemented; it does not include IDM code, binaries, extensions, drivers, or artwork.

UDM is still evolving. Some advanced compatibility, media, localization, and distribution work remains. The project avoids claiming feature or speed parity where it has not been established.

## Source and contributions

The complete application source, browser integration, installer sources, tests, and technical notes are in this repository. UDM is free to use and welcomes issue reports and contributions.

> **License status:** a repository-wide open-source license has not yet been selected. Until one is added, the published source is available for review and development, but it is not accompanied by a general license grant.

## For developers

- `native/` — C++17/MFC desktop app, transfer engine, native host, and monitor.
- `browser/` — Chrome, Edge, and Firefox extension sources.
- `drivers/` — network-monitor implementation and supporting materials.
- `tests/` — protocol, browser, and local-fixture test coverage.
- `docs/` — technical notes and compatibility records.

See [CHANGELOG.md](CHANGELOG.md) for release history and version comparisons. Build details are in [native/README.md](native/README.md).
