# Catalog duplicate identity — 1.6.8

Catalog duplicate detection previously compared URL, filename, folder, request headers and POST body only. Two records could be considered equivalent despite different signed-in browser sessions, proxy routes, recapture requirements, offline project settings or expected hashes.

The comparison now projects the incoming record into the context import can actually retain. Omitted credentials and nonportable staged POST bodies become recapture requirements; encrypted fields are compared through their decoded values rather than randomized ciphertext. Available media plans and rendered HTML on an existing entry remain distinct from omitted catalog context. Incoming credentials are validated before considering duplicates. If an existing record's credentials cannot be read, equivalence is unproven and a new collision-safe entry is admitted instead of aborting the import.

When import assigns a numbered filename, the record saves its requested and assigned names. Repeated imports can match that assigned entry with the same request context and folder. A later filename change disables this alternate-name match. These internal origin fields are not imported from catalogs, exported as credentials or inherited by Download Again.

Existing files and records are not overwritten. Import continues to stage all entries and queues until its final save. The correction does not merge cookies or change browser login policy. Older numbered imports have no origin fields and are not retroactively identified.

Native version: 1.6.8; browser remains 0.63.82. Runtime tests are deferred by user request. Installed UDM remains 1.6.0; no installer, installation or publication is requested. Full IDM parity remains unverified.

Build record: [native receipt](validation/catalog-context-identity-build-20261101.json).

All five production targets built successfully at 1.6.8. Build-start source hashes remained unchanged; executable versions, sizes and SHA-256 hashes are recorded in the receipt.
