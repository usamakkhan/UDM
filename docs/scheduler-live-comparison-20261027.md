# Installed Scheduler comparison

Desktop control recovered on 2026-10-27. Installed UDM was verified by SHA-256
`CC091430309D474C3487AC131D2A08606D759DFD1DDFBA9616185ACE995B8526`.
UDM's Scheduler and installed IDM 6.43's Scheduler were opened and inspected
through live desktop screenshots. No downloads were started, removed or reordered;
no schedule values were deliberately edited. Both Scheduler dialogs were closed.
Personal catalog entries are omitted from this report.

UDM visibly presents native time spinners and the long-date picker. Its dark
Scheduler differs from IDM's light Scheduler in this session, even though both
main windows were dark. Queue names/icons and several enabled states also differ:
IDM's built-in main queue showed Delete, Stop and Apply disabled; UDM showed those
buttons enabled. These state differences need behavioral investigation before
changing logic, because the applications have different saved profiles.

The actual IDM Files in the queue tab was successfully opened. It shows:

- A numeric spinner beside “Download,” followed by “files at the same time.”
- File Name, Size, Status and Time left columns.
- Compact icon actions ordered Down, Up, Remove from left to right.

Correction after full source review: UDM already creates a `ThemeSpin` with the
parallel-count edit in `referenceLayout`; the initial claim of a missing spinner
was incorrect. The confirmed gaps were File name/Size/Status columns only and
Up/Down/Remove actions. Earlier resource evidence established button rectangles
but did not establish these runtime column or icon details. This live observation
supersedes the earlier wrong-tab screenshot as evidence of the reference's file
page. It does not establish identical runtime scheduling behavior.

The desktop helper exposed the modals visually but did not consistently expose
their accessibility tree. Coordinate selection using the returned dialog
screenshot successfully selected IDM's file tab and closed the dialog. The
installed UDM navigation changes remain backed by the separate 693-assertion
harness; this observation is not a complete installed keyboard test.
