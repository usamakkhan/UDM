# Scheduler long date and keyboard access

The start-date picker now uses the native long-date style found in IDM dialog 306,
rather than a forced ISO date format. Windows supplies the user's long-date
format. The advanced stop-date picker remains separate.

Bottom actions retain the mnemonics from IDM dialog 308: Alt+N for New queue,
Alt+D for Delete, Alt+S for Start now, Alt+T for Stop, Alt+H for Help, Alt+A for
Apply and Alt+C for Close. Existing actions and validation remain in place.

[Reference styles and captions](reference/idm-scheduler-date-shortcuts-20261027.json)
were extracted read-only from installed IDM 6.43.10.2. No IDM code or assets are
included in the app.

The Scheduler harness passed 99 checks, including native long-date formatting,
mnemonic captions and actual Alt+A/Alt+C dispatch. The queue-export/startup
harness passed 69 checks with mnemonic-aware caption lookup; its existing Apply,
Close, Escape and queue-switch persistence checks exercise the visible controls.

The initial bare `IsDialogMessage` shortcut fixture failed its Alt+A check. The
corrected fixture uses a focused native control, the Alt keyboard modifier and
MFC's normal pre-translation/dispatch route, restoring keyboard state afterward.
No application-side shortcut workaround was needed; the retest passed.

The [rendered dialog](validation/scheduler-date-shortcuts-20261027.png) was
inspected: the full long date fits, and time/stop controls remain readable.
[Scheduler results](validation/scheduler-date-shortcuts-ui-20261027.json) and
[startup/save/export results](validation/scheduler-date-queue-ui-20261027.json).
Both test builds, the desktop build and `git diff --check` passed. Installation
completed after Windows elevation, with the installed hash verified. It was then
superseded by the tested [quota keyboard build](scheduler-quota-keyboard-20261027.md),
which includes these changes. Original candidate SHA-256:
`7ED315B82895D4A66E71068083E2EBAA8B0F5058A7EFBC887A1DB358D8D65AF6`.
[Build and installation record](validation/scheduler-date-shortcuts-20261027.json).
