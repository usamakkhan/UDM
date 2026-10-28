# Native import sorting

Native batch and text-list imports now sort by File name or Address when their
column header is clicked. Repeating the click reverses the direction, and the
active header shows its direction. Comparison is case-insensitive Unicode ordinal
ordering; equal values keep their original input order. This is UDM's comparison
policy, not a claim that IDM uses the same collation.

Each list item carries its original row ID. Editing, checked-row counts, Check
all, and admission use that ID instead of the displayed position. Sorting thus
keeps filenames, folders, descriptions, checkboxes and the partial-import
already-added guard associated with the same address. Admission follows the
displayed order. Editing a value updates its cell; another header click sorts
the changed values.

## Reference and limitations

The retained analysis of the reference IDMan.exe (SHA-256
03cc62e9adb77a380f9dc12f67ccaaee5106f12844aa73ce32c914ddd16d607c)
maps dialog 228's list control 1392 through function 004bbc10. Function 004bbc70
updates its header sort-direction flags. The implementation here is independently
written; no proprietary code or assets are included.

Wildcard replacement remains open. Resource edit 1040 and function 004bdb80
establish a replacement path, but the retained pseudocode has not yet established
all substitution semantics. A fresh REA search using Ghidra 12.1.3 timed out after
330 seconds; it is not successful verification of those semantics.

Runtime, interactive and regression tests are deferred at the user's request.
Build and staging evidence is in
[the build record](validation/native-import-sorting-build-20261027.json).
The candidate is not installed.
