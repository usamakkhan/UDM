# Release history

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
