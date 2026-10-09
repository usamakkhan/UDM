# Changes since UDM 0.84.0 RC1 — 8 October 2026

Scope: source commits after `v0.84.0-rc.1` (`ae4500b`) through `80858a1`. This is a changelog for the Git push, not a new release. Native source still identifies as 0.84.0; the browser extensions now identify as 0.62.13. The 0.62.13 installer was built and tested but has not been installed or published. Subsequent native GUI candidates are also not packaged or installed.

## Download engine and media

- Preserve selected streaming segments across complete-part responses so continued downloads do not discard verified work.
- Decode selected gzip and Brotli SABR segments with bounded output, end-of-stream draining and failure checks.
- Record live HLS WebVTT subtitles with synchronized timing, selected language and track name. Recover subtitle state after restart and preserve selected audio metadata in MP4, M4A and TS output.
- Accept separate WebVTT initialization headers in live and recorded HLS. Validate and reuse recorded headers for cue segments, preserve timestamp mapping, and support offline retry from saved receipts.
- Reject ranged HTTP responses that omit the validator established by the download probe. Honor configured Windows FTP static-proxy bypasses and PAC URLs within the supported policy.

## Browser integration

- Expand captured POST bodies to 4 MiB, with an 8 MiB native request frame and explicit oversized-body rejection.
- Recover video panels when a player changes source during format lookup or download handoff; keep stale replies from replacing the current video's choices.
- Reject known stale playlist captures across single-page-app route revisits, including delayed fetch/XHR and tracked webRequest responses.
- Offer live and recorded captions only when the matching native capability is present. Preserve subtitle selection through the actual Edge and Firefox panel workflows.
- Advance both browser bundles from 0.62.8 to 0.62.13. The exact staged 0.62.13 package passed isolated Edge acceptance and migration checks; it was not installed or published.

## Desktop, scheduler and CLI

- Make individual Pause transactional so a failed catalog save does not prematurely stop the worker or discard the job's state.
- Defer CLI `/q` exit and `/h` dial-up hang-up until completion is saved; retain pending actions if persistence fails. Improve completion-scanner diagnostics.
- Correct Properties subtitle labels and completion-dialog icon/drag behavior.
- Preserve dialog list column widths through repeated DPI changes, including user-resized and hidden columns.
- Refresh open dialogs when appearance or system colors change. Render themed tabs and headers consistently in native captures, make scheduler queue-action glyphs readable, and theme scheduler tabs, tree and headers while preserving queue selection and download state.

## Project and validation

- Add the Apache 2.0 project license, third-party notices, portable progress-dialog references and sanitized validation records.
- Record isolated app, browser, installer and GUI acceptance in `docs/validation/` and the corresponding candidate notes. The latest scheduler-theme candidate passed 119 focused GUI checks; its build reused unchanged backend objects and was not a full clean build.

The comparison is [`v0.84.0-rc.1...80858a1`](https://github.com/usamakkhan/UDM/compare/v0.84.0-rc.1...80858a1). This source push does not establish full IDM parity, a new installed build, or a new published release.
