# Completion dialog file drag

The large icon now converts its client-relative mouse position to screen coordinates before calling the Windows drag detector. Windows uses that point to center its drag threshold. [DragDetect reference](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-dragdetect)

All 139 native completion-dialog checks passed against the retained reference measurements. Four added checks used Windows `DragQueryFileW` to read the production file-drop data. They covered the exact Unicode path after Move/Rename from each drag handle, ordering and filtering in a multi-file drop, and omission of a file moved outside UDM. The fixture results are in `candidates/completion-drag-5.70-tests/results/results.json`.

The physical gesture and a drop into a live Explorer window remain to be checked. [Native build receipt](completion-drag-5.70-build.json)
