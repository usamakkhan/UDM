# Reference metadata

`pe-analysis.json` was refreshed on 2026-09-20 by `tests/inspect-reference.cjs` from ten files in the user's installed IDM directory. Input SHA-256 values identify the exact inspected binaries. The script reads ordinary/delayed PE import tables and dialog resources without loading DLLs or executing reference code.

This folder contains structural metadata, symbol names and control text/geometry. It contains no executable sections, driver binaries, extracted icon images or copied extension implementation. The report is evidence for an independent implementation; imported function names do not by themselves establish runtime behavior.

Additional 0.10 reports contain resolved delay imports, symbol-address references, UDM before/after samples, and live IDM request summaries. See [the analysis](../reverse-engineering-0.10.md) for confidence levels and measurement limits. Addresses describe the identified binary at its preferred image base, not stable runtime addresses.

`completion-dialog.json` contains only dialog 286's control roles, labels, geometry and source hash, selected from the retained 0.51.0 reference report. It makes the completion fixture reproducible from a checkout without the local raw-evidence directory. It contains no icon images or executable data.

`progress-dialogs.json` contains status panel 363, completion options 364 and speed limiter 365 from the same retained reference report. The progress fixture uses this tracked metadata by default; no local IDM installation or ignored evidence directory is required.
