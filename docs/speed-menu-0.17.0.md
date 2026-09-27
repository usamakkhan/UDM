# UDM 0.17.0 — transfer measurements, video formats and download dialogs

Implemented and tested on 24 September 2026 Pacific time.

## Measured comparison

[Speed graph](../benchmarks/speed-menu-20260924/speed-comparison.png), [machine-readable comparison](../benchmarks/speed-menu-20260924/comparison.json), [IDM CSV](../benchmarks/speed-menu-20260924/idm-speed.csv), [UDM CSV](../benchmarks/speed-menu-20260924/udm-speed.csv).

Both applications downloaded the identical public Ubuntu 24.04.4 desktop ISO URL with eight connections, sequentially. IDM 6.43 build 10 averaged **4.203 MiB/s / 504.34 MiB**; installed UDM 0.16.1 averaged **4.319 MiB/s / 518.27 MiB**, over 120 measured seconds each. The graph uses the same five-second payload-byte differences for both, rather than comparing the applications' differently calculated UI rates. IDM had 14 decreasing intervals; UDM had 13. Actual throughput fluctuated in both.

This is **not a controlled cold-start result or a universal speed-win claim**. IDM prefetched while File Info was open. The UDM background logger failed to launch and was restarted directly after the transfer had begun. The graph therefore subtracts each window's starting byte count and compares two in-progress windows. IDM connected to 91.189.91.107; UDM to 91.189.91.108. Order, network variation and the applications' different counter-persistence cadences remain limitations. UDM's 2.76% difference is too small and confounded to establish superiority. No post-update ISO speed increase is claimed. The host file and license state were not changed.

IDM was paused and its partial ISO retained. The isolated UDM test process was terminated after logging, with its isolated state and partial files retained and marked Paused. The normal UDM history was never used for this benchmark.

## Changes

- **Honest, steadier transfer rate:** a time-weighted five-second window, based on actual elapsed time and network bytes. Resume excludes retained bytes; retries do not create negative speed. A full five-second stall reaches zero. Peak speed still records the short-interval measurement. This changes presentation, not physical bandwidth.
- **Reusable adaptive HTTP session:** HLS/DASH workers now share one per-download WinHTTP session/connection pool, allowing connections to be reused across segments. Headers/cookies retain their existing origin scoping. Segment hashing is moved outside the shared manager lock before recording completion. No new percentage speed-up is claimed for this change.
- **Compact video menu:** platform, output container, actual resolution and declared playlist bitrate. Dailymotion no longer repeats the opaque playlist filename on every row. Generic sources retain identifying labels where association can be ambiguous. The list scrolls independently; header/footer controls stay visible and horizontal position is clamped to the viewport.
- **Real MP4 / TS choices:** compatible HLS codec declarations expose both containers. TS is actually muxed as MPEG-TS, with final quality/audio checks and SHA-256 validation. Unknown/incompatible codecs do not advertise TS. MP4 remains the default for existing plans. DRM/live restrictions and document/video identity checks remain enforced.
- **Download all:** sends each current format offer once, stops on failure, reports the accepted count, and prevents overlapping clicks. Every offer is revalidated by the background service before native handoff.
- **Download dialog:** progress and action buttons above collapsible connection details; state-aware Pause/Resume/Open, Cancel, Hide, Refresh address, Open folder and Open with. Opening an existing minimized progress window restores it. Completion avoids duplicate progress/completion windows. Cancel pauses and preserves data; Hide keeps the transfer running.

## Verification

- **251 native checks passed**, including seven new speed-window checks and three TS container/decoding checks.
- **107 browser checks passed:** 12 Dailymotion association/format checks, 31 cross-site parser/capture checks, 30 browser integration checks, one preparation check, seven real pointer control checks, 16 placement checks, four compact-menu/bulk-failure checks, and six real Chrome-to-native end-to-end checks.
- Actual isolated Chrome/native downloads verified byte-identical direct MP4, playable recorded HLS with audio, automatic file handoff, and document-navigation isolation.
- Live Dailymotion xba9f3y recovered its current playlist with the capture history deliberately empty, and displayed MP4 and TS at the declared 288p / 461 kbps offered to this fresh profile. The eight-row screenshot is a fixture representing the user's 1080/720/480/288 list, not a fabricated live catalog.
- The installed app was opened and its paused-download dialog, collapsed details and Hide behavior inspected. Existing records were checked by ID, filename, status and received bytes against the pre-change snapshot.
- **15 existing records retained**; the two existing paused ISO counts are still 546,308,096 and 571,867,136 bytes.

Installed binaries: `D:\UDM\release`. Source: `D:\UDM\native` and `D:\UDM\browser`. Rollback snapshot: `D:\UDM\backups\speed-menu-20260924`. Test data and graph: `D:\UDM\benchmarks\speed-menu-20260924`.

This focused update does not establish complete IDM feature parity or compatibility with every video platform. The earlier feature-gap audit remains applicable.

Additional live validation: the current Dailymotion video was downloaded through an actual trusted TS menu click and real native messaging. The MPEG-TS output is 45,157,036 bytes with 288p H.264 and AAC audio; full decoding passed. Native network time was 13.709 seconds and merge/validation time 0.752 seconds. This is a functional test, not a paired speed claim. Evidence: benchmarks/speed-menu-20260924/live-ts/download-result.json.

Final desktop check: Minimize was reproduced as nonfunctional when its style was added after dialog creation. Progress now uses its own native dialog resource with WS_MINIMIZEBOX set at creation. The installed title-bar button was clicked successfully; double-clicking the same paused download restored the existing progress window. No transfer was resumed during these checks.

Installed Chrome verification: reloaded the existing unpacked extension and confirmed version 0.17.0, refreshed the user's Dailymotion page, and opened the new menu. It displayed eight actual choices: MP4 and TS at 1080p (6222 kbps), 720p (2149 kbps), 480p (836 kbps), and 288p (461 kbps). These values use the playlist's declared bandwidth in decimal kilobits/s. Download all and the fixed Refresh / Reset position / Hide here footer were visible. No new site permissions were requested or changed.

The installed Chrome Refresh button was clicked and replaced the catalog with eight freshly generated choices. Reset position displayed its confirmation. Final history recheck preserved all 15 prior download records and their received byte counts. UDM is running and the updated Chrome video menu is left open.
