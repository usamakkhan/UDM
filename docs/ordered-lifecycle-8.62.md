# Ordered HTTP lifecycle qualification, 8.62

The unchanged private candidate passes **62 native assertions in ten processes**, covering folder changes, address refresh and list removal on each of C: and D:. Each scenario exercises an active transfer, retirement behind a held old catalog save, an injected operation-save failure with rollback, and successful mutation after retirement. Folder and refresh cases then resume in fresh processes to the exact complete 8 MiB file. No production code, version or default activation changed.

## Actual command boundaries

The [controller](../benchmarks/speed/ordered_lifecycle_control.cpp) calls actual Manager methods, not stand-ins. The synthetic range origin runs a slower primary and faster helpers. Once at least 512 KiB has been received, it attempts the selected mutation and requires refusal with unchanged address, destination and list membership.

A separate save captures a detached old-generation checkpoint outside the model lock. A concurrent pause drains accepted ordered writes, but retirement remains active while the save is held. The fixture requires that the old save is still pending, parts still exist, and neither a final destination nor a completion acknowledgment exists. It attempts the mutation again and requires the same active-operation refusal. Releasing the checkpoint lets pause and worker retirement finish before mutation proceeds.

| Operation | While active or retiring | Injected save failure after retirement | Successful command |
| --- | --- | --- | --- |
| Folder edit through configure | Refused, old folder retained | Exact job and previous on-disk catalog restored; parts hash unchanged | New folder persisted without changing segment credit; fresh process publishes only in that folder |
| Refresh address | Refused, old address retained | Exact job and previous on-disk catalog restored; parts hash unchanged | Address with a new query persisted, partial credit retained; origin observes the refreshed request and fresh process completes |
| Remove from list | Refused, membership retained | Original job/list membership and previous catalog retained; parts hash unchanged | Membership absent from real saved catalog; no destination published and retained partial data not rewritten |

The injected failure throws at `snapshot-prepared` in the actual save path before publication. This covers operation rollback at that boundary, not every filesystem failure or a crash after catalog replacement. List removal intentionally tests its existing retain-file semantics; it does not claim to delete partial files immediately. Owned test cleanup removes them afterward.

## Independent recovery evidence

After each prepare process exits normally, the orchestrator reads every credited segment prefix from the actual parts file and compares it with the deterministic source. Saved received counts equal the sum of those verified prefixes. Successful folder/address edits retain that segment plan and count in their catalog. Each of the four resume processes completes the exact 8,388,608 bytes; independent expected SHA-256 matches both file and catalog. A separate post-exit output flush also passes.

The [summary](validation/ordered-lifecycle-8.62/results.json) records six scenarios. Individual `C-folder.json`, `C-refresh.json`, `C-remove.json` and D: counterparts contain sanitized checks, catalogs and request traces. Each prepare has nine native assertions and each resume has two: 6 × 9 + 4 × 2 = 62. This aggregate is separate from the response, shutdown and original build suites.

The first Python collector used `target` instead of the origin request's `path` field. The C: folder and refresh native processes, prefix checks and final hash checks had already succeeded. Their outputs and origin trace were retained and revalidated; correcting the field allowed continuation without repeating those downloads. The [collector correction](validation/ordered-lifecycle-8.62/rejected-orchestrator.json) records this as a harness error, not a backend failure. No production change was made to satisfy it.

## Cleanup and scope

[Cleanup](validation/ordered-lifecycle-8.62/cleanup.json) removed **40 generated files / 60,226,000 logical bytes** in six owned roots, including all payloads and the fixture executable/object. Hashes, resolved root boundaries, reparse refusal and owned hardlink groups scoped deletion. Logical bytes are not a physical freed-space claim.

[Preservation](validation/ordered-lifecycle-8.62/preservation.json) verifies 524 qualified inputs, ten products, 43 distinct core objects and three protected installed/personal files. Source versions remain 8.62 / 0.72.26. Normal builds retain synchronous writes. No installation, installer, extension deployment, administrator prompt, push or release occurred.

These cases close the scoped active/retiring mutation, post-retirement save rollback and fresh-process folder/refresh recovery checks. Combined with the separate response, storage-fault and shutdown controls, they support the selected fresh known-size root HTTP candidate. They do not cover buffered child/media operations, every crash/storage-failure interleaving or universal correctness. Balanced C:/D: full-lifetime performance comparisons, with every slower case and final background work retained, remain before deciding default activation. Installed IDM/browser comparisons remain separately pending.
