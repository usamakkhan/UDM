# Scheduler theme consistency — 8 October 2026

The scheduler still used a plain tab control and list header, while shared Form theme refresh did not update tree colors. The previous scheduler action images and source showed those light surfaces in dark mode. Scheduler now uses ThemeTabs and ThemeList, refreshes its colors after initialization, and Form refresh updates tree foreground/background colors. The existing light-mode tab style is explicitly retained.

The fixture opens scheduler once in light mode and once in dark mode. It checks initial tree/list colors, dark tab/header print pixels, selected queue and tab through light/dark transitions, compact action painters, move-up/down ordering, removal of queue membership while retaining download records, and absence of started downloads. Existing open-dialog/progress tests also run. Final source passed 119 checks. Light and dark images were inspected; after the first pass, light tab styling was restored and all 119 checks reran successfully.

Shared tree refresh also applies when another Form owns a tree, including Grabber. This turn does not qualify all Grabber interactions. Native spin buttons, title bars and other system-rendered details remain platform-controlled; no claim of complete dark-mode or high-contrast accessibility parity is made.

Application and test entry points were rebuilt using unchanged backend objects. Not a full clean build; existing compiler warnings remain. Raw scripts, images and logs are in `C:\Users\Abuzar\AppData\Local\Temp\udm-scheduler-theme-20261008`; final results are in `final-preserved`. See [candidate and validation receipt](validation/scheduler-theme-20261008.json). Not packaged, installed or published. Full IDM parity remains unverified.
