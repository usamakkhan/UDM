# Offline archive admission — 1.6.5

Standalone addOfflineProject previously persisted an ordinary download through add(), then attached offline project settings, protected HTML and login state before saving a second time. Failure during this second stage removed the entry only from memory, leaving the first persisted record available after restart.

The method now defers intermediate saves until the archive record is fully configured. On failure it restores the previous job list and transaction state. When called inside Site Grabber's project transaction or Download Again, it retains that outer transaction and leaves final persistence to its caller. Standalone calls perform one complete catalog save. The manager lock excludes scheduler and worker observation of the intermediate entry.

Both the ordinary and rendered archive paths retain their existing destination, category, session validation and unique-name behavior. No page is downloaded by admission itself. Tests are deferred by user request; this source correction does not prove runtime or IDM parity.

Native version: 1.6.5. Browser code remains 0.63.82. Installed UDM remains 1.6.0. No installer, installation or publication is requested.

Build record: [native receipt](validation/offline-admission-build-20261101.json).

All five production targets built successfully at 1.6.5. Build-start source hashes remained unchanged; executable versions, sizes and SHA-256 hashes are recorded in the receipt.
