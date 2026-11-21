# UDM 8.51: responsive journal discovery with verified ownership

UDM 8.51 moves journal enumeration, content reading/parsing and ownership claim outside recovery's own model lease. Both extension source manifests advance to 0.72.15; browser JavaScript is unchanged. Installation remains deferred at 8.32.

## Reproduced lock in 8.50

The [exact control source](../benchmarks/speed/recovery_discovery_control.cpp) links an observed 8.50 recovery implementation. A bounded hook inside its actual held read loop reproduces unavailable model access in ordinary publication, paired replacement and recovery selected through the original download. Each control reconciles exact completed bytes after release. These are lock observations, not elapsed-time comparisons with IDM. [Results and source/core hashes](validation/recovery-discovery-8.51/discovery-baseline-proof.json), [instrumentation manifest](validation/recovery-discovery-8.51/baseline-source-manifest.json).

## Discovery and shutdown lifetime

Recovery enrolls a work-only token under Model before releasing it. Shutdown waits even when enumeration has not identified a job. Multiple discovery tokens and normal publication owners retire independently. Enumeration copies paths outside Model; content reading retains the existing 32 MiB limit, and JSON parsing also runs outside Model.

Scoped or known-job discovery normally reserves the captured graph and known paths during each unlocked interval. Membership, pointers, descendants and relevant inputs are checked after reacquiring Model; indexes are rebuilt. Independent description/settings edits and durable saves remain usable and are preserved.

An active incoming replacement legitimately discovers its original before publishing. Temporarily reserving that predecessor can reject its own worker. This path instead captures/revalidates read-only scope inputs while retaining the shutdown token. An actual relevant journal still requires current role resolution, rejection of conflicting active ownership and the full operation reservation before claim. The real synchronization retry suite exposed the initial overstrict guard; that failed result remains in evidence.

## One content read and a same-object claim

The read-only snapshot opens with READ access and READ/DELETE sharing. It permits namespace replacement but denies ordinary direct and hard-link byte writers. The live handle remains open while recovery obtains operation ownership. Exclusive claim opens the current name with READ/DELETE access and READ-only sharing, then compares live volume serial, nonzero file index and raw byte length. The still-open snapshot prevents file-index reuse.

Only a same-object claim inherits the exact bounded content already read; it performs no second content read. A replaced name, identical-content clone or missing scoped intent is refused. The exclusive claim pins its name through verification/catalog acceptance; retirement still removes the exact owned handle. Startup inventory entries that disappear before reading are skipped without acknowledging completion. Unrelated scoped replacement journals remain read-only discoveries rather than unnecessary exclusive claims.

Abandoned discovery closes outside recovery's own model lease. Existing held byte verification, catalog proof, conditional rollback and exact-handle retirement remain. No ordinary transfer durability rule or final flush is relaxed.

## Qualification

Journal enumeration, bounded content reading/parsing and same-file claim now release recovery's own model lease. Scope/input revalidation and shutdown enrollment retain recovery authority; active synchronization workers can discover predecessors without reserving them out from under themselves. Three 8.50 controls reproduced the model lock during actual reads. All 16,276 selected checks pass across 26 runs, including 2,795 discovery checks and 238 workflow cases per drive. Five products and twenty fixtures built from 506 frozen inputs; five core units rebuilt and nineteen native units retained by include-closure audit. Initial fixture and synchronization-worker failures are retained. A one-request 4 MiB lifecycle/hash/normal-exit/post-exit-flush smoke passes. Removed 9,495 owned files / 273,029,223 logical bytes plus the smoke payload. Installation stays deferred at 8.32. Cleanup/pruning, further outer-scope audit, installed acceptance and IDM finishing-speed parity remain open.

The matrix covers ordinary, paired, original-scope, private and child publications; actual enumeration/inventory/read/claim holds; independent saves; destination/membership/graph changes; batches; failures; shutdown; deletion; writers; namespace replacement and clones. Startup cases cover vanished entries and newly active job/path owners. Low-level cases cover BOM handling, bounds, single-use claim and hard-link retirement. The network synchronization suite covers actual worker retry and publication.

One initial fixture checked a deliberately changed destination instead of the original published target. Another expected raw BOM bytes from a helper that strips the BOM. Both failures and the real synchronization regression are retained in [validation notes](validation/recovery-discovery-8.51/validation-notes.json). Source freezing also rejected a stale build after the BOM literal correction; the affected object was rebuilt. Final object/source closures are verified.

[Receipt](validation/recovery-discovery-8.51.json), [suite results](validation/recovery-discovery-8.51/suite-results.json), [source reconstruction](validation/recovery-discovery-8.51/source-reconstruction.json), [cleanup ledger](validation/recovery-discovery-8.51/cleanup.json).

## Remaining work

Post-acceptance retained-media cleanup, zone metadata and receipt-only pruning still need their own detached ownership protocol. Unknown outer recursive scopes can still prevent model availability and require caller audit. Normal mutation persistence and further namespace checks remain separate work. The original slower sequential/split finishing cases, installed/browser acceptance and IDM parity remain unresolved. No fresh IDM comparison or installation is claimed.
