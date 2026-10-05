# UDM 0.73.0 / browser 0.47.0 — queue startup and export scopes

Scheduler now offers **Start download on UDM startup** for each queue. The choice persists independently of Stop, applies on the next app launch, and preserves existing schedules and membership protections.

Export now offers **All downloads**, **Selected downloads**, or **Files in download queue**, with named/all queue selection and retained per-file checkboxes. URL lists and UDM catalogs remain available.

**1,938 checks passed:** 1,895 native, 30 native dialog checks, nine host protocol checks and four actual app-startup checks. An isolated startup download matched the server's SHA-256; opting out prevented the next launch from downloading. Overlong instance names now report an error instead of using the main app's connection.

[Evidence](evidence-0.73.0/summary.json), [behavior, corrections and limits](evidence-0.73.0/DEVELOPMENT.md), [Scheduler](evidence-0.73.0/scheduler-startup.png), [Export](evidence-0.73.0/export-queues.png).

R10 startup and R12/G18 scope-control omissions are addressed. IDM catalog interoperability, complete GUI/reference qualification and the broader research gaps remain open. [Current inventory](idm-parity-0.73.0.md). Browser 0.47.0 and the network runtime are unchanged; no new extension reload is needed for this native release.
