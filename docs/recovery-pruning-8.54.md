# UDM 8.54: receipt-only recovery cleanup

Native source is **8.54** and both source extension manifests are **0.72.18**. Browser JavaScript is unchanged. The build is qualified privately; installed desktop remains **8.32**, and no extension deployment, installer, push or release was performed.

## Behavior and baseline

A private completion receipt can remain after its publication journal has been retired. Unscoped recovery previously checked journal absence and saved receipt removal while retaining its recursive model lease. The [qualified 8.53 control](validation/finishing-8.53/receipt-pruning-baseline.json) reproduced blocked model access at a held, real catalog flush with one and sixteen receipts on C: and D:. That is a causal hold rather than a measurement of ordinary latency.

The new transaction keeps model snapshots available during those filesystem and persistence operations. It does not remove a completed file or change its bytes. It retains receipts when either journal is observed, and releases every temporary reservation after success or refusal.

## Transaction architecture

1. Copy and validate canonical receipt descriptors under Model. Capture matching visible/descendant jobs when present; private receipts require no fabricated visible row. Skip destinations already owned by other publication work.
2. Reserve the affected job graph, destinations, previous-version paths and both publication/replacement journal names. The lease remains enrolled in shutdown through the persistence decision. Real publishers and claimed recovery operations reserve their journal names too, so another destination cannot bypass the same-identifier exclusion.
3. Release recovery's own model lease for journal-absence checks using immutable paths. Reacquire it and validate the catalog root, current receipt values, membership, job graph, relevant inputs, closing and batch state. Unrelated receipts and metadata can change independently.
4. Remove only selected exact receipts and prepare a distinct absence proof. Conflicting completion or retirement proofs for every affected job are excluded in either acquisition order. A catalog snapshot retaining any selected key cannot acknowledge this outcome.
5. Release the model lease for serialization, persistence and resolution of captured writers. An actual newer snapshot containing the same absence can prove acceptance if an older save fails. A deferred save, noncontaining snapshot or unknown acknowledgement cannot prove acceptance from a sequence number alone.
6. On refusal, release the proof and restore only missing original keys. Preserve a later receipt under the same key, unrelated new receipts and independent metadata. Once durable retirement is accepted, do not undo it. Shutdown drains the owned operation; a later closing check can still abort the remaining recovery scan after accepted cleanup.

Reservations coordinate in-process operations. They do not freeze external filesystem namespace changes. The new inventory interval also releases only this function's own model lease; an unknown recursive outer caller can still retain another lease. Those remain explicit audit gates.

## Qualification

All **22,769 checks across 21 runs** match the final executable hashes. The new fixture passes **267 checks per drive**. It covers actual held inventory, prepared-save, flushed-file, captured-snapshot and post-publication acknowledgement boundaries; single/sixteen receipts; independent saves and metadata; new, removed and replaced receipts; changed visible destinations; a new journal; real conflicting and unrelated publications; failed, deferred and lost-ack saves; newer containing writers; active ownership and shutdown. Completed bytes remain exact. [Final suite records](validation/recovery-pruning-8.54/suite-results.json).

The retained regressions cover publication preparation/ownership/commit/recovery, replacement commit and measured metadata, early-intent recovery, recovery catalog/cleanup/discovery, checkpoint coordination, Properties commands and synchronization replacement. C: and D: runs retain their own results. Native build uses 510 frozen inputs and 42 core objects: five rebuilt native units, nineteen retained native units and eighteen external C objects. A later journal-guard refinement rebuilt Duplicates, rebuilt the pruning entry point and relinked all 21 outputs after verifying every retained entry include closure. All file versions are 8.54. [Build and closure records](validation/recovery-pruning-8.54.json).

A real native-app 4 MiB download used one origin request, matched the independent length/hash and saved Complete digest, exited normally after its worker work, passed a separate completed-output flush and left no publication intent. The payload was removed. Correctness qualification may have been running concurrently; this is a lifecycle/integrity check with **no IDM speed-parity claim**. [App record](validation/recovery-pruning-8.54/app-4m-smoke.json).

The first expanded fixture incorrectly tested the broad destination query, which also includes ordinary visible completed downloads. Its assertion was corrected to query the temporary publication lease. A reduced 181-check probe and a 22,703-check intermediate pass precede journal-name hardening; neither is added to the final count. [Validation notes](validation/recovery-pruning-8.54/validation-notes.json).

## Source and cleanup

The [eight-input source delta](../benchmarks/speed/experiments/recovery-pruning-8.53-to-8.54.json) verifies both reconstruction directions against qualified 8.53 inputs, including raw line endings. The [journal-guard delta](../benchmarks/speed/experiments/pruning-journal-guard-8.54.json) identifies the intermediate and final variants of this unreleased increment. Reconstruct matching 8.53 headers before rebuilding its diagnostic control with old objects.

Removed **14,422 generated files / 243,500,410 logical bytes**, the separate **4 MiB** smoke payload, and **15,147 empty owned test directories**. Each deletion stayed in verified generated roots on C: or D:. Complete hard-link groups, identity, hashes and reparse exclusions were checked before individual unlinking; directories were removed only when empty. Shared tools, personal catalogs, prior qualified objects and five final products remain. The public [cleanup receipt](validation/recovery-pruning-8.54/cleanup.json) is compact; a hashed compressed per-file inventory remains private. Logical totals do not measure reclaimed physical allocation.

## Next work

Future cleanup helpers should resolve the allowed roots once and cache containment checks before verifying each file's identity and link group. Repeated root resolution in this cleanup pass generated unnecessary metadata traffic; it did not affect the completed native qualification.

Continue the external namespace and recursive caller audits, and identify remaining synchronous persistence callers. Verify installed desktop/browser behavior when installation is resumed. The underlying zone-policy difference still needs an Internet-origin IDM comparison. Original sequential/split finishing delays need valid IO attribution and fresh full-lifetime comparisons; no final flush, recovery barrier or pacing rule changed here. The failed bounded kernel captures and reverted loader-profile experiment remain documented in [8.53 research](finishing-research-8.53.md).
