# Whole main-menu comparison — current staged UDM

Read-only extraction of installed IDM's English main-menu resource was compared with the menu handles of the actual staged UDM executable in a private profile. Dynamic submenus were captured before opening; that limitation matters for queue and toolbar contents.

| Menu | IDM entries | UDM entries | Findings |
|---|---:|---:|---|
| Tasks | 11 | 12 | Missing mnemonic markers, different shortcut presentation, extra catalog import/export entries and backup/recovery placement. |
| File | 4 | 13 | IDM orders Stop Download, Remove, Download Now, Redownload. UDM uses different names/order and puts additional commands here; most also exist in its context menu. |
| Downloads | 15 | 15 | First-level labels and positions match. Static queue placeholder contents differ; earlier tests verify UDM dynamic population, not exact IDM dynamic behavior. |
| View | 9 | 15 | Categories placement differs; toolbar and tray controls need submenu grouping; Customize URL List caption differs; font position/mnemonics differ; English language route absent; extra commands remain. |
| Help | 13 | 3 | UDM exposes integration diagnostics and About. IDM exposes help contents, tutorials, scheduler help, grabber help, tips, home/support, update and sharing routes. Those menu routes are not present in UDM; underlying feature availability requires separate inspection. |

Entry counts include separators. They are not feature counts or parity percentages. Registration/purchase routes remain intentionally excluded, and English-only remains agreed.

Next GUI priorities: review the File menu and context-menu preservation together; rebuild View grouping without losing toolbar customization; supply useful UDM-owned help and support/update routes instead of pointing at IDM services. Do not implement nonfunctional placeholder actions merely to make labels match.

Raw evidence: D:/UDM-Workspace/candidates/native-084-backup/control/all-menu-gap-audit.json, idm-all-menus-reference.json, udm-all-menus-actual.json. The actual app's 18 existing menu/queue/limiter/reopen checks also passed during capture. Protected personal binaries/catalog hashes are unchanged.

This audit confirms substantial GUI work remains. It does not assess rendered dialogs, DPI, keyboard accessibility, toolbar images, download transport, drivers, browser capture or platform coverage. Full IDM parity is not established.
