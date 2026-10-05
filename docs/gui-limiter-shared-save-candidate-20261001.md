# Shared speed-limit retention — staged correction

A confirmed defect let a rate set through the Options settings path be forgotten: setting 777 KB/s, disabling it there, and enabling it from the Speed Limiter menu restored an older 321 KB/s value. The native regression failed before the fix.

Manager::setSettings now stores every positive global limit as the remembered rate in the same save transaction. Disabled settings retain their supplied remembered value, including deliberate edits through the limiter dialog. Save failure still restores the prior settings object.

Verification: 90 native menu/transfer checks passed, including three new checks for settings-path-to-menu retention, explicit disabled-rate replacement, and unrelated-setting preservation. Twelve actual app/dialog/restart checks passed. All protected installed application/catalog hashes remain unchanged; isolated app processes exited.

Evidence: D:/UDM-Workspace/candidates/native-084-backup/control/limiter-shared-save-acceptance.json; options-limiter-before and options-limiter-after results retain the failing and corrected runs.

The Options regression exercises its shared Manager save path, not clicks through the complete Options dialog. Exact IDM visual behavior, broad Options rollback/concurrency qualification, throughput comparison, and full parity remain unverified. This correction is staged, not installed or packaged. App and menu fixture objects were relinked after the Core rebuild; diagnostic LimiterTrace files must not be packaged.
