# Selective imported-file metadata refresh — UDM 9.39

UDM 9.39 / source extension manifests 0.73.3 removes the whole-batch metadata refresh performed after editing an imported file's address or per-file request credentials. Browser rows already used their changed index; imported rows passed no index and restarted every preview. A missing preview owner also made single-slot preparation fail instead of recovering.

## Reproduced 9.38 behavior

The new native editor fixture is compiled against every frozen qualified 9.38 production input and linked core hash, plus its test component. It opens the actual Import links dialog, waits for two authenticated metadata responses, opens File Properties for the first row, changes its address and saves.

The other row's size is cleared, failing the assertion that its metadata remains available. The preserved diagnostics record four accepted metadata requests and two policy reads: the original two probes plus two new probes for one edited file. The later exact-one-request assertion is not reached because the unrelated-size assertion fails first. The final close assertion retains that error as well; the process exits and its owned desktop closes.

A separate backend reproduction records three assertions. Reusing an existing owner and its policy passes, but recreating a missing owner fails. Both reproductions use the same test bytes as the corrected runs, after the initial draft's compile errors were fixed.

[Editor baseline](validation/import-single-9.39/excluded-preflight/baseline-9.38-import-edit.json), [owner baseline](validation/import-single-9.39/excluded-preflight/baseline-9.38-single-owner.json), [scope](validation/import-single-9.39/baseline-scope.json), [editor inputs](validation/import-single-9.39/excluded-preflight/baseline-source-inputs.json), [owner inputs](validation/import-single-9.39/excluded-preflight/backend-baseline-source-inputs.json).

## Request and owner lifetime

The imported editor now passes its actual changed source index to refreshMetadata, exactly as the browser editor does. Address/credential changes invalidate and prepare that one row. Name/folder/description-only edits and Cancel retain their existing no-probe behavior.

When an owner exists, BatchInitialTask uses that owner's captured preview-policy cohort and replaces only the requested slot on its worker. It performs no new Manager read and does not cancel or recreate the other slots. This retains existing browser-row semantics for imported rows; it is not a fresh-global-settings read on every edit.

When the owner is absent, the worker reads current settings using the same cancellable acquisition loop as other policy reads. It releases Manager before projecting requests. It then creates an empty bounded BatchDownloadPreview owner and calls indexed replace for the changed slot. It does not allocate a full batch of placeholder requests or enqueue probes for the unresolved rows. A nonzero source index stays nonzero in metadata completion callbacks, and subsequent edits can reuse the recreated owner.

Changed descriptors are validated before projection: negative/out-of-range indices and missing changed-row inputs are rejected. The result still carries the full source count and the declared changed identity, and the 9.38 request-identity guard must match before any metadata is published.

After successful single-slot recovery, other rows invalidated by the failed bulk operation can remain unresolved. Their suppression is retained, and the UI keeps an enabled Retry with “Some file details are unavailable. Retry to check all files.” No bulk retry is started automatically. An explicit bulk Retry retains its existing fresh-policy/all-files semantics.

## Current-build validation

Five products report file/product version 9.39; both source extension manifests are 0.73.3. The serialized build freezes 629 repository inputs, generated COM inputs and linked core hashes before compilation. Bridge recompiles for the version input.

All 876 targeted backend assertions pass across 36 C:/D: runs and 18 suites. The new single-owner suite contributes 19 per drive. It covers existing-owner reuse, fresh settings after owner loss, nonzero mapping, subsequent other-slot reuse, error/Retry, acquisition under a held writer, cancellation before that writer releases, malformed descriptors, projection cancellation and unchanged fixture catalogs/command ownership.

All 1,559 current UI assertions pass: 79 imported-property workflows, 14 Form dispatch, 53 large-list/refresh, 261 loading/layout transitions, 190 opening/refresh/late-metadata, 131 admission and 831 Main/Options.

The six actual native import workflows edit the first address, second address, per-file Referer/credentials toggle, cosmetic fields, Cancel and recovery after a forced bulk-policy failure. They begin with exactly two accepted authenticated probes. A saved request edit adds exactly one; cosmetic edits and Cancel add none. Explicit check choices and the fixture catalog are preserved.

