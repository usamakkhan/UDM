# Changes since UDM 0.84.0 RC1 — 8 October 2026

Scope: source commits after `v0.84.0-rc.1` (`ae4500b`) through `b2a10af`. Native source still identifies as 0.84.0; the browser extensions now identify as 0.62.13. An earlier 0.62.13 installer was built and tested but has not been installed or published. Subsequent native GUI candidates were not part of that earlier package.

## Download engine and media

- Preserve selected streaming segments across complete-part responses so continued downloads do not discard verified work.
- Decode selected gzip and Brotli SABR segments with bounded output, end-of-stream draining and failure checks.
- Record live HLS WebVTT subtitles with synchronized timing, selected language and track name. Recover subtitle state after restart and preserve selected audio metadata in MP4, M4A and TS output.
- Accept separate WebVTT initialization headers in live and recorded HLS. Validate and reuse recorded headers for cue segments, preserve timestamp mapping, and support offline retry from saved receipts.
- Reject ranged HTTP responses that omit the validator established by the download probe. Honor configured Windows FTP static-proxy bypasses and PAC URLs within the supported policy.
- Recognize HLS and DASH playlist MIME types in the signed network HTTP inspector and require an exact `attachment` disposition token before treating a response as a file candidate.
- Preserve the original browser response if a network handoff decision fails; disable later decisions for that flow and record the failure without claiming interception.
- Retain a bounded, process-attributed diagnostic snapshot of HTTP download observations without taking ownership of the original response.
- Expose scoped capture observations through the desktop broker with session identity and bounded snapshots; a process watch is required before capture starts.

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
- Verify scheduler Apply, Close, window-close and Escape save paths, per-queue startup preferences, and catalog persistence without starting downloads.
- Support Ctrl+Tab, Ctrl+Shift+Tab and Ctrl+PageUp/PageDown across tabbed settings dialogs from their child controls, honoring canceled tab changes and restoring visible focus.
- Show the active download-list sort direction in light and dark headers.

## Project and validation

- Add the Apache 2.0 project license, third-party notices, portable progress-dialog references and sanitized validation records.
- Record isolated app, browser, installer and GUI acceptance in `docs/validation/` and the corresponding candidate notes. The latest scheduler-theme candidate passed 119 focused GUI checks; its build reused unchanged backend objects and was not a full clean build.
- Requalify direct and adaptive media refresh with isolated Edge fixtures and add before/after network parser evidence for the playlist MIME correction.
- Make failed required installer steps terminate safely in silent mode; a private compiled fixture passed seven success/failure and later-failure cases without installing UDM.

The comparison is [`v0.84.0-rc.1...b2a10af`](https://github.com/usamakkhan/UDM/compare/v0.84.0-rc.1...b2a10af). Complete IDM parity and a new installed build remain unverified.
