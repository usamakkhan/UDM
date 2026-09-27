# IDM and UDM comparison — native 0.26.0 / extension 0.23.3

This release narrows the verified interface and behavioral gaps. It is not a claim that every IDM feature, internal algorithm, browser, or video platform is now identical.

## Evidence and scope

- Reference: installed IDM 6.43 build 10, its visible main/progress/completion-option dialogs, earlier captured Properties/File Info screens, and the existing dialog/menu resource inventory. Resource inspection describes controls and placement; it does not establish IDM's source code or exact transport algorithms.
- UDM: current C++/MFC sources, Chromium/Firefox extension sources, local transfer fixtures, and the two-minute same-file stopwatch comparison.
- No IDM executable code, images, icons, license checks, or installer components were incorporated into UDM.

## Front end

| Area | Difference verified | Change in this release |
|---|---|---|
| File Info | UDM exposed recovery/authentication/queue fields in a large initial dialog | Compact URL, category, save path, description, file icon and size; Download Later / Start Download / Cancel in one row. More exposes parent page, queue and credentials. Existing Basic authentication opens these fields automatically. |
| Progress | Wide dialog; rate and remaining time shared a row; extra buttons occupied a second row | Compact vertical statistics, URL at the top, separate file size, three-decimal byte/rate display, readable hour/minute/second ETA, progress bar, details map and connection list. Pause/Start and Cancel stay beside the details controls. |
| Progress actions | Recovery/folder/open-with took permanent space | Actions menu retains Refresh address, Properties, Copy address, Open folder, Open with, and Hide. Minimize is available in the title bar. |
| Completion options | No dedicated progress tab | Options on completion controls the completion dialog, closing the progress window, and opening the saved folder. Choices are per file and saved; folder opening is off by default. Global suppression still takes precedence. |
| Properties | Excessive vertical spacing | Compact layout retaining size/status, editable address and description, parent page, referer, login/password, advanced options, Move/Rename and Open. |
| Completion | Oversized spacing and buttons | Compact file/size/path/rate display and Open with / Open / Open folder / Close actions; drag icon retained. |
| Window bounds | Expansion could place controls outside the monitor work area | Dialogs clamp their origin to the monitor work area after creation/resizing. Mixed-DPI and very small screens still need separate validation. |
| Main window | Minimum width substantially larger than the reference | Minimum width reduced from 754 to 596 logical pixels; existing toolbar wrapping retained. |
| Dark mode | Unreadable checkbox text, white toolbar/buttons/title bars and progress-tab background | Readable checkbox text, original drawn buttons, themed title bars, dark toolbar, tabs and download-list headers. Standard menus, some combo boxes and scrollbars retain Windows styling. |
| Video menu | Large heading and secondary source line on each choice | Compact numbered single-line choices, Download all at top, controls in the video bar, source detail in tooltips, dark/light menu colors, and a 206-pixel bar that fits its caption. Only actual offered variants are shown. |

## Back end

| Area | Verified UDM behavior / difference | Outcome |
|---|---|---|
| Scheduling behind a dialog | A modal File Info opened inside the refresh loop prevented subsequent queued jobs from starting | Scheduler ticking moved out of the UI refresh guard. An isolated live test completed another byte-identical file while File Info remained awaiting confirmation. |
| State persistence | A short reader lock could cause an atomic checkpoint warning | Bounded retry for Windows sharing/lock errors, with the error code retained for other failures. Tests cover permanent contention, later recovery, brief contention and preservation of the prior checkpoint backup. |
| Transport | UDM uses WinHTTP with bounded parallel range workers, validation, dynamic splitting and persistent connections | Existing engine retained and regression-tested. IDM's exact implementation is not established by the executable/resource evidence. |
| Speed measurement | The earlier two-minute pair showed 396.422 MiB for IDM and 398.656 MiB for UDM | Difference is 0.56%, one pair, with different IPv4/IPv6 routes. This does not establish a general winner or identical performance. No invented acceleration was added. |
| Video | Direct media, recorded HLS/DASH, and captured YouTube transports have dedicated workflows | Existing capture/format selection retained. This release changes presentation, not the set of supported sites or encrypted/ciphered formats. |
| Drivers | UDM's WFP component is an inspection/diagnostic component | No new driver installed. It is not the download engine, and its presence is not evidence of IDM-equivalent capture or throughput. |

The checkpoint implementation uses the documented replacement semantics of [Microsoft ReplaceFileW](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-replacefilew). Only sharing and lock errors receive the bounded retry; other failures are surfaced rather than reported as successful saves.

## Validation

- Native suite: **456 passed, 0 failed** on the staged 0.26.0 core.
- Native launch tests: **6 passed**, including queued work with a modal confirmation open and byte-for-byte validation.
- Real native-host framing/protocol tests: **7 passed**.
- Browser component suites: **54 browser**, **12 Dailymotion recovery**, **20 integration parity**, and **7 integration policy** checks passed.
- Isolated Edge UI: **4 menu**, **12 controls**, **19 geometry**, and **16 lifecycle** checks passed, plus **1 package identity/source parity** check.
- Follow-up reload hardening: **18 lifecycle** checks passed, including two new cases where the runtime API disappears during owner replacement. Both video and selected-link panels abort cleanly and can be injected again.
- Installed Edge extension/native app: **13 end-to-end checks passed**, including authenticated transfers, MP4 byte identity, HLS video/audio assembly, capture shortcuts, and exclusion of stale iframe playlists.
- Native visual review: File Info, More/credentials, keyboard Start, actual eight-worker local transfer, completion, dark styling. Final installed-app verification is recorded separately.

## Remaining gaps

Full parity is still unproven. Remaining differences include arbitrary browser POST-download replay, comprehensive video-site coverage, unsupported ciphered/DRM media, offline website mirroring, SOCKS/dial-up workflows, languages/skins, signed public distribution, and some Windows-standard control styling. IDM-specific per-download power/exit controls are not duplicated in the progress tab; UDM's queue completion actions remain available in Scheduler. Multi-monitor/mixed-DPI behavior and screen-reader behavior need broader testing.

Original UDM branding is retained. Native sources and binaries are version 0.26.0; Chromium and Firefox sources are version 0.23.3. Backups, logs and the hash-checked deployment manifest are stored beside this report on C:.

Installation preserved all 23 existing download records. On restart, the existing state loader normalized three missing `ConfirmationPending` fields to `false`; addresses, paths, saved bytes, credentials, statuses and record IDs remained unchanged. Dark content theme was enabled to match the reference appearance.
