# Replacement recovery catalog rollback — 1.2.15

Replacement recovery previously assigned the recovered download and original-history records directly, then called save(). A failed catalog write could leave those live records changed while the durable catalog and recovery journal still described an interrupted operation.

Recovery now prepares both JSON snapshots locally before changing live state. It swaps both prepared records into place, saves the catalog, and swaps both back if saving throws. The recovery journal is removed only after saving succeeds. Missing required snapshots raise an error before live state changes; the original-file restoration branch also reads its snapshots before moving the file.

If restoring the original file succeeds but catalog saving fails, the file remains restored and the journal remains available. A subsequent recovery can recognize the original-at-target state. The fix does not undo a successful filesystem restoration or delete downloaded staging data on a catalog error.

Native production build: passed for all five targets at version 1.2.15. Build-start source hashes remained unchanged; executable sizes and SHA-256 hashes are recorded. Runtime and fault-injection tests are deferred by user request; compilation does not establish runtime recovery correctness. No installer, installation or publication requested. Installed UDM remains 1.2.8; browser code remains 0.63.82.

Build evidence: [record](validation/replacement-recovery-transaction-build-20261030.json).
