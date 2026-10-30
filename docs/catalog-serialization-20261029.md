# Catalog serialization outside the manager lock

Large adaptive plans are stored encrypted inside the download catalog. The catalog write/read size limit is consistently 32 MiB: save rejects an oversized snapshot before replacing state.json, and receipt-bearing admission rolls back on save failure. This review did not change that storage limit or claim unlimited history capacity.

The periodic save path nevertheless serialized the entire detached catalog snapshot to JSON while holding the manager mutex. Even though the disk write was already outside that mutex, CPU time spent serializing large saved plans could block UI and transfer access to shared state.

Manager::save now takes the snapshot and assigns its sequence while locked, then serializes the independent JSON snapshot outside the manager mutex. It reacquires the lock to update the checkpoint cache only when that sequence is still current. Serialization and disk-write errors update storageError only for the current sequence, so a delayed older failure cannot overwrite the status of a newer save. The existing write mutex and catalogWritten ordering still prevent a slower older snapshot from overwriting a newer completed write. The 32 MiB guard still runs before writing.

This benefits callers such as the periodic save that do not already hold the manager lock. Callers that intentionally hold an outer recursive lock still retain it. Snapshot creation and checkpoint-cache copying still take place under the lock; these are not claimed to be eliminated. No latency or throughput measurement was performed.

All five native production targets compiled and installer packaging passed. See [build evidence](validation/catalog-serialization-build-20261029.json); the production bundle manifest contains all 147 payload hashes. Extension 0.63.76 is unchanged. Runtime, concurrency and fault-injection tests remain deferred at user request. No installation or publication is performed.
