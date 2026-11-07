# Next major release preparation

The next major release number and publication date are not assigned. Advance the desktop version and both browser manifests together for each development increment. Per the 2026-10-30 instruction, build native binaries but do not generate setup packages during these increments; tests remain deferred. A build is not a release approval or proof of IDM parity.

Use at most two digits per version component. After 99, increment the component to its left and reset the trailing component to zero. For example, desktop 2.99 advances to 3.0 and browser 0.64.99 advances to 0.65.0.

Keep the cumulative increment count when changing the display format. The 2.69 audit restored 89 increments. A second audit restored four more browser increments and filled the gap in the desktop sequence after the dark-menu build. A third audit restored 42 more paired increments. A fourth restored 39 GUI deliveries, bringing the recovered total to 174. The [changelog](../CHANGELOG.md) shows accounted versions alongside the recorded numbers. The build evidence table below uses the recorded version for each build. [Counting policy](versioning.md)

## Current delivery baseline

- Installed: 4.17 with browser integration files 0.67.81. Verified 134 payload hashes, five native versions and the Windows display version after an in-place update from 4.13. Extension activation awaits browser reload. [Installation record](validation/install-4.17-local.json)
- Last historical installer: 1.2.9, browser 0.63.82; built but not installed or published.
- Current development versions are desktop 4.53 and browser 0.68.17 for Chromium and Firefox. All five production executables were staged and hash-verified.

## Changes to carry into the release

