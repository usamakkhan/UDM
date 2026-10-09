# Changes since UDM 0.84.0 RC2

Source range: [`v0.84.0-rc.2...2097180`](https://github.com/usamakkhan/UDM/compare/v0.84.0-rc.2...2097180). Native files remain version 0.84.0 and both browser extensions remain version 0.62.13.

- Correct HLS WebVTT timing when LOCAL/MPEGTS anchors are far apart, including 33-bit clock rollover. Keep valid empty subtitle tracks through live recording, download recovery, and output.
- Support larger recorded HLS and DASH selections, up to 10,000 parts and a 4 MiB plan, with corresponding browser/native message bounds.
- Reduce adaptive checkpoint writes with durable per-segment receipts and generation-aware snapshots. Existing completed-download records remain readable.
- Keep pipelined, partially received, and body-framed GET requests with their original HTTP client during network takeover decisions. The signed network helper remains observation-oriented for those flows.
- Expose the Properties source link through Windows accessibility with a link role, accessible name, state, and default action.
- Record isolated integrated app, CLI, Edge caption-panel, installed companion, browser, network, and adaptive-journal validation in `docs/validation/`.

The controlled browser and loopback fixtures do not establish broad public-site compatibility, driver-enabled gateway behavior, or complete IDM parity.
