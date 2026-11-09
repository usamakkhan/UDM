# Live saved destination in Download complete

The completion dialog is modeless, so a file can be moved from the main list or Properties while its completion dialog stays open. In the previous implementation the Open actions used the catalog's new destination, while the saved-path field and the two drag controls retained the original destination.

5.67 reads the committed target once when it opens the dialog. It updates the path field, drag handles and file-type icon together when Move/Rename commits a new target. The passive update skips a busy catalog lock. Open, Open with, and Open folder each resolve the committed target at invocation. An external move that does not update the catalog still receives the moved-file warning.

The completion fixture checks the retained IDM tab sequence, a move to a Unicode destination with a new extension, both drag sources, and the missing-file warning after a subsequent external move. [IDM completion resource measurements](../reference/completion-dialog.json)

All 135 native completion checks passed. The fixture also retained the ordinary geometry, scanner bounds at simulated DPI, suppression persistence and single completion event for captured downloads. It exercised the moved-file warning with the new target. Explorer launches, actual drag-and-drop into another app and a live IDM comparison remain open.
