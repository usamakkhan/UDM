# Category file types — staged candidate

Category properties now displays the effective built-in file extensions. Saving a replacement list establishes the override on the first edit; saving an empty list disables that category's default extension matching. Folder-only changes preserve existing category rule order and site-specific priority. A failed catalog save restores the prior settings.

The rebuilt native suite passes 250 checks, including seven new category checks covering replacement lists, empty overrides, folder-only rule priority and real locked-catalog rollback. Ten actual-app checks pass in a fresh isolated profile: visible defaults, Cancel, replacement save/reopen, clearing/reopen, exact download-record preservation, unchanged saved files and normal close.

The initial GUI fixture failed its exact record comparison because it omitted ConfirmationPending=false, which Manager startup adds to loaded records. The successful fresh fixture explicitly seeds that field; the equality assertion was retained. The failed run remains recorded separately. No production code was changed to suppress startup normalization.

[Acceptance](D:/UDM-Workspace/candidates/native-084-backup/control/category-types-acceptance.json) · [Native results](D:/UDM-Workspace/candidates/native-084-backup/control/category-types-native-results.json) · [GUI results](D:/UDM-Workspace/candidates/native-084-backup/control/category-types-app-2-results.json).

Installed app/host/monitor and the personal catalog retain their previous hashes. This candidate is not installed. Built-in category rename/delete, complete category rendering and full IDM parity remain unproven or incomplete.