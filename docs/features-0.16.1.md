# UDM 0.16.1 — other-feature audit and history repair

Reviewed 24 September 2026 against the current C++/MFC implementation, fresh local tests, native desktop dialogs, the historical 103-item audit, and IDM's published workflows. This is a focused update, not a claim that every historical item has been retested or that UDM has complete IDM parity.

## Confirmed fixes

**One-time queues can be scheduled again.** Previously, changing a completed one-time queue to a new date retained `OnceStarted`. If it was empty, the next scheduler tick disabled it before the new start time. `Manager::setQueue` now resets that marker when the one-time window changes, when switching into one-time mode, or when explicitly re-enabling a disabled one-time queue. Ordinary edits to a running schedule preserve its marker. A regression run reproduced two failures before the fix; the reschedule and persisted-state checks now pass.

**All launch routes have a stable history location.** On this PC, the tool-launched app and the browser-launched app read different physical directories despite the same nominal Local AppData path. One contained 13 records; the other contained the newly completed video. The installation now reads `release/udm-data.json` and uses `D:\UDM\user-data`. All 14 records were merged by identity, saved partial paths rebased, and copied partial files checked by SHA-256. Both source directories and pre-upgrade binaries were retained. The two paused ISO byte counts remain 546,308,096 and 571,867,136. There are nine complete, three failed and two paused downloads, with none running.

The new native `diagnostics` command reports version, data directory and record count without exposing download URLs. The installed host reports version 0.16.1 and 14 records in the shared directory. The setting is optional: ordinary installations without `udm-data.json` still use Local AppData. Malformed configuration fails visibly instead of silently selecting an unrelated history. The development machine's private configuration and user data are excluded from the portable package.

## Current feature findings

“Tested” below refers to the native regression suite in this audit unless another source is stated. A local fixture proves the tested behavior, not every Internet server or Windows configuration.

| Area | Current UDM result | Evidence or remaining limit |
|---|---|---|
| Pause/resume and range splitting | Tested | Exact final bytes, retained parts, restart restoration, stalled-request cancellation and dynamic range ownership. |
| Expired download addresses | Tested | Reject bad replacements; validate and resume retained bytes with the same job identity. The current old ISO links were not retried during this audit. |
| Duplicate downloads | Tested; settings inspected live | Existing job, numbered copy and explicit completed-file replacement, including collisions, atomic rollback and account/query distinctions. |
| Completed-file double-click | Verified live | Opens File Properties for the local 8 MiB fixture; file remains unopened. |
| Properties, Move/Rename, Redownload | Tested; Properties inspected live | Metadata edits, collision protection, cross-volume file integrity and rollback; Open, Open with and Open folder remain available. |
| File Info prefetch | Tested | Receives bytes before confirmation, blocks publication, cancels promptly and resumes retained data after confirmation. |
| Queue concurrency and stop | Tested with new fixtures | Per-queue and global limits, no premature future-schedule network requests, cancellation and reuse of released slots. |
| Daily/overnight/one-time schedules | Tested with stated bounds | Existing overnight start-day check plus new exact one-time boundaries, completion and rearming/restart checks. DST transitions and wake-from-sleep remain unverified. |
| Pending queue membership | Tested and inspected live | Scheduler shows five pending records and excludes all nine completed records. Queue ordering is button-based. |
| Queue retries | Partial | HTTP request retries are bounded and server-aware. Failed-file rotation/retry as an entire queue operation is incomplete; Start now currently queues paused members. |
| Periodic synchronization / every-N-minutes runs | Missing | No remote-change synchronization queue or repeat-interval workflow. |
| Queue completion actions | Missing | No queue-specific run-program, disconnect, application-exit or shutdown action. |
| Speed limits, quotas, connection overrides | Tested; controls inspected live | Global/per-file limits, 1–168 hour quota periods, up to 32 connections and per-server limits. |
| Temporary folders and server timestamps | Tested | Existing parts keep their paths; new jobs use the chosen directory; valid server timestamps applied to completed files. |
| History and settings | Tested; 14 records visible live | Atomic writes/backups, failure recovery, stable data location and persisted appearance/interaction preferences. |
| Cold/warm launch and CLI | Five fresh installed-build tests passed | Explicit folder/name, paused mode, silent exact-byte transfer, ordinary File Info confirmation and rejection of browser-issued local-only commands. |
| Search, columns and appearance | Implemented; prior UI evidence | Saved columns, sort, Find Next, font, category visibility and content theme; not all were freshly clicked in this audit. Full toolbar/skin customization remains missing. |
| Batches and import/export | Partial | Numeric/letter URL ranges and text URL lists. No rich history/credential/queue metadata round-trip or ZIP-content preview. |
| Categories, drag/drop and tray | Partial | Category rules and input drag/drop exist. No complete category context editor, floating drop basket, completed-file drag-out or rich tray queue controls. |
| Proxy/authentication | Partial | HTTP proxy and encrypted saved credentials exist; SOCKS and broad live proxy/authentication matrices remain incomplete. |
| FTP | Partial | Fresh sequential transfer; resumable FTP and a live FTP-server test remain outstanding. |
| Scanner hook and event sounds | Partial | Configured scanner launch and completion sound exist; no scan-verdict gating or full per-event sound editor. |
| Grabber | Tested within a limited scope | Bounded same-origin static HTML exploration and saved projects. No full offline site mirroring or rendered-site wizard. |
| Distribution and accessibility | Incomplete | Portable build and current-user setup exist. Production signing/updater/repair, localization, screen-reader and mixed-DPI coverage remain outstanding. |

IDM documents synchronization intervals, failed-file queue retries, completion actions and drag ordering in its [scheduler documentation](https://www.internetdownloadmanager.com/support/idm-scheduler/idm_scheduler.html). Its [options documentation](https://www.internetdownloadmanager.com/support/options.html) describes additional controls such as capture modifier keys, per-address exclusions, manual User-Agent configuration, proxy/SOCKS and per-event sounds. These are behavior references; no IDM code or artwork was copied.

## Validation and deployment

- Native tests: **241 passed, zero failed**, including 11 new scheduler checks and eight history/diagnostic checks added during the preceding history investigation.
- Regression demonstration before the scheduling fix: 239 passed, two failed; the two failures were rescheduling and restoration of the rearmed queue.
- Installed desktop launch tests: **five passed**, using separate fixture history and a loopback HTTP server.
- Extension diagnostics JavaScript syntax passed. Repeated extension preparation passed and retained the stable Chromium identity, permissions and shared Firefox sources. Bundles are version 0.16.1; a running browser's new diagnostics script was not reloaded or live-tested during this audit.
- All three deployed executable hashes match the tested isolated build. The upgraded app is open with 14 records and zero active transfers. Live native inspection covered completed Properties, Scheduler membership, and General/Connection/Duplicates settings; dialogs were closed without changing user preferences.
- Machine-readable summary: [features-0.16.1.json](reference/features-0.16.1.json). Detailed local logs are under `benchmarks/features-0.16.1`; private history exports are excluded from packages.

## Correction to the previous video status

The earlier 0.16.0 report's unverified live-YouTube status has been superseded by one successful ordinary-browser 1080p capture. Original UDM SABR transport completed 130,762,165 bytes in 34.383 seconds. FFprobe verified 1920×1080 H.264 plus AAC over 635.786 seconds; the saved file hash matches the completed record. No yt-dlp was used. This does not establish every quality/platform, universal ad exclusion, overlay visibility on every page, subtitle/audio-track selection or an IDM speed advantage. Those remain separate gaps; no new Internet speed comparison was performed in this audit.
