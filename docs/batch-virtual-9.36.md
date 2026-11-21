# Responsive large batch lists — UDM 9.36

UDM 9.36 / source extension manifests 0.73.0 removes initial per-row Win32 item/subitem population from browser and imported-link batch windows. The earlier loading frame was responsive, but converting thousands of links into native list items still occupied the window thread for seconds.

## Measurement and scope

The isolated 9.35 source reproduction matches every frozen 9.35 repository input and linked core hash, with the new timing component added. It opens the actual batch dialog behind a held catalog writer. The timer observes and paints its available Cancel frame, releases the first writer, and measures until the complete list is available. A second writer holds Manager after capture, proving that completing and painting the list does not reacquire the live catalog.

| Rows | 9.35 ready after release | 9.36 ready after release | 9.35 UI CPU | 9.36 UI CPU |
| ---: | ---: | ---: | ---: | ---: |
| 100 | 410.0 ms | 334.0 ms | 375.0 ms | 203.1 ms |
| 2,000 | 2509.3 ms | 442.3 ms | 1484.4 ms | 218.8 ms |
| 10,000 | 10951.7 ms | 1443.4 ms | 7156.2 ms | 250.0 ms |

These are individual local runs, not repeated medians, installed-app measurements or IDM comparisons. The 100-row current fixture also exercises HTML filtering; the larger fixtures retain the same observed application/octet-stream source profile. Each source carries an observed 1 KiB response size, so these measurements exclude network probing. Ready time includes the worker's projection and the window's full-view construction. UI CPU is measured from fixture entry through readiness and is not whole-process CPU.

The 100-row first-frame sample is slower than baseline (50.9 ms versus 32.9 ms), while the larger first-frame samples improve. This is not a claim that every timing sample is faster; repeated distributions and installed comparisons remain open. The first loading frame is recorded separately in the [current results](validation/batch-virtual-9.36/batch-large-checks.json). No physical mouse/keyboard input, desktop switch or personal browser operation is used.

## Implementation

BatchInitialTask snapshots settings/categories/queues under Manager, releases its lock, and prepares rows with cooperative cancellation. BatchRowProjection computes fallback names, origins, browser names/extensions, initial visibility, destinations and preview requests on that worker. Request preparation retains browser headers, proxy/session context, observed-response policy and saved-login invalidation. The scoped reader borrows immutable caller containers only until its joined take/finish; cancellation and close cannot outlive them. Preview ownership moves into the window's existing lifecycle.

BatchVirtualList uses LVS_OWNERDATA with a single item-count publication. Stable source IDs, checkbox values and text stay in the row model; the window asks for visible cells. A per-source/per-column text cache supplies stable Unicode callback buffers and invalidates on cell edits or list rebuilds. The 10,000-row fixture checks the first/last values and viewport cache size instead of accepting an empty placeholder list.

Existing batch code uses item data, text reads/writes, checkbox state and sorting operations that native owner-data lists do not directly support. The adapter handles those messages explicitly. Sorting/filtering retain source checks, selection and focus. State-icon clicks and Space update the model once; programmatic selection commands use the same model. Exact/prefix searches respect their requested wrap flag.

Microsoft's [list-view overview](https://learn.microsoft.com/en-us/windows/win32/controls/list-view-controls-overview) documents the owner-data callback/storage model and unsupported ordinary item messages; [extended list styles](https://learn.microsoft.com/en-us/windows/win32/controls/extended-list-view-styles) documents checkbox state images and input behavior. Those API contracts guided the adapter; local behavior is established by the test records below.

Initial browser rows remain unchecked. Imported rows retain their original defaults. Metadata still updates the same cells and default checks, and accepted rows retain the original authenticated admission path. Idle enablement counts the model's visible checks instead of reading every native checkbox each pulse. Per-file request/header editing and later policy refresh still have work to move off the window thread.

## Rejected approaches and preliminary failures

The first attempted fix projected rows on the worker but populated native items in small window-thread slices. Its functional checks passed while total time regressed. Instrumented slicing still spent substantial CPU setting native text and repainting between slices. These implementations were rejected rather than qualified.

All preliminary records, inventories and logs are retained under [excluded-preflight](validation/batch-virtual-9.36/excluded-preflight/batch-large-preflight-ui-result.json), including both slower sliced results, the first virtual pass and the subsequent interaction passes. They link 9.35 cores to developing 9.36 sources and are excluded from the current-build totals. The baseline test component changed as interaction assertions were added; exact source hashes are retained, and the original baseline is not labeled as an identical-test rerun.

An initial standalone-header compile failed because helper-owning includes were missing. The first row projection test then failed on a mixed-separator path expectation; the assertion was corrected to compare normalized filesystem paths. Their failed records and logs remain distinct from the later passed checks. The corrected preliminary backend run passes 19 assertions; the final preliminary virtual/UI run passes 49.

## Current-build validation

Five products report file/product version 9.36. Both source extension manifests are 0.73.0, rolling over the previous 0.72.99 while keeping each part within two digits. The build freezes 623 repository inputs and generated COM inputs before compilation, audits each reused core object's dependencies, recompiles Bridge for the version input, and verifies hashes after compilation.

All 752 targeted backend assertions pass across 30 C:/D: runs and 15 suites. New row-projection checks cover filtering and source identity, destination caches, observed metadata reuse and invalidation, request/header/proxy/session context, cancellation and work outside a held Manager. They are not a full backend-suite claim.

The 1,339 current UI assertions comprise 49 large-list and interaction checks, 261 loading/layout transitions, 90 opening/late-metadata checks, 108 admission checks and 831 Main/Options checks. Virtual checks exercise native checkbox hit targets/messages, Space/release, programmatic checks, actual sort handlers, selected/focused source IDs, HTML filter removal/restoration, Check All/Uncheck All, search wrapping and model-cell cache invalidation. They use messages within the owned test desktop, not physical user input.

The compatibility suites still verify held-lock cancellation/retry, late metadata preserving explicit choices, theme/focus/DPI/resizing and Main/Options behavior. Ten accepted authenticated files complete with their full payload and exactly one transfer request each. All five owned UI desktops close.

[Build](validation/batch-virtual-9.36/build.json), [dependency inventory](validation/batch-virtual-9.36/dependency-inventory.json), [changes from 9.35](validation/batch-virtual-9.36/source-change-audit.json), [versions](validation/batch-virtual-9.36/product-versions.json), [backend](validation/batch-virtual-9.36/backend-checks.json), [large list](validation/batch-virtual-9.36/batch-large-checks.json), [transitions](validation/batch-virtual-9.36/batch-transition-checks.json), [opening](validation/batch-virtual-9.36/batch-opening-checks.json), [admission](validation/batch-virtual-9.36/batch-admission-checks.json), [Main/Options](validation/batch-virtual-9.36/modal-checks.json).

## Cleanup and next work

Removed 39 owned fixture/compiler roots and 41 temporary build files (416.3 MiB logical bytes) after retaining evidence. Current products/required cores, previous qualified 9.35 products, installed 8.68 / extension files 0.72.32, both personal catalogs and shared media tools are preserved. No media helpers were copied. [Cleanup](validation/batch-virtual-9.36/cleanup.json), [preservation](validation/batch-virtual-9.36/final-preservation.json).

No installer, installation, commit, push or release is included. Next move the remaining row/request/credential editing and metadata-refresh preparation off the window thread, then qualify actual owner/minimize/multi-monitor behavior and remaining Main/modal/shutdown waits. Installed browser capture, complete media output and live IDM timing comparisons remain separate work. Full inside/out parity is still unproven.
