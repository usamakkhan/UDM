# Desktop workflow source recheck

Historical GUI audits contained stale missing-feature statements. Current source was read before choosing another desktop change; no installed app was launched or altered.

| Item | Source evidence | Remaining work |
|---|---|---|
| 3D toolbar style | ToolbarUi.hpp saves and applies ToolbarLook; ToolbarImages.hpp renders raised tiles | Installed visual and interaction comparison |
| Toolbar discovery menu | ToolbarUi.hpp opens the local Toolbar folder | This does not establish equivalent online discovery; no gallery was added |
| Queue sounds | OptionsPages.hpp lists queue events; Queue.cpp invokes queueSound; App.cpp supplies playback | Installed playback and transition acceptance |
| Scheduler time fields | SchedulerUi.hpp uses SchedulerTime with DTS_TIMEFORMAT, DTS_UPDOWN and h:mm:ss tt | DPI, keyboard and installed scheduling comparison |
| Media length in Size | StreamProgress.hpp::sizeCellText uses saved duration when byte size is unknown | Unknown duration and live recordings remain blank; installed comparison pending |

The GUI audit and October 26 comparison now distinguish these source paths from deferred qualification. Older screenshots and test records were not relabeled as current evidence. No app source changed, and no build, runtime test, installation or publication was performed for this review. The successful dash-finalized-drain candidate remains current. Source hashes are in [the review record](validation/desktop-source-recheck-20261029.json).

Remaining source gaps include segmented DASH captions, incomplete low-latency chunk transfer, template representation indexes and unsupported boundary adjustments. Signing, delivery, platform support and runtime qualification remain separate. This review does not establish complete IDM parity.
