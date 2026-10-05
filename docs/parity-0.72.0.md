# UDM 0.72.0 / browser 0.47.0 — native toolbar customization

**View → Toolbar → Customize** now opens an Available/Current toolbar dialog with Add, Remove, Move Up/Down, repeatable separators and Reset. Changes persist; existing order/visibility preferences migrate. Size, icon/text modes, visibility and queue dropdowns remain available.

Load independently supplied .tbi skins with normal, hover, disabled and high-DPI image support. Missing skins fall back to UDM icons. The release retains UDM's own artwork.

**1,917 passing checks:** 1,868 native, 41 native toolbar/dialog checks and eight host protocol checks. [Evidence and limits](evidence-0.72.0/DEVELOPMENT.md), [summary](evidence-0.72.0/summary.json), [toolbar rendering](evidence-0.72.0/toolbar-default.png), [customization dialog](evidence-0.72.0/customize-toolbar.png).

This addresses major R05/G19 omissions in the [research review](idm-research-gap-review-2026-09-29.md). Full toolbar/GUI parity remains partial: mixed-DPI/accessibility and exhaustive keyboard/drag acceptance remain open, and complete native visual comparison remains open. [Current inventory](idm-parity-0.72.0.md). Recognition/capture rules, cancellation offers, COM integration, driver handoff, transport comparison and distribution qualification remain open.

Browser integration stays 0.47.0; this native update needs no extension reload.