| Version | Change | Evidence/status |
|---|---|---|
| 4.53 / browser 0.68.17 | Keep the original HTTP response when an optional range probe fails | [Native build receipt](validation/http-helper-fallback-4.53-build.json). Runtime helper refusal, malformed response and resume checks remain deferred. |
| 4.52 / browser 0.68.16 | Refine the IDM-style browser strips and video menu | [Native build and visual preview receipt](validation/browser-panel-reference-4.52-build.json). Installed browser comparison remains pending. |
| 4.51 / browser 0.68.15 | Bound HTTP body reads at the current worker span | [Native build receipt](validation/http-range-read-cap-4.51-build.json). Runtime range races, timing and wire byte counts remain deferred. |
| 4.50 / browser 0.68.14 | Skip a redundant HTTP probe and wake workers when range ownership changes | [Native build receipt](validation/http-range-wake-4.50-build.json). Runtime timing, cancellation and resume comparisons remain deferred. |
| 4.49 / browser 0.68.13 | Scope the selected-link command to the clicked frame and verify fallback review | [Native build receipt](validation/selected-link-frame-4.49-build.json). 66 focused browser checks passed. Installed context menus and native runtime tests remain deferred. |
| 4.48 / browser 0.68.12 | Keep Chromium browser batches on one proxy settings revision | [Native build receipt](validation/browser-batch-proxy-4.48-build.json). 155 focused browser checks passed. Installed proxy changes and native runtime tests remain deferred. |
| 4.47 / browser 0.68.11 | Share signed-in batch tab and store lookups and reject stale preparation | [Native build receipt](validation/browser-batch-session-4.47-build.json). 75 focused browser checks passed. Installed handoff timing and native runtime tests remain deferred. |
| 4.46 / browser 0.68.10 | Move temporary request preparation off the browser batch dialog thread | [Native build receipt](validation/browser-batch-startup-4.46-build.json). Installed startup timing and native runtime tests remain deferred. |
| 4.45 / browser 0.68.9 | Keep browser batch previews in place when one request changes | [Native build receipt](validation/browser-batch-preview-refresh-4.45-build.json). Installed timing and native runtime tests remain deferred. |
| 4.44 / browser 0.68.8 | Use the File Properties layout for edits in browser batch review | [Native build receipt](validation/browser-batch-properties-4.44-build.json). Installed dialog comparison and native runtime tests remain deferred. |
| 4.43 / browser 0.68.7 | Add the resizable browser batch window and anchor its controls | [Native build receipt](validation/resizable-browser-batch-4.43-build.json). Installed resizing and native runtime tests remain deferred. |
| 4.42 / browser 0.68.6 | Use the reference browser batch layout and add checked links directly to the primary queue | [Native build receipt](validation/browser-batch-layout-4.42-build.json). Installed comparison and native runtime tests remain deferred. |
| 4.41 / browser 0.68.5 | Apply browser batch filters to the files queued | [Native build receipt](validation/browser-batch-filters-4.41-build.json). Installed comparison and native runtime testing remain deferred. |
| 4.40 / browser 0.68.4 | Place the selected-link strip at the selection endpoint | [Native build receipt](validation/selected-link-strip-4.40-build.json). The Chrome fixture passed 11 selected-link checks per bundle. Installed comparison and native runtime tests remain deferred. |
| 4.39 / browser 0.68.3 | Align the compact video menu and move its options to the strip context menu | [Native build receipt](validation/classic-video-menu-4.39-build.json). The local Chrome fixture passed 18 panel checks per bundle. Live IDM comparison and native runtime testing remain deferred. |
| 4.38 / browser 0.68.2 | Defer hidden desktop history and limit modal row updates | [Native build receipt](validation/defer-hidden-history-4.38-build.json). Source and manifest checks passed. Desktop startup, restore timing and native runtime tests remain deferred. |
| 4.37 / browser 0.68.1 | Cancel pending streaming format discovery when the menu closes | [Native build receipt](validation/cancel-format-discovery-4.37-build.json). 248 focused browser checks passed. Unpacked Chrome verified immediate playlist cancellation, reopening and preservation of a selected TS handoff. Native runtime tests deferred. |
| 4.36 / browser 0.68.0 | Display confirmed streaming qualities during discovery | [Native build receipt](validation/progressive-stream-formats-4.36-build.json). 284 focused browser checks passed. Unpacked Chrome verified early 720p display, stable offer keys and a scoped TS handoff with a simulated desktop reply. Native runtime tests deferred. |
| 4.35 / browser 0.67.99 | Keep verified video formats when other playlists miss the lookup deadline | [Native build receipt](validation/stream-format-deadline-4.35-build.json). 242 focused browser checks passed. Unpacked Chrome retained the confirmed 720p MP4 and TS choices when two playlists stalled. Native runtime tests deferred. |
| 4.34 / browser 0.67.98 | Show direct video files while the stream catalog loads | [Native build receipt](validation/progressive-site-formats-4.34-build.json). 255 focused browser checks passed. Unpacked Chrome verified early direct display, late HLS rows and a trusted early handoff with a simulated desktop reply. Native runtime tests deferred. |
| 4.33 / browser 0.67.97 | Reduce browser and DOM work when listing cross-site video formats | [Native build receipt](validation/site-menu-latency-4.33-build.json). 167 focused browser checks and six unpacked Chrome checks passed. Native runtime tests deferred. |
| 4.32 / browser 0.67.96 | Match the classic compact video menu and toolbar icon states | [Native build receipt](validation/idm-browser-panel-4.32-build.json). 144 focused browser checks and unpacked Chrome visual checks passed. Native runtime tests deferred. |
| 4.31 / browser 0.67.95 | Share settings and tab reads across each browser batch | [Native build receipt](validation/shared-batch-handoff-4.31-build.json). 145 focused browser checks and five unpacked Chrome checks passed. Native runtime tests deferred. |
| 4.30 / browser 0.67.94 | Reuse matching browser responses to fill batch details | [Native build receipt](validation/browser-batch-response-preview-4.30-build.json). 137 focused browser checks and four unpacked Chrome checks passed. Native runtime tests deferred. |
| 4.29 / browser 0.67.93 | Add file icons and formatted sizes and use the previewed name in browser batches | [Native build receipt](validation/browser-batch-file-details-4.29-build.json). Source and manifest checks passed. Runtime tests deferred. |
| 4.28 / browser 0.67.92 | Match the browser batch Save To controls and dialog layout | [Native build receipt](validation/browser-batch-save-to-4.28-build.json). Source and manifest checks passed. Runtime tests deferred. |
| 4.27 / browser 0.67.91 | Open native review from the browser batch menus and preview destinations | [Native build receipt](validation/native-context-link-review-4.27-build.json). 38 focused browser checks and five isolated Chrome checks passed. Native runtime tests deferred. |
| 4.26 / browser 0.67.90 | Open the Windows batch dialog from the selected-link strip | [Native build receipt](validation/native-selected-link-review-4.26-build.json). 48 focused browser checks and two unpacked Chrome smoke checks passed. Native runtime tests deferred. |
| 4.25 / browser 0.67.89 | Use classic download strips and open selected-link review in one click | [Native build receipt](validation/classic-browser-strips-4.25-build.json). 130 focused browser checks and an isolated Chrome extension smoke check passed. Native runtime tests deferred. |
| 4.24 / browser 0.67.88 | Warm the native video handoff channel and restore Browser Tools routing | [Native build receipt](validation/warm-format-control-and-browser-tools-4.24-build.json). 18 focused checks and an isolated Chrome check passed. Native runtime tests deferred. |
| 4.23 / browser 0.67.87 | Dismiss the quality menu immediately for a single video or audio handoff | [Native build receipt](validation/quiet-single-media-handoff-4.23-build.json). 43 focused browser checks passed. Native runtime tests deferred. |
| 4.22 / browser 0.67.86 | Recreate the compact browser video panel and per-tab toolbar toggle | [Native build receipt](validation/classic-extension-ui-4.22-build.json). Focused browser and toolbar checks passed. Native runtime tests deferred. |
| 4.21 / browser 0.67.85 | Refresh failed video sessions and retry background prefetch from File Info | [Native build receipt](validation/file-info-media-session-refresh-4.21-build.json). Both manifest versions verified. Runtime tests deferred. |
| 4.20 / browser 0.67.84 | Show publication progress for confirmed prepared videos and honor the keep-open setting | [Native build receipt](validation/prepared-media-progress-4.20-build.json). Both manifest versions verified. Runtime tests deferred. |
| 4.19 / browser 0.67.83 | Sample prefetch speed at confirmation and transfer completion | [Native build receipt](validation/prefetch-rate-boundary-samples-4.19-build.json). Both manifest versions verified. Runtime tests deferred. |
| 4.18 / browser 0.67.82 | Retry direct-file prefetch when a transfer slot frees up | [Native build receipt](validation/retry-direct-prefetch-slot-4.18-build.json). Both manifest versions verified. Runtime tests deferred. |
| 4.17 / browser 0.67.81 | Explain verified saved bytes in progress details and hide unused completion rows | [Native build receipt](validation/saved-prefetch-progress-rows-4.17-build.json). Both manifest versions verified. Runtime tests deferred. |
| 4.16 / browser 0.67.80 | Display measured prefetch peak separately from current rate after Start | [Native build receipt](validation/measured-prefetch-peak-4.16-build.json). Both manifest versions verified. Runtime tests deferred. |
| 4.15 / browser 0.67.79 | Retain verified media output when File Info closes as a pause | [Native build receipt](validation/retain-file-info-close-4.15-build.json). Both manifest versions verified. Runtime tests deferred. |
| 4.14 / browser 0.67.78 | Keep exact byte counts visible in compact Download File Info | [Native build receipt](validation/compact-file-info-byte-count-4.14-build.json). Both manifest versions verified. Runtime tests deferred. |
| 4.13 / browser 0.67.77 | Create and configure categories from Download File Info using the shared editor | [Native build receipt](validation/file-info-category-editor-4.13-build.json). Both manifest versions verified. Runtime tests deferred. |
| 4.12 / browser 0.67.76 | Recover the saved YouTube streaming format from the page opened by UDM | [Native build receipt](validation/youtube-stream-refresh-routing-4.12-build.json). JavaScript syntax checked. Runtime tests deferred. |
| 4.11 / browser 0.67.75 | Show YouTube cache verification progress and honor Pause during hashing | [Native build receipt](validation/sabr-saved-data-progress-4.11-build.json). Both manifest versions verified. Runtime tests deferred. |
| 4.10 / browser 0.67.74 | Reclaim known YouTube streaming files after explicit cancellation | [Native build receipt](validation/cancel-sabr-prefetch-cache-4.10-build.json). Both manifest versions verified. Runtime tests deferred. |
| 4.9 / browser 0.67.73 | Keep verified YouTube output eligible for reuse after an exact session refresh | [Native build receipt](validation/retain-sabr-output-refresh-4.9-build.json). Both manifest versions verified. Runtime tests deferred. |
| 4.8 / browser 0.67.72 | Rebuild complete retained YouTube segment caches offline after session expiry | [Native build receipt](validation/offline-sabr-cache-4.8-build.json). Both manifest versions verified. Runtime tests deferred. |
| 4.7 / browser 0.67.71 | Use shared YouTube playback session checks and reject expired sessions at dispatch | [Native build receipt](validation/youtube-session-expiry-4.7-build.json). JavaScript syntax checked. Runtime tests deferred. |
| 4.6 / browser 0.67.70 | Revalidate prepared YouTube stream identity and native-pair freshness at dispatch | [Native build receipt](validation/youtube-transport-dispatch-4.6-build.json). JavaScript syntax checked. Runtime tests deferred. |
| 4.5 / browser 0.67.69 | Restore 39 omitted GUI delivery increments and update historical accounting labels | [Native build receipt](validation/gui-historical-increments-4.5-build.json). Version accounting checked. Runtime tests deferred. |
| 3.65 / browser 0.67.29 | Revalidate the selected video and permissions after native startup or reconnect | [Native build receipt](validation/revalidate-media-after-connect-3.65-build.json). JavaScript syntax checked. Runtime tests deferred. |
| 3.64 / browser 0.67.28 | Check video selection deadlines at native dispatch and distinguish unsent receipts | [Native build receipt](validation/guard-video-native-dispatch-3.64-build.json). JavaScript syntax checked. Runtime tests deferred. |
| 3.63 / browser 0.67.27 | Share fresh desktop capabilities across direct YouTube audio request preparation and admission | [Native build receipt](validation/shared-direct-audio-capabilities-3.63-build.json). JavaScript syntax checked. Runtime tests deferred. |
| 3.62 / browser 0.67.26 | Read desktop capabilities during validated YouTube or adaptive video preparation | [Native build receipt](validation/parallel-desktop-media-capabilities-3.62-build.json). JavaScript syntax checked. Runtime tests deferred. |
| 3.61 / browser 0.67.25 | Reuse static host capabilities on the connection that negotiated them | [Native build receipt](validation/reuse-native-capabilities-3.61-build.json). JavaScript syntax checked. Runtime tests deferred. |
| 3.60 / browser 0.67.24 | Negotiate the native host connection during selected video preparation | [Native build receipt](validation/parallel-video-host-connect-3.60-build.json). JavaScript syntax checked. Runtime tests deferred. |
| 3.59 / browser 0.67.23 | Load video qualities independently of Site Grabber and avoid flashing progress text on quick handoffs | [Native build receipt](validation/video-popup-handoff-feedback-3.59-build.json). JavaScript syntax checked. Runtime tests deferred. |
| 3.58 / browser 0.67.22 | Reclaim recorded adaptive segment caches after a fresh browser offer is canceled | [Native build receipt](validation/cancelled-adaptive-cache-3.58-build.json). Runtime tests deferred. |
| 3.57 / browser 0.67.21 | Reclaim owned HTTP range files after a fresh browser offer is canceled | [Native build receipt](validation/cancelled-prefetch-ranges-3.57-build.json). Runtime tests deferred. |
| 3.56 / browser 0.67.20 | Reject a fresh browser offer on explicit File Info Cancel | [Native build receipt](validation/cancel-fresh-browser-offer-3.56-build.json). Runtime tests deferred. |
| 3.55 / browser 0.67.19 | Add selected downloads to an existing queue run without resetting its schedule or retries | [Native build receipt](validation/join-running-queue-3.55-build.json). Runtime tests deferred. |
| 3.54 / browser 0.67.18 | Apply Scheduler queue-start eligibility to desktop queue menus and the primary Start command | [Native build receipt](validation/desktop-queue-start-state-3.54-build.json). Runtime tests deferred. |
| 3.53 / browser 0.67.17 | Commit a batch with its selected files, new queue and prompt preference | [Native build receipt](validation/batch-queue-admission-3.53-build.json). Runtime tests deferred. |
| 3.52 / browser 0.67.16 | Create a new queue with its first Download Later admission | [Native build receipt](validation/download-later-create-queue-3.52-build.json). Runtime tests deferred. |
| 3.51 / browser 0.67.15 | Match the queue prompt caption and save its preference with Download Later | [Native build receipt](validation/download-later-queue-prompt-3.51-build.json). Runtime tests deferred. |
| 3.50 / browser 0.67.14 | Match File Info category labels and use the common category destination policy | [Native build receipt](validation/file-info-category-destination-3.50-build.json). Runtime tests deferred. |
| 3.49 / browser 0.67.13 | Queue the prepared continuation of an interrupted browser response with File Info edits | [Native build receipt](validation/interrupted-response-admission-3.49-build.json). Runtime tests deferred. |
| 3.48 / browser 0.67.12 | Save File Info edits with admission of complete browser responses for publication | [Native build receipt](validation/saved-response-start-3.48-build.json). Runtime tests deferred. |
| 3.47 / browser 0.67.11 | Save the individual Start decision with edited File Info after prefetch stops | [Native build receipt](validation/file-info-start-admission-3.47-build.json). Runtime tests deferred. |
| 3.46 / browser 0.67.10 | Save File Info edits, folder and login preferences, and Download Later queue enrollment together after prefetch stops | [Native build receipt](validation/atomic-file-info-edits-3.46-build.json). Runtime tests deferred. |
| 3.45 / browser 0.67.9 | Recover 42 omitted historical increments and advance both browser manifests | [Native build receipt](validation/historical-version-audit-3.45-build.json). Runtime tests deferred. |
| 3.2 / browser 0.66.66 | Remember an unchanged File Info login in the prefetch confirmation transaction | [Native build receipt](validation/remember-login-prefetch-3.2-build.json). Runtime tests deferred. |
| 3.1 / browser 0.66.65 | Refresh File Info against edited login details and hide stale size and filename | [Native build receipt](validation/file-info-login-preview-3.1-build.json). Runtime tests deferred. |
| 3.0 / browser 0.66.64 | Show the browser-observed server filename before desktop metadata arrives | [Native build receipt](validation/browser-server-filename-3.0-build.json). Runtime tests deferred. |
| 2.99 / browser 0.66.63 | Show a matching browser GET response size immediately in Download File Info | [Native build receipt](validation/browser-response-size-2.99-build.json). Runtime tests deferred. |
| 2.98 / browser 0.66.62 | Show a direct YouTube M4A's reported size immediately in Download File Info | [Native build receipt](validation/youtube-direct-audio-size-2.98-build.json). Runtime tests deferred. |
| 2.97 / browser 0.66.61 | Extend durable handoff recovery to direct YouTube M4A | [Native build receipt](validation/youtube-direct-audio-receipts-2.97-build.json). Runtime tests deferred. |
| 2.96 / browser 0.66.60 | Recover a previous YouTube handoff before reading playback URLs | [Native build receipt](validation/youtube-early-handoff-recovery-2.96-build.json). Runtime tests deferred. |
| 2.95 / browser 0.66.59 | Recover interrupted YouTube video and streaming audio handoffs by receipt | [Native build receipt](validation/youtube-handoff-receipts-2.95-build.json). Runtime tests deferred. |
| 2.94 / browser 0.66.58 | Wake the pending YouTube format check when verified playback becomes ready | [Native build receipt](validation/youtube-ready-notification-2.94-build.json). Runtime tests deferred. |
| 2.93 / browser 0.66.57 | Read ready YouTube sessions alongside format metadata and recheck the visit before handoff | [Native build receipt](validation/youtube-bundled-session-read-2.93-build.json). Runtime tests deferred. |
| 2.92 / browser 0.66.56 | Scope verified YouTube streaming readiness to the current visit | [Native build receipt](validation/youtube-stream-navigation-2.92-build.json). Runtime tests deferred. |
| 2.91 / browser 0.66.55 | Show HTTP prefetch verification separately from download progress and use the same display for FTP checks | [Native build receipt](validation/http-prefetch-check-progress-2.91-build.json). Runtime tests deferred. |
| 2.90 / browser 0.66.54 | Show replacement FTP verification progress separately from the download rate and ETA | [Native build receipt](validation/ftp-refresh-verification-progress-2.90-build.json). Runtime tests deferred. |
| 2.89 / browser 0.66.53 | Scope browser proxy and session state to the replacement request and isolate manual FTP refresh | [Native build receipt](validation/refresh-browser-request-context-2.89-build.json). Runtime tests deferred. |
| 2.88 / browser 0.66.52 | Accept replacement FTP links and verify retained bytes before resuming | [Native build receipt](validation/ftp-address-refresh-2.88-build.json). Runtime tests deferred. |
| 2.87 / browser 0.66.51 | Keep saved HTTP bytes protected through replacement-link range validation | [Native build receipt](validation/http-refresh-retain-validation-2.87-build.json). Runtime tests deferred. |
| 2.86 / browser 0.66.50 | Verify every saved prefetch range and checkpoint all damaged offsets before retry | [Native build receipt](validation/prefetch-all-range-repair-2.86-build.json). Runtime tests deferred. |
| 2.85 / browser 0.66.49 | Reuse the independently verified checksum of a complete single-range prefetch on Resume | [Native build receipt](validation/verified-prefetch-digest-reuse-2.85-build.json). Runtime tests deferred. |
| 2.84 / browser 0.66.48 | Separate direct-download prefetch receipt work and confirmation wait from network and content timing | [Native build receipt](validation/direct-prefetch-finish-timing-2.84-build.json). Runtime tests deferred. |
| 2.83 / browser 0.66.47 | Stop optional direct-download prefetch receipt hashing after Start | [Native build receipt](validation/prefetch-start-during-receipt-hash-2.83-build.json). Runtime tests deferred. |
| 2.82 / browser 0.66.46 | Reconcile omitted historical increments and separate the two builds recorded as 2.05 | [Native build receipt](validation/version-history-reconciliation-2.82-build.json). Runtime tests deferred. |
| 2.80 / browser 0.66.41 | Reuse the running hash for single-range HTTP prefetch receipts and publication | [Native build receipt](validation/http-prefetch-running-hash-2.80-build.json). Runtime tests deferred. |
| 2.79 / browser 0.66.40 | Preserve valid empty HTTP prefetches through Download Later | [Native build receipt](validation/http-empty-prefetch-receipt-2.79-build.json). Runtime tests deferred. |
| 2.78 / browser 0.66.39 | Measure HTTP response size at clean EOF when no length header was supplied | [Native build receipt](validation/http-prefetch-eof-size-2.78-build.json). Runtime tests deferred. |
| 2.77 / browser 0.66.38 | Reuse the source metadata inspection while independently verifying the preserved MP4 | [Native build receipt](validation/muxed-mp4-probe-reuse-2.77-build.json). Runtime tests deferred. |
| 2.76 / browser 0.66.37 | Stage verified combined MP4s with a hard link where supported | [Native build receipt](validation/muxed-mp4-staging-link-2.76-build.json). Runtime tests deferred. |
| 2.75 / browser 0.66.36 | Preserve the original bytes of eligible combined YouTube MP4 files | [Native build receipt](validation/preserve-muxed-mp4-2.75-build.json). Runtime tests deferred. |
| 2.74 / browser 0.66.35 | Preserve browser proxy and request context for direct YouTube audio | [Native build receipt](validation/youtube-audio-request-context-2.74-build.json). JavaScript syntax checked. Runtime tests deferred. |
| 2.73 / browser 0.66.34 | Bound YouTube audio and video preparation so expired selections cannot submit downloads | [Native build receipt](validation/youtube-handoff-deadline-2.73-build.json). JavaScript syntax checked. Runtime tests deferred. |
| 2.72 / browser 0.66.33 | Recheck direct video selection and permissions after browser handoff preparation | [Native build receipt](validation/direct-dispatch-revalidation-2.72-build.json). JavaScript syntax checked. Runtime tests deferred. |
| 2.71 / browser 0.66.32 | Validate video and cookie permissions at adaptive handoff after receipt preparation | [Native build receipt](validation/adaptive-dispatch-revalidation-2.71-build.json). JavaScript syntax checked. Runtime tests deferred. |
| 2.70 / browser 0.66.31 | Reuse the live DASH capability check within the selected download | [Native build receipt](validation/dash-live-capability-reuse-2.70-build.json). JavaScript syntax checked. Runtime tests deferred. |
| 2.69 / browser 0.66.30 | Restore 89 omitted version increments and reconcile historical changelog headings | [Native build receipt](validation/historical-browser-increments-2.69-build.json). Both manifests verified. |
| 2.68 / browser 0.65.40 | Fetch HLS subtitles alongside video and audio before validating the download plan | [Native build receipt](validation/parallel-hls-subtitles-2.68-build.json). JavaScript syntax checked. Runtime tests deferred. |
| 2.67 / browser 0.65.39 | Fetch selected HLS video and audio playlists concurrently before desktop handoff | [Native build receipt](validation/parallel-hls-playlists-2.67-build.json). JavaScript syntax checked. Runtime tests deferred. |
| 2.66 / browser 0.65.38 | Preserve the full increment count since browser version 0.63.0 under the two-digit rule | [Native build receipt](validation/paired-version-count-2.66-build.json). Both manifests verified. |
| 2.65 / browser 0.64.0 | Roll over both browser versions and document the two-digit version component rule | [Native build receipt](validation/paired-version-rollover-2.65-build.json). Both manifests verified. |
| 2.64 / browser 0.63.236 | Reconnect after a pre-dispatch disconnect without repeating a submitted download | [Native build receipt](validation/browser-reconnect-before-dispatch-2.64-build.json). Both bridge scripts and manifests verified. Runtime tests deferred. |
| 2.63 / browser 0.63.235 | Give browser duplicate and capture-exception prompts their own taskbar entries | [Native build receipt](validation/browser-capture-prompt-owner-2.63-build.json). Both manifests verified. Runtime tests deferred. |
| 2.62 / browser 0.63.234 | Use an independent taskbar entry and hidden owner for browser-triggered replacement link dialogs | [Native build receipt](validation/browser-refresh-dialog-owner-2.62-build.json). Both manifests verified. Runtime tests deferred. |
| 2.61 / browser 0.63.233 | Present saved browser captures before list refresh and retry notifications received during another dialog | [Native build receipt](validation/browser-capture-priority-2.61-build.json). Both manifests verified. Runtime tests deferred. |
| 2.60 / browser 0.63.232 | Retry media prefetch when all transfer slots are occupied and show the waiting state in File Info | [Native build receipt](validation/file-info-prefetch-capacity-2.60-build.json). Both manifests verified. Runtime tests deferred. |
| 2.59 / browser 0.63.231 | Label combined media input lengths as selected track size until the final output size is available | [Native build receipt](validation/file-info-media-track-size-2.59-build.json). Both manifests verified. Runtime tests deferred. |
| 2.58 / browser 0.63.230 | Show the exact byte count for known direct responses before Start | [Native build receipt](validation/file-info-direct-byte-size-2.58-build.json). Both manifests verified. Runtime tests deferred. |
| 2.57 / browser 0.63.229 | Synchronize the parent download counters with its media tracks before presenting progress on Start | [Native build receipt](validation/prefetch-track-progress-2.57-build.json). Both manifests verified. Runtime tests deferred. |
| 2.56 / browser 0.63.228 | Close File Info after saving Download Later and preserve queue admission while prefetch stops | [Native build receipt](validation/prefetch-download-later-2.56-build.json). Both manifests verified. Runtime tests deferred. |
| 2.55 / browser 0.63.227 | Save prefetch cancellation after the worker stops and preserve the confirmation decision if that save fails | [Native build receipt](validation/prefetch-cancel-settlement-2.55-build.json). Both manifests verified. Runtime tests deferred. |
| 2.54 / browser 0.63.226 | Report a failed background transfer in File Info and retry size discovery for eligible direct files | [Native build receipt](validation/file-info-prefetch-fallback-2.54-build.json). Both manifests verified. Runtime tests deferred. |
| 2.53 / browser 0.63.225 | Retry one mismatched FTP resume range after checking the original file size and modification time | [Native build receipt](validation/ftp-single-range-repair-2.53-build.json). Both manifests verified. Runtime tests deferred. |
| 2.52 / browser 0.63.224 | Check overlap between adjacent FTP ranges before publication | [Native build receipt](validation/ftp-adjacent-overlap-2.52-build.json). Both manifests verified. Runtime tests deferred. |
| 2.51 / browser 0.63.223 | Reconcile the two browser manifests with the 138 recorded increments since 1.15.0 and advance the desktop version | [Native build receipt](validation/paired-browser-versions-2.51-build.json). Both manifest versions verified. |
| 2.50 | Retry one mismatched HTTP resume range without resetting other ranges when the file identity still matches | [Native build receipt](validation/http-single-range-repair-2.50-build.json). Runtime tests deferred. |
| 2.49 | Verify overlap between adjacent HTTP ranges before publication, including ranges retained across pause | [Native build receipt](validation/http-adjacent-overlap-2.49-build.json). Runtime tests deferred. |
| 2.48 | Verify HTTP resume overlap and restart ordinary mismatches on a single connection | [Native build receipt](validation/http-resume-overlap-2.48-build.json). Runtime tests deferred. |
| 2.47 | Compare up to 64 saved bytes before appending to a resumed FTP part | [Native build receipt](validation/ftp-resume-overlap-2.47-build.json). Runtime tests deferred. |
| 2.46 | Fall back from an unreachable FTP EPSV data port to PASV on an eligible route | [Native build receipt](validation/ftp-passive-data-fallback-2.46-build.json). Runtime tests deferred. |
| 2.45 | Inspect and assemble FTP downloads from their own numbered parts when a stale HTTP partial exists | [Native build receipt](validation/ftp-part-publication-2.45-build.json). Runtime tests deferred. |
| 2.44 | Commit zero-progress HTTP range resets before deleting old partials and recover interrupted cleanup on retry | [Native build receipt](validation/http-transactional-range-reset-2.44-build.json). Runtime tests deferred. |
| 2.43 | Keep legacy HTTP partial files until the migrated bytes and resume offsets are saved | [Native build receipt](validation/http-legacy-part-migration-2.43-build.json). Runtime tests deferred. |
| 2.42 | Roll back HTTP resume offsets to the first missing allocation in a sparse partial file | [Native build receipt](validation/http-sparse-resume-2.42-build.json). Runtime tests deferred. |
| 2.41 | Checkpoint resume offsets on a background timer independent of desktop ticks | [Native build receipt](validation/independent-checkpoints-2.41-build.json). Runtime tests deferred. |
| 2.40 | Stop speculative reads at HTTP range boundaries and finish fully consumed responses before releasing them to the connection pool | [Native build receipt](validation/http-clean-response-reuse-2.40-build.json). Runtime tests deferred. |
| 2.39 | Overlap the first HTTP body with verification of unadvertised ranges while withholding resume until validation | [Native build receipt](validation/http-unadvertised-midpoint-2.39-build.json). Runtime tests deferred. |
| 2.38 | Let HTTP workers split the live remainder when a late validated helper overlaps the primary worker's progress | [Native build receipt](validation/http-late-midpoint-2.38-build.json). Runtime tests deferred. |
| 2.37 | Read the first HTTP body while validating the midpoint helper on servers advertising ranges with a usable validator | [Native build receipt](validation/http-overlap-midpoint-2.37-build.json). Runtime tests deferred. |
| 2.36 | Start fresh HTTP transfers with a validated midpoint helper and split ranges as other workers join | [Native build receipt](validation/http-dynamic-start-2.36-build.json). Runtime tests deferred. |
| 2.35 | Bisect the largest eligible HTTP span after reserving bytes the original worker is writing | [Native build receipt](validation/http-largest-range-split-2.35-build.json). Runtime tests deferred. |
| 2.34 | Let a finished HTTP worker take the next unstarted, unassigned adjacent range before splitting an active range | [Native build receipt](validation/http-adjacent-range-2.34-build.json). Runtime tests deferred. |
| 2.33 | Honor Start view for automatic browser-video progress and default new queue settings to the system tray | [Native build receipt](validation/progress-start-view-2.33-build.json). Runtime tests deferred. |
| 2.32 | Make the completion dialog and per-download modem, exit and computer power actions mutually exclusive | [IDM observation](idm-workflow.md), [native build receipt](validation/progress-completion-options-2.32-build.json). Runtime tests deferred. |
| 2.31 | Match the Speed Limiter numeric field and disabled labels, and skip redundant remembered-rate saves | [Native build receipt](validation/progress-limiter-controls-2.31-build.json). Runtime tests deferred. |
| 2.30 | Match the three-column connection table, zero-based numbering and grouped whole-KB counts | [Native build receipt](validation/progress-connection-table-2.30-build.json). Runtime tests deferred. |
| 2.29 | Match the published IDM segment chart colors and mark the latest accepted HTTP and FTP request offsets | [Native build receipt](validation/progress-range-map-2.29-build.json). Runtime tests deferred. |
| 2.28 | Show Resume for paused or failed downloads with saved resumable progress and Pausing… during the stop transition | [Native build receipt](validation/progress-resume-action-2.28-build.json). Runtime tests deferred. |
| 2.27 | Update the download segment chart and connection rows as their values change and refresh expanded details immediately | [Native build receipt](validation/progress-redraw-2.27-build.json). Runtime tests deferred. |
| 2.26 | Reuse an eligible full response after a parallel HTTP range restart and save the reset before recreating the partial file | [Native build receipt](validation/parallel-range-restart-2.26-build.json). Runtime tests deferred. |
| 2.25 | Reuse an eligible full HTTP response when a single-connection resume must restart | [Native build receipt](validation/full-resume-restart-2.25-build.json). Runtime tests deferred. |
| 2.24 | Keep an accepted full response running during a long helper Retry-After wait and require review before opening another request | [Native build receipt](validation/helper-long-retry-2.24-build.json). Runtime tests deferred. |
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

