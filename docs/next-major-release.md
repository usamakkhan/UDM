# Next major release preparation

The next major release number and publication date are not assigned. Development versions continue to increase for each functional improvement. Per the 2026-10-30 instruction, build native binaries but do not generate setup packages during these increments; tests remain deferred. A build is not a release approval or proof of IDM parity.

## Current delivery baseline

- Installed: 1.6.0, browser 0.63.82; four native executable hashes and Windows display version verified after the 2026-10-31 direct update. Existing integration paths were retained. See [installation record](validation/install-1.6.0-20261031.json).
- Last historical installer: 1.2.9, browser 0.63.82; built but not installed or published.
- Current native development version is 2.23. All five production executables built and their version metadata was verified. [Current build receipt](validation/single-range-resume-2.23-build.json)

## Changes to carry into the release

| Version | Change | Evidence/status |
|---|---|---|
| 2.23 | Preserve a single resumable range after a transient helper failure when the original response advertises byte ranges and supplies a usable validator | [Native build receipt](validation/single-range-resume-2.23-build.json). Runtime tests deferred. |
| 2.22 | Keep the full HTTP response after a recoverable optional range failure and defer later requests as directed by the server | [Native build receipt](validation/optional-range-fallback-2.22-build.json). Runtime tests deferred. |
| 2.21 | Verify range support with the first helper's actual byte range and reuse its validated response | [Native build receipt](validation/reused-range-probe-2.21-build.json). Runtime tests deferred. |
| 2.20 | Start with balanced HTTP ranges bounded by the connection count and split slow ranges on demand | [Native build receipt](validation/balanced-ranges-2.20-build.json). Runtime tests deferred. |
| 2.19 | Show a filename-titled HTTP 403 warning for confirmed individual direct downloads | [Native build receipt](validation/forbidden-dialog-2.19-build.json). Runtime tests deferred. |
| 2.18 | Download Complete puts file actions first in keyboard order and matches the captured abbreviated size format | [Native build receipt](validation/completion-dialog-2.18-build.json). Runtime tests deferred. |
| 2.17 | Batch Download selects a supported clipboard URL, shows previews for sequences and keeps authorization controls in sync | [Native build receipt](validation/batch-dialog-2.17-build.json). Runtime tests deferred. |
| 2.16 | Add URL prefills and selects a supported clipboard URL and disables authorization labels with their fields | [Native build receipt](validation/add-url-2.16-build.json). Runtime tests deferred. |
| 2.15 | Floating drop target uses the captured dimensions and native caption controls and accepts native URL drags | [Native build receipt](validation/drop-target-2.15-build.json). Runtime tests deferred. |
| 2.05 (later build) | Download context menu follows the captured IDM order and groups UDM commands under More actions | [Native build receipt](validation/native-2.05-current-build.json). Runtime tests deferred. |
| 1.2.5 | Explorer .url registration repair and association refresh | Setup succeeded; registry and payload checks recorded |
| 1.2.6 | Synchronization detects same-size local changes using saved SHA-256 | Native build passed; runtime acceptance deferred |
| 1.2.7 | Unscheduled periodic synchronization stops at local midnight by default | Native build passed; schedule acceptance deferred |
| 1.2.8 | Synchronization replacement preparation hashes outside the catalog lock | Built and installed; runtime responsiveness deferred |
| 1.2.9 | Final replacement verification uses cancellable held-handle hashing outside the catalog lock | Built; cancellation/edit/rollback acceptance deferred |
| 1.2.10 | Live HLS preserves journal-dependent assembled output and prevents retry overwrite | Native build passed; recovery acceptance deferred |
| 1.2.11 | Broader interrupted-save output protection and retry guards | Built in 1.2.12; tests deferred |
| 1.2.12 | Restore publication of internal media child jobs after the 1.2.9 regression | Native build passed; tests deferred |
| 1.2.13 | Recover publication journals for saved private audio/video tracks | Native build passed; tests deferred |
| 1.2.14 | Resume recovers the selected download and private tracks without a global recovery pass | Native build passed; tests deferred |
| 1.2.15 | Roll back both replacement-history records when recovery catalog saving fails | Native build passed; tests deferred |
| 1.3.0 | Recorded MP4 WebVTT playback edits at positive whole-number rates through 16x | Native build passed; tests deferred |
| 1.4.0 | Finite MP4 WebVTT dwell edits holding active captions at a selected media time | Native build passed; tests deferred |
| 1.5.0 | Fractional forward MP4 WebVTT playback edits through 16x with bounded clock scaling | Native build passed; tests deferred |
| 1.6.0 | Finite reverse MP4 WebVTT playback edits with guarded media-distance mapping | Native build passed; tests deferred |
| 1.6.1 | Plain WebVTT cue-text timestamps follow HLS and DASH clock shifts and clipping | Native build passed; tests deferred |
| 1.6.2 | Recover selected downloads and private tracks before catalog removal or recycling | Native build passed; tests deferred |
| 1.6.3 | Recover before Move/Rename and preserve paths needed by pending replacements | Native build passed; tests deferred |
| 1.6.4 | Download Again commits inherited properties and the new entry together | Native build passed; tests deferred |
| 1.6.5 | Offline archive admission saves complete records and preserves enclosing transactions | Native build passed; tests deferred |
| 1.6.6 | Commit selected-download recovery before Site Grabber starts a batch | Native build passed; tests deferred |
| 1.6.7 | Retain form/media recapture requirements for completed catalog imports and Download Again | Native build passed; tests deferred |
| 1.6.8 | Compare full transfer context for catalog duplicates and recognize import-numbered copies | Native build passed; tests deferred |
| 1.7.0 / browser 0.63.83 | Support non-negative DASH duration-boundary overlaps with Period clipping and live capability gating | Browser syntax and native build passed; runtime tests deferred |
| 1.8.0 / browser 0.63.84 | Complete media transfer for DASH per-segment index templates | Browser syntax and native build passed; runtime tests deferred |
| 1.9.0 | Case-only filename renaming with spelling-aware recovery | Native build passed; runtime tests deferred |
| 1.9.1 | Verify current file contents during move, recovery and rollback | Native build passed; runtime tests deferred |
| 1.9.2 | File-identity verification avoids full reads for supported same-volume renames | Native build passed; runtime tests deferred |
| 1.10.0 | Relocation progress/cancellation backend; UI integration pending | Native build passed; runtime tests deferred |
| 1.11.0 | Background relocation dialog, reservations and shutdown coordination | Native build passed; tests deferred |
| 1.11.1 | Relocation verification/rollback byte progress and persistent destination display | Native build passed; tests deferred |
| 1.11.2 | Separate relocation from transfer scheduling | Native build passed; tests deferred |
| 1.11.3 | Recover selected moves before removal, recycling or redownload | Native build passed; tests deferred |
| 1.12.0 | Measured move speed, elapsed time and phase ETA | Native build passed; tests deferred |
| 1.12.1 | Completed-file control state and snapshot-safe path display during moves | Native build passed; tests deferred |
| 1.12.2 | Shared retained-file MP4 WebVTT decoder; live integration pending | Native build passed; tests deferred |
| 1.12.3 | Manifest-derived live track kind preflight and caption classification | Native build passed; tests deferred |
| 1.13.0 | Retained DASH caption decoding and capture-interval clipping | Native build passed; tests deferred |
| 1.13.1 | Retained caption muxing and joined-output stream validation | Native build passed; tests deferred |
| 1.14.0 | Native live caption admission and three-track recovery | Native build passed; tests deferred |
| 1.15.0 / browser 0.63.85 | Live DASH caption discovery, selection and capability-gated handoff | Native build passed; tests deferred |
| 1.15.1 | Validate live audio/caption selection metadata | Native build passed; tests deferred |
| 1.15.2 | Selected caption metadata in final recording output | Native build passed; tests deferred |
| 1.15.3 | Preserve MP4 WebVTT cue-current-time metadata | Native build passed; tests deferred |
| 1.16.0 | Forward MP4 WebVTT inline timing and capture/Period rebasing | Native build passed; tests deferred |
| 1.16.1 | Caption assembly revision invalidates stale Period/session outputs while retaining downloaded fragments | Native build passed; tests deferred |
| 1.17.0 | Source/output track and duration consistency checks before media confirmation | Native build passed; tests deferred |
| 1.18.0 | Direct tray queue submenus and bounded popup/large-catalog chooser | Native build passed; tests deferred |
| 1.19.0 | Session-wide live caption muxing across silent Periods and explicit empty-caption results | Native build passed; tests deferred |
| 1.20.0 | Immutable list snapshots move sorting, header decoding and painting outside the manager lock | Native build passed; tests deferred |
| 1.21.0 | Background Shell file-type icons with bounded viewport requests and generation-safe results | Native build passed; tests deferred |
| 1.22.0 | Reuse unchanged list ordering while preserving current catalog tie order | Native build passed; tests deferred |
| 1.23.0 | Windows logical filename ordering traced to the installed IDM reference | Native build passed; tests deferred |
| 1.24.0 | IDM optional-text empty ordering and bounded UTF-16 path/page comparisons | Native build passed; runtime comparisons deferred |
| 1.25.0 | IDM multibyte Description/Referer comparison keys with whole-sort error fallback | Native build passed; tests deferred |
| 1.26.0 | Reference-derived Status completion/progress ordering and HTTP 404 Not Found labels | Native build passed; tests deferred |
| 1.27.0 | Retained-progress Status display, reference float/truncation and cached Windows decimal separator | Native build passed; tests deferred |
| 1.28.0 | Safe long-duration ETA, list rate/display rules and reference Time Left completion ties | Native build passed; tests deferred |
| 1.29.0 | Duration-aware Size sort and guarded integer Transfer Rate display/sort/ETA keys | Native build passed; tests deferred |
| 1.30.0 | Rational inline MP4 caption offsets without whole-media-tick truncation; caption assembly revision 4 | Native build passed; tests deferred |
| 1.31.0 | Navigation snapshot lock boundary, in-place queue images and in-session tree context restoration | Native build passed; tests deferred |
| 1.32.0 | Batched toolbar/context availability, shorter selection/file-probe lock scopes and conditional settings reads | All five production targets built; versions/hashes verified; tests deferred |
| 1.33.0 | Single-pass queue menu availability, shared tray snapshot and independent submenu/child enabling | All five production targets built; versions/hashes verified; tests deferred |
| 1.34.0 | Reference-derived Find whole-site matching, uppercase folding and unlocked row matching | All five production targets built; versions/hashes verified; tests deferred |
| 1.35.0 | Saved Find text/options, startup restoration and targeted persistence rollback | All five production targets built; versions/hashes verified; tests deferred |
| 1.36.0 | Reference-derived Find Next cursor, wrap and list-rebuild integration | All five production targets built; versions/hashes verified; tests deferred |
| 1.37.0 | Early live-DASH whole-segment HTTP streaming and per-location framing admission | All five production targets built; versions/hashes verified; tests deferred |
| 1.38.0 | Strong-ETag DASH prefix checkpoints, conditional range resume and full-request fallback | All five production targets built; versions/hashes verified; tests deferred |
| 1.39.0 | Recover completed DASH staging across payload/receipt publication gaps | All five production targets built; versions/hashes verified; tests deferred |
| 1.40.0 | Legacy full-receipt scratch recovery and retry-safe promoted completion intents | All five production targets built; versions/hashes verified; tests deferred |
| 1.41.0 | Strong Last-Modified/Date DASH prefix resume, protected date evidence and exact validator/range matching | All five production targets built; versions/hashes verified; tests deferred |
| 1.42.0 | Background ZIP/recycle advisory file checks with selection prewarming and fresh execution validation | All five production targets built; versions/hashes verified; tests deferred |
| 1.43.0 | Background completed-ZIP reading, cancelable virtual-list dialog and held-file validation | All five production targets built; versions/hashes verified; tests deferred |
| 1.44.0 | Persisted sampled-rate fallback for stopped-download Time Left display and sorting | All five production targets built; versions/hashes verified; tests deferred |
| 1.45.0 | Reference-derived completed/unfinished removal confirmations and post-recovery approval checks | All five production targets built; versions/hashes verified; tests deferred |
| 1.46.0 | Missing-file continuation for combined remove/recycle and catalog-ownership rechecks | All five production targets built; versions/hashes verified; tests deferred |
| 1.47.0 | Verified primary/scratch DASH prefix recovery and pre-request checkpoint promotion | All five production targets built; versions/hashes verified; tests deferred |

