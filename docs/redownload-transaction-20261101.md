# Download Again catalog transaction — 1.6.4

The direct Download Again path previously called add(), which persisted a default entry, and then copied properties and saved again. If preparing or saving those properties failed, the catch removed the copy only from memory; the earlier disk entry remained. A restart could restore that incomplete entry without the original browser session or other inherited properties. Offline archive copies had the same intermediate-save structure.

Download Again now holds the existing catalog transaction gate across creation, inherited-property preparation and offline archive preparation. Intermediate save calls are deferred. It releases the gate for one final save, and restores the prior job list if any preparation or final save fails. Existing originals are not changed. Stale selections and overlapping transactions are rejected before creation. The manager lock prevents a scheduler or worker from observing the intermediate entry.

This covers ordinary direct downloads, stopped FTP downloads, saved rendered archives and offline project copies reached through Download Again. Standalone addOfflineProject admission remains a separate path to review. The copy remains paused and retains its existing unique-filename behavior. Browser code is unchanged at 0.63.82.

Tests are deferred by user request. No installer, installation or publication is requested; installed UDM remains 1.6.0. This correction does not establish full IDM parity.

Build record: [native receipt](validation/redownload-transaction-build-20261101.json).

All five production targets built successfully at 1.6.4. Build-start source hashes remained unchanged; executable versions, sizes and SHA-256 hashes are recorded in the receipt.
