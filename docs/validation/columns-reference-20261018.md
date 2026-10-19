# Columns dialog reference alignment — 18 October 2026

Live IDM 6.43 observation showed a compact Columns dialog with a headerless checkbox list, Move Up/Down, Show/Hide, Reset, a width field below the list, and OK/Cancel. It opened without a selected column. UDM instead used a 460 by 430 pixel layout with an additional Width column and Set width button.

The retained resource metadata is now available independently in `docs/reference/columns-dialog.json`. The installed reference executable hash matches that metadata. UDM uses its 236 by 177 dialog-unit dimensions, Tahoma 8-point font, instruction text, control labels and positions. The extra Width column and Set width button are removed. Valid width edits survive selection/reordering and are persisted on OK; Cancel discards edits. The existing File Name protection and transactional settings persistence remain in place.

The final suite passed all 147 isolated native control checks, including the title-icon check. They cover initial selection state, dimensions, headerless list, width editing without a separate apply action, visibility, narrow saved widths, reopening, Cancel, Reset, button boundaries, and absence of a horizontal scrollbar at 96/144/192 simulated DPI. The 96 and 192 DPI renders from the earlier 146-check run were inspected; the tracked images correspond to that run. The first successful 143-check run still showed a horizontal scrollbar at 96 DPI; its list column was narrowed and the new regression added before the final run.

The desktop has been rebuilt, installed and checked at startup. The reference screenshot and test WM_PRINT render use different window-frame rendering, so those images alone do not establish pixel equivalence. Physical monitor transitions and complete IDM parity are not proven. No IDM preference was changed; its Columns dialog was only opened for inspection.

Live desktop follow-up: an isolated candidate opened with the same 332-pixel width as IDM, but a 314-pixel height versus IDM's 310 pixels. Removing the extra title icon matched that detail but did not fix the height difference. The updated native suite passed 147 checks, and the desktop rebuilt successfully. The resulting build was subsequently installed successfully. Full pixel equivalence is still unproven.

Installation completed after elevation returned. The installed SHA-256 is 2C298FB225F6372E2EBCB10C13F484AAAAC3076AB7F51E5A23E2CC4E3B287306. UDM reopened normally, with all 28 download IDs, statuses and sizes unchanged. The old executable was backed up. The frame-height mismatch remains open.
