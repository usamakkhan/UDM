# Revalidated shutdown checkpoint: 8.22

Version 8.21's full-process comparison still failed the sequential 64 MiB and split median targets. All failed results remain in [the 8.21 record](validation/finishing-8.21/comparison.json). The measurement includes automatic quit scheduling and application shutdown, not visual dialog time; see [the protocol and its limits](finishing-performance-8.21.md).

## Duplicate write

MainWindow::OnClose explicitly stops the Manager and forces its final catalog save. UDM then destroys the Manager from application exit. The destructor previously called stop again, forcing an identical second catalog serialization, temporary-file write, FlushFileBuffers and replacement. No product behavior required a second unchanged save after a successful first shutdown.

The 8.22 destructor shortcut requires stopping state, no remaining worker threads or active ownership, no transaction, no storage error and an acknowledged checkpoint sequence equal to the latest requested sequence. It snapshots the current live catalog, overlays admission reservations and captures publication outcomes. Any nonempty pending capture prevents the shortcut, even when the resulting JSON equals the previous checkpoint. No synthetic publication acknowledgement is made.

It then pins the actual state.json with read access while denying writes/deletion, rejects directory/reparse targets, reads and parses at most 32 MiB and compares it with the exact durable checkpoint and live snapshot. A missing, malformed, changed, inaccessible or otherwise unverified catalog takes the original forced-save path. Changes after explicit shutdown are also saved. The first explicit stop, every explicit save, active-worker drain and storage-failure retry retain their previous durability behavior. This is not a general skip for unchanged explicit saves.

## Validation

Eleven added lifecycle checks cover the unchanged destructor, changed live settings, missing file, malformed and semantically replaced catalogs, a real failed-write retry and a pending equal-content admission overlay. They count actual temporary-file flush boundaries and inspect the resulting catalog/receipt. The first explicit stop must still issue its forced save. Existing publication/ownership/checkpoint suites remain required.

An initial new-fixture compile used a shared_ptr return for a unique_ptr-owned registry. It was corrected to a non-owning accessor used only before Manager destruction. The product had not been installed. A final review also explicitly required empty active ownership. Core recompiles and all final targets relink; three previously compiled candidate core objects are hash-verified against unchanged inputs. Initial fixture results remain separate from final qualification.

Desktop 8.22, both browser manifests 0.71.86. Browser runtime JavaScript is unchanged. Build, tests, installation and full-exit comparison remain separate records in [qualification](validation/finishing-8.22.json). No setup package or release is requested. Shared media helpers are reused without copying.

## Limits

The shortcut verifies actual catalog bytes against an already acknowledged checkpoint. It is not a new filesystem namespace atomicity or adversarial power-loss guarantee. Required first shutdown/data flushes and catalog/publication recovery remain. Dialog timing is still blocked by desktop-control initialization failure. The paired runner includes all observed app-owned work through exit, plus quit scheduling; system-service work after exit is outside that endpoint. Background storage variance and profile/request-scheduling differences must remain visible in results.

## Installed outcome

Windows approval completed installation of seven verified payloads; both personal catalogs are unchanged. Eighteen full-process comparison downloads match expected hashes and UDM's final stored SHA-256. Median UDM/IDM tails are 118.564/442.065 ms (4 MiB), 560.663/325.819 ms (64 MiB sequential), 536.626/374.405 ms (64 MiB split). Target remains unmet. [Results](validation/finishing-8.22/comparison.json). All eighteen comparison payloads were removed after individual root/reparse/length/hash checks.

Sequential trial 1 records 469 ms of network-tail wait with 2.262 s total prefinish IO; trial 3 records 828 ms with 1.211 s prefinish IO. Final flush is already almost free after those waits. The second early flush starts too late for the measured disk latency. Split cases instead spend 172–225 ms on final flush followed by 142–237 ms on advisory hashing. Candidate 8.23 addresses these observed costs without dropping either required writeback or checksum gates.