| Import edit case | Metadata requests in total | Policy read attempts |
| --- | ---: | ---: |
| address-first | 3 | 1 |
| address-second | 3 | 1 |
| cancel | 2 | 1 |
| cosmetic | 2 | 1 |
| credentials | 3 | 1 |
| recovery | 3 | 3 |

Address-second and credential cases hold Manager while the edited row still completes, proving reuse does not acquire it again. The recovery case first injects a failure at the real bulk policy-read boundary, then edits the nonzero row with the owner missing. It verifies responsive pulse/paint and Cancel while fresh-policy acquisition waits, releases the writer, and sees only the changed row recover. The other row remains Unknown and explicit bulk Retry stays visible.

The exact displayed sizes match the local origin's 1,048,647-byte Content-Length. All metadata requests authenticate without challenges. These are metadata HEAD workflows; they are not new full media-download or installed browser comparisons. The separate admission suite still completes all ten selected authenticated payloads with one transfer request each.

All seven owned UI desktops close. No physical input, interactive desktop switch or personal browser control is used.

[Build](validation/import-single-9.39/build.json), [inputs](validation/import-single-9.39/dependency-inventory.json), [changes from 9.38](validation/import-single-9.39/source-change-audit.json), [versions](validation/import-single-9.39/product-versions.json), [backend](validation/import-single-9.39/backend-checks.json), [import edits](validation/import-single-9.39/batch-import-edit-checks.json), [dispatch](validation/import-single-9.39/form-command-checks.json), [large lists](validation/import-single-9.39/batch-large-checks.json), [transitions](validation/import-single-9.39/batch-transition-checks.json), [opening/refresh](validation/import-single-9.39/batch-opening-checks.json), [admission](validation/import-single-9.39/batch-admission-checks.json), [Main/Options](validation/import-single-9.39/modal-checks.json).

## Preliminary boundaries and corrections

Developing 9.39 sources linked to qualified 9.38 cores pass 19 backend and 79 import-editor assertions. These runs are excluded from current-build totals. Their inventories, results, hashes and logs are retained.

The first test draft did not compile because its diagnostics variable collided with an existing function and one auto declaration combined different types. No fixture ran from that draft. Those two test-only compile fixes preceded both baseline reproductions.

The initial baseline runner expected the later request-count assertion specifically; the earlier unrelated-size assertion failed first. Its classification was corrected using the already-written failed record and observed four-request diagnostics. The baseline was not rerun or relabeled as passing.

[Boundaries](validation/import-single-9.39/preliminary-boundaries.json), [compile failure](validation/import-single-9.39/excluded-preflight/harness-compile-failure.json), [compile log](validation/import-single-9.39/excluded-preflight/harness-compile-failure.log), [baseline classification](validation/import-single-9.39/excluded-preflight/baseline-classification.json), [corrected owner checks](validation/import-single-9.39/excluded-preflight/single-owner-preflight-result.json), [corrected editor checks](validation/import-single-9.39/excluded-preflight/batch-import-edit-ui-result.json).

## Cleanup and remaining work

Removed 47 owned fixture/compiler roots and 49 temporary build files (481.2 MiB logical bytes) after retaining sanitized evidence. Current products/required cores, qualified 9.38 products, installed 8.68 / extension files 0.72.32, both personal catalogs and shared media tools remain intact. No media helper is copied. [Cleanup](validation/import-single-9.39/cleanup.json), [preservation](validation/import-single-9.39/final-preservation.json).

No installer, installation, commit, push or release is included. Next audit remaining editor validation/credential work and Main close/shutdown waits from measured behavior, then address very large sort/filter and exceptional teardown paths. Actual owner/minimize/multi-monitor transitions, installed extension capture, complete media files and live IDM comparisons remain separate validation. These targeted checks do not prove the entire backend registry or full inside/out IDM parity; the original goal remains open.
