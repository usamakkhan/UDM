# Next major release preparation

The next major release number and publication date are not assigned. Development versions continue to increase for each functional improvement. Per the 2026-10-30 instruction, build native binaries but do not generate setup packages during these increments; tests remain deferred. A build is not a release approval or proof of IDM parity.

## Current delivery baseline

- Installed: 1.6.0, browser 0.63.82; four native executable hashes and Windows display version verified after the 2026-10-31 direct update. Existing integration paths were retained. See [installation record](validation/install-1.6.0-20261031.json).
- Last historical installer: 1.2.9, browser 0.63.82; built but not installed or published.
- Current native development version: 1.30.0; browser source 0.63.85; production build in progress.

## Changes to carry into the release

| Version | Change | Evidence/status |
|---|---|---|
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
| 1.30.0 | Rational inline MP4 caption offsets without whole-media-tick truncation; caption assembly revision 4 | Native build in progress; tests deferred |

The complete prior history remains in CHANGELOG.md. Carry forward later increments here rather than recreating an installer after each change. Full parity, runtime regression, signed delivery and platform qualification remain outstanding; this ledger does not waive them.

