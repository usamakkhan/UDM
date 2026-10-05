# Predefined category lifecycle — staged candidate

UDM now permits renaming and deleting its six predefined file-type categories: Archives (shown as Compressed), Documents, Music, Programs, Video and Images. Other remains a reserved fallback; its exact IDM lifecycle equivalence is not established.

A persisted HiddenBuiltinCategories list prevents a renamed or deleted default from returning during classification or restart. The main tree, option lists, download admission and Grabber destinations use the active category model. Renaming updates saved record labels, project references, category paths, type rules and remembered-path settings. Deletion reassigns existing category records/project references to Other without moving downloaded files. Reusing a deleted predefined name uses the new explicitly configured types rather than its original defaults. Failed saves restore settings and references.

The candidate previously mixed four rebuilt engine objects with base objects. Because category helpers are shared inline code, all root native sources/headers were materialized without overwriting existing overlay changes, and all 23 engine translation units were rebuilt. The shared object selector now requires every rebuilt overlay object. Base materialization provenance is recorded in control/category-source-materialization.json. The installed project/release is unchanged.

## Verification

269 native checks pass. New coverage includes real locked-catalog rename/deletion rollback, active category list agreement, new engine download admission, Grabber destination routing, persisted defaults, record/project remapping, destination memory, preserved saved paths/bytes, reusing deleted names and rejecting removal of the fallback through settings validation.

37 actual-app checks pass in a fresh isolated profile. The suite retains category dialog geometry and control checks, then renames Video, verifies Cancel and current tree selection, declines and accepts deletion, deletes unrenamed Music, verifies all three category branches, closes/reopens the app and verifies persistence. Saved file bytes remain unchanged. The harness uses Win32 messages only against its owned test app.

[Acceptance](D:/UDM-Workspace/candidates/native-084-backup/control/category-lifecycle-acceptance.json) · [Native results](D:/UDM-Workspace/candidates/native-084-backup/control/category-lifecycle-native-results.json) · [Actual-app results](D:/UDM-Workspace/candidates/native-084-backup/control/category-lifecycle-app-1-results.json).

The personal catalog and installed app/host/monitor hashes are unchanged. This is an uninstalled candidate. Reserved Other behavior, arbitrary site wildcards, wider category interactions and full IDM parity remain unverified or unfinished.
