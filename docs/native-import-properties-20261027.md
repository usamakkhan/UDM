# Native import properties

The retained G16 comparison and IDM's browser import guide identify per-link
property editing as part of the import workflow:
https://www.internetdownloadmanager.com/support/right_click_IE.html

UDM's native `selectBatch` dialog is also used by generated batches and imported
text URL lists. It previously offered a shared destination and authorization only.
It now has an Edit command for the selected, not-yet-imported row. The editor
provides Address, File name, Save to with a folder picker, and a multiline
Description. Cancel leaves the row unchanged.

Empty filenames retain server naming. An empty per-link folder inherits the import
folder, then the category default. Address parsing, safe filename validation and
absolute-folder validation occur before accepting the edit. The main import list
shows the changed address/name. Added rows cannot be edited or imported again
within the same dialog after a partial failure.

Native admission retains duplicate policy and collision handling. Description is
applied through the existing configuration/save path only when admission produced
a new record. Choosing an existing duplicate does not overwrite its metadata.
The manager lock spans admission and configuration so scheduled workers cannot
start a newly admitted row between those operations. If configuration fails after
admission, that row remains marked added to prevent a retry from adding it again.

This change does not establish visual equivalence to IDM's property dialog, and
does not close the native import's remaining type/size preview or wildcard rename
work. Interactive and regression testing are deferred at the user's request.
Build evidence is recorded in
[the candidate record](validation/native-import-properties-build-20261027.json).
The new candidate is not installed.

## Destination modes

The retained IDM dialog 228 explicitly exposes the three destination choices
(controls 1357–1359 and 1204). Native import now offers those choices, enabling
the category selector or folder controls only for their matching mode. Category
mode resolves the current configured category folder under the manager lock and
validates that the category still exists before admitting each row. Explicit
per-row folders retain precedence. Existing duplicates keep their prior metadata.

An explicit category is marked on a newly admitted record so extension correction
from downloaded content cannot reclassify it. Redownload and duplicate-restart
paths preserve that marker alongside the category. Unmarked records retain existing
automatic category inference. The import window grows to accommodate the extra
destination controls; this is not a claim of pixel-identical IDM geometry.

The destination candidate is recorded in
[its build record](validation/native-import-destinations-build-20261027.json).
Runtime and regression tests remain deferred; installation is pending.

## Batch filename pattern follow-up

Explicit output-pattern substitution for generated sequences is now implemented in [batch output patterns](batch-output-pattern-20261027.md). The earlier wildcard gap is narrowed to metadata-driven shared extension adjustment and runtime qualification.