| 1.48.0 | Owned background periodic catalog checkpoints with coalescing and shutdown join | All five production targets built; versions/hashes verified; tests deferred |

| 1.49.0 | Bounded parallel advisory file probes with partial current-generation answers | All five production targets built; versions/hashes verified; tests deferred |

| 1.50.0 | Reference-derived Find/list-filter coupling, shared matching options and invalid-field recovery | All five production targets built; versions/hashes verified; tests deferred |

| 1.51.0 | Full-width submitted search bar with Find/Settings/close, Enter and draft/committed state | All five production targets built; versions/hashes verified; tests deferred |

| 1.52.0 | Bounded UTF-16 Find submissions, empty-dialog retention and exact surrogate query preservation | All five production targets built; versions verified; tests deferred |

| 1.53.0 | Nonjoining remote ZIP preview with bounded workers and Manager-safe session persistence | All five production targets built; versions/hashes verified; tests deferred |

| 1.54.0 | File details can abandon slow lookups during Refresh, Start, and Cancel | [Native build receipt](validation/file-details-cancellation-20261104-build.json). Runtime tests deferred. |

| 1.55.0 | IDM category defaults and destinations for new profiles | [Native build receipt](validation/idm-category-defaults-20261104-build.json). Runtime tests deferred. |

| 2.13 | Captured dark menu colors, row spacing and label insets | [Native build receipt](validation/dark-menu-2.13-build.json). Runtime tests deferred. |
| 2.12 | Reference dark toolbar surface and large-button height target | [Native build receipt](validation/toolbar-frame-2.12-build.json). Runtime tests deferred. |
| 2.11 | Captured dark colors for the Categories pane and surrounding client area | [Native build receipt](validation/categories-palette-2.11-build.json). Runtime tests deferred. |
| 2.10 | Captured dark palette for the main download list, grid and header | [Native build receipt](validation/download-list-palette-2.10-build.json). Runtime tests deferred. |
| 2.09 | Captured visible download-list widths and hidden-column captions | [Native build receipt](validation/download-list-columns-2.09-build.json). Runtime tests deferred. |
| 2.08 | Reference-derived Categories frame, tree insets and list alignment | [Native build receipt](validation/categories-pane-2.08-build.json). Runtime tests deferred. |
| 2.07 | Reference queue column width across fresh and reset layouts | [Native build receipt](validation/queue-column-2.07-build.json). Runtime tests deferred. |
| 2.06 | Versioned main window title and reference-derived startup frame size | [Native build receipt](validation/desktop-frame-2.06-build.json). Runtime tests deferred. |
| 2.05 | Persist Pause and Resume decisions and show local save progress for captured browser responses | [Native build receipt](validation/native-2.05-20261105-build.json). Runtime tests deferred. |
| 1.105.0 | Historical build for captured browser response Pause and Resume | [Native build receipt](validation/captured-response-durable-pause-20261105-build.json). Runtime tests deferred. |
| 1.104.0 | Rebuild interrupted browser-capture staging copies from saved data | [Native build receipt](validation/captured-response-staging-reservation-20261105-build.json). Runtime tests deferred. |
| 1.103.0 | Choose a different save destination after a verified browser-capture conflict | [Native build receipt](validation/captured-response-destination-conflict-20261105-build.json). Runtime tests deferred. |
| 1.102.0 | Recover completed browser captures on a worker after desktop startup | [Native build receipt](validation/captured-response-startup-recovery-20261105-build.json). Runtime tests deferred. |
| 1.101.0 | Save live-captured browser responses on a worker with local Pause and Resume | [Native build receipt](validation/captured-response-background-save-20261105-build.json). Runtime tests deferred. |
| 1.100.0 | Run original browser-response virus checks off the desktop thread | [Native build receipt](validation/captured-response-background-scan-20261105-build.json). Runtime tests deferred. |
| 1.99.0 | Run pending virus checks after a recovered save and preserve existing results | [Native build receipt](validation/recovered-completion-scan-20261105-build.json). Runtime tests deferred. |
| 1.98.0 | Recover interrupted ordinary downloads when the final checksum was deferred | [Native build receipt](validation/ordinary-publication-recovery-20261105-build.json). Runtime tests deferred. |
| 1.97.0 | Retry publication of verified live DASH assemblies and show their exact size | [Native build receipt](validation/live-dash-fast-save-retry-20261105-build.json). Runtime tests deferred. |
| 1.96.0 | Protect the measured media assembly while Download File Info waits for Start | [Native build receipt](validation/prefetch-verified-file-20261105-build.json). Runtime tests deferred. |
| 1.95.0 | Retain verified media that finishes assembling after Start | [Native build receipt](validation/late-media-assembly-retry-20261105-build.json). Runtime tests deferred. |
| 1.94.0 | Retain fully prepared output after Start and show publication failures | [Native build receipt](validation/confirmed-media-save-retry-20261105-build.json). Runtime tests deferred. |
| 1.93.0 | Reconcile interrupted publication before editing download properties | [Native build receipt](validation/publication-recovery-before-file-edit-20261105-build.json). Runtime tests deferred. |
| 1.92.0 | Retry save controls for sealed live recordings and saved HLS output size | [Native build receipt](validation/live-retry-save-ui-20261105-build.json). Runtime tests deferred. |
| 1.91.0 | Reuse verified live HLS assembly after publication failure | [Native build receipt](validation/live-hls-assembly-reuse-20261105-build.json). Runtime tests deferred. |
| 1.90.0 | Commit live HLS/DASH completion details in the publication journal | [Native build receipt](validation/live-publication-metadata-20261105-build.json). Runtime tests deferred. |
| 1.89.0 | Save sealed live HLS and DASH recordings without dialing | [Native build receipt](validation/offline-live-recording-save-20261105-build.json). Runtime tests deferred. |
| 1.88.0 | Resume verified local HTTP and media data before attempting dial-up | [Native build receipt](validation/offline-retained-download-resume-20261105-build.json). Runtime tests deferred. |
| 1.87.0 | Show retained media size before and after re-verification | [Native build receipt](validation/saved-media-size-display-20261105-build.json). Runtime tests deferred. |
| 1.86.0 | Recover the final rename for confirmed retained media | [Native build receipt](validation/retained-media-rename-recovery-20261105-build.json). Runtime tests deferred. |
| 1.85.0 | Buffered cross-drive media copy and destination-handle verification | [Native build receipt](validation/media-cross-drive-verification-20261105-build.json). Runtime tests deferred. |
| 1.84.0 | Retain verified recorded HLS/DASH MP4, M4A and TS output through Download Later | [Native build receipt](validation/retained-adaptive-media-output-20261105-build.json). Runtime tests deferred. |
| 1.83.0 | Retain verified direct and SABR MP4/M4A output through Download Later | [Native build receipt](validation/retained-direct-media-output-20261105-build.json). Runtime tests deferred. |
| 1.82.0 | Save the Download Later choice before canceling prefetch | [Native build receipt](validation/prefetch-defer-transaction-20261105-build.json). Runtime tests deferred. |
| 1.81.0 | Save destination and folder history together when confirming prefetch | [Native build receipt](validation/prefetch-folder-transaction-20261105-build.json). Runtime tests deferred. |
| 1.80.0 | Signal prepared workers on confirmation and save adaptive completion with publication | [Native build receipt](validation/prefetch-publication-signal-20261105-build.json). Runtime tests deferred. |
| 1.79.0 | Direct progress notifications and prefetch byte count at Start | [Native build receipt](validation/prefetch-progress-handoff-20261105-build.json). Runtime tests deferred. |
| 1.78.0 | Accepted browser offers open ahead of legacy confirmation scans | [Native build receipt](validation/browser-presentation-priority-20261105-build.json). Runtime tests deferred. |
| 1.77.0 | Exact byte count and supported metadata check in compact Download File Info | [Native build receipt](validation/download-info-compact-exact-size-20261105-build.json). Runtime tests deferred. |
| 1.76.0 | Download File Info paints before loading Windows Shell file details | [Native build receipt](validation/download-info-async-shell-details-20261105-build.json). Runtime tests deferred. |
| 1.75.0 | Shared queue admission guard for downloads awaiting user or browser action | [Native build receipt](validation/queue-user-action-gate-20261105-build.json). Runtime tests deferred. |
| 1.74.0 | Reevaluate rearmed one-time schedules after saving edits | [Native build receipt](validation/scheduler-runonce-rearm-20261105-build.json). Runtime tests deferred. |
| 1.73.0 | Scheduled wakes account for server retry deadlines and queue windows | [Native build receipt](validation/queue-wake-retry-deadline-20261105-build.json). Runtime tests deferred. |
| 1.72.0 | Scheduled wake eligibility for synchronization replacements and retry review | [Native build receipt](validation/sync-wake-eligibility-20261105-build.json). Runtime tests deferred. |
| 1.71.0 | Startup recovery for interrupted synchronization checks and replacement transfers | [Native build receipt](validation/sync-interrupted-startup-20261105-build.json). Runtime tests deferred. |
| 1.70.0 | Atomic synchronization admission for replacement and missing-file restoration | [Native build receipt](validation/sync-atomic-admission-20261105-build.json). Runtime tests deferred. |
| 1.69.0 | Keep interrupted synchronization checks and replacements pending across recurring schedule windows | [Native build receipt](validation/sync-schedule-pause-20261104-build.json). Runtime tests deferred. |
| 1.68.0 | Preserve the longest HTTP server deadline across competing worker failures | [Native build receipt](validation/http-retained-retry-deadline-20261104-build.json). Runtime tests deferred. |
| 1.67.0 | Preserve HTTP Retry-After deadlines through range handoff and resource re-probe | [Native build receipt](validation/http-range-retry-deadline-20261104-build.json). Runtime tests deferred. |
| 1.66.0 | Live peer progress and response-scoped write reservations for HTTP range handoff | [Native build receipt](validation/http-active-range-peer-20261104-build.json). Runtime tests deferred. |
| 1.65.0 | HTTP split selection accounts for reserved disk writes and actual helper range size | [Native build receipt](validation/range-split-inflight-boundary-20261104-build.json). Runtime tests deferred. |
| 1.64.0 | Queue membership admission and safe cancellation alongside browser-owned transfers | [Native build receipt](validation/queue-membership-admission-20261104-build.json). Runtime tests deferred. |
| 1.63.0 | Reference concurrency defaults for built-in and custom queues | [Native build receipt](validation/idm-queue-concurrency-defaults-20261104-build.json). Runtime tests deferred. |
| 1.62.0 | Reference Scheduler file-list captions, widths, gridlines, and numeric edit | [Native build receipt](validation/idm-scheduler-file-columns-20261104-build.json). Runtime tests deferred. |
| 1.61.0 | Reference built-in queue names across desktop workflows | [Native build receipt](validation/idm-queue-labels-20261104-build.json). Runtime tests deferred. |
| 1.60.0 | State-aware queue context and Scheduler Start and Stop controls | [Native build receipt](validation/idm-queue-run-state-20261104-build.json). Runtime tests deferred. |
| 1.59.0 | Queue context menu, direct Scheduler tabs, and built-in queue deletion protection | [Native build receipt](validation/idm-queue-context-20261104-build.json). Runtime tests deferred. |
| 1.58.0 | Primary download queue toolbar routing and named queue context actions | [Native build receipt](validation/idm-toolbar-queue-routing-20261104-build.json). Runtime tests deferred. |
| 1.57.0 | Save To commits folder and checkbox changes immediately and displays the effective temporary directory | [Native build receipt](validation/idm-save-to-folders-20261104-build.json). Runtime tests deferred. |
| 1.56.0 | Reference Save To category labels, controls, and shared New/Edit workflow | [Native build receipt](validation/idm-save-to-categories-20261104-build.json). Runtime tests deferred. |

The complete prior history remains in CHANGELOG.md. Carry forward later increments here rather than recreating an installer after each change. Full parity, runtime regression, signed delivery and platform qualification remain outstanding; this ledger does not waive them.

