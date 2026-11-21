# Selection commands during model contention — UDM 9.42

Keep selection-dependent menu advice consistent with the displayed row when Manager is busy. Capture runtime activity/prefetch flags with immutable rows; refresh supported selection actions from that cohort instead of retaining another selection's menu state. Keep live validation on invocation. Build five 9.42 products from 636 frozen product-build inputs; pass 90 focused backend and 867 current UI assertions. The legacy menu suite passes the former active-selection Stop failure, then stops after 435 assertions at: Selected Stop pauses transfer and closes only its progress window. Both source extension manifests advance to 0.73.6; installed 8.68 remains unchanged.

## Confirmed failure

The retained 9.41 legacy menu diagnostic confirms that the active transfer was both rendered and selected, the filter was all, and no history refresh was deferred. Stop remained disabled while the model was busy. This is not explained by a missing selection or row.

The new focused native component makes this deterministic. It receives real loopback bytes, renders that active transfer, first establishes disabled Stop for a completed selection, switches to the active row, and holds Manager on an independent writer. The old menu updater returns without waiting, but preserves the preceding selection's disabled Stop. Its fifth assertion fails while the writer is still held. The baseline uses all 634 frozen 9.41 inputs/cores plus the new UI component, with identical UI test bytes in the corrected run. It is not an installed IDM comparison.

[Natural diagnostic](validation/selection-9.42/excluded-preflight/command-diagnostic-ui-result.json), [deterministic baseline](validation/selection-9.42/excluded-preflight/selection-commands-baseline-ui-result.json), [scope](validation/selection-9.42/baseline-scope.json).

## Snapshot and command policy

DownloadListRow now owns two runtime-only flags: authoritative activity and prefetch deferral. The existing history read captures these under Manager together with each visible row. Changes to either flag invalidate row reuse even if the rendered JSON and rate stay identical; earlier snapshots remain immutable. No catalog migration or persisted field is added.

When a live advisory read succeeds, menus still use that result. When it reports busy, supported selection commands use the current displayed row cohort. Stop, Resume, Remove, Move/Rename, queue removal and single-row queue ordering no longer inherit another selection's state. Mixed and empty selections are evaluated from the current controls. Missing/incomplete render caches retain the existing unavailable-advice path rather than reading mutable jobs.

The fallback reads only UI-owned row/cache structures. It verifies each row's cache ownership without reading live Job JSON under another thread's model lock. Resume and Stop share their predicates with live command validation, including the paused deferring-prefetch exception. Supported toolbar selection actions use the same fallback; unrelated global actions keep their prior behavior.

Actual invocation continues to read and validate current model state. Captured advice does not authorize an obsolete Stop, duplicate Resume, active removal or stale file operation. Filesystem-dependent and unsupported advisory commands are not newly inferred from the limited row snapshot. The broader synchronous context-menu construction path and actual shutdown joins remain separate work.

## Current validation

Five products report file/product version 9.42. Both source extension manifests are 0.73.6. The optimized build freezes 636 repository inputs, generated COM inputs and core hashes before compilation and records dependency-based core reuse. Qualification inventories separately record two test-only adjustments after product compilation; app/core binaries are unchanged by them.

All 90 focused backend assertions pass on D: 22 selection-snapshot checks, 13 navigation checks, 32 history read checks, nine tooltip checks and 14 close-policy checks. They cover activity-only invalidation, unchanged-row reuse, prefetch-only invalidation, active/paused/queued/completed/mixed/empty selections, membership and reorder constraints, immutable prior snapshots, and authoritative activity even when a record's status alone says Paused.

The focused native selection component passes all 13 assertions. It updates real native menu items with Manager held, switches through active/completed/mixed/empty selections, then invokes the real Stop and Resume paths. Its completed 1 MiB file matches the origin's expected SHA-256 exactly. This is a bounded local HTTP transfer, not a live YouTube/X download.

The current close component passes 23 assertions. The full Main/Options suite passes 831 assertions with its updated explicit selection-advice contract. These checks do not resolve the unexplained timing and login-test variability retained from 9.40.

The unchanged legacy menu component passes the former active-selection Stop failure, then stops after 435 assertions at: Selected Stop pauses transfer and closes only its progress window. Passing totals include that run only if all of its assertions pass. Partial or failed menu runs remain visible in their own record.

[Build](validation/selection-9.42/build.json), [original inputs](validation/selection-9.42/dependency-inventory.json), [qualification inputs](validation/selection-9.42/qualification-inputs.json), [versions](validation/selection-9.42/product-versions.json), [backend](validation/selection-9.42/backend-checks.json), [selection/complete file](validation/selection-9.42/selection-commands-checks.json), [Main/Options](validation/selection-9.42/modal-checks.json), [close](validation/selection-9.42/main-close-policy-checks.json), [menus](validation/selection-9.42/main-menu-checks.json).

## Qualification adjustments and cleanup

The first new backend fixture passed its 19 pure policy checks, then rejected its literal test Job identifier. Giving the real Job a valid GUID allows the three model-snapshot checks to run; the failed result and separate test input epoch are retained. No product code changed for that correction.

The old Main suite explicitly expected a disabled Resume item to remain disabled even though its displayed selected row is Paused. That expectation is incompatible with the new selection-coherent advice. Its first run stops at that assertion; the updated test explicitly requires enabled Resume for that fixture and separately retains the prior-state assertion for unrelated global Stop All. Both assertions remain. The original failed record, updated contract diff and separate qualification input epoch are preserved. [Adjustments](validation/selection-9.42/qualification-adjustments/modal-test-contract-update.json), [old contract failure](validation/selection-9.42/qualification-adjustments/old-contract-modal-ui-result.json).

Removed 15 owned fixture/compiler roots and 20 temporary build files (555.2 MiB logical bytes) after exporting sanitized evidence. Current products and required cores, previous 9.41 native products, installed 8.68 / extension files 0.72.32, both personal catalogs and shared media tools remain intact. No media helper was copied. [Cleanup](validation/selection-9.42/cleanup.json), [preservation](validation/selection-9.42/final-preservation.json).

No installer, installation, commit, push or release is included. Owned test desktops close without switching the interactive desktop or sending physical input. These are the declared focused suites, not a complete registry, installed-browser or IDM timing comparison. The original inside/out parity goal remains open.

Next classify any later legacy-menu failure using actual rendered/selected/model state, continue reducing synchronous context-menu and Exit work, and validate broader startup/performance and installed extension capture independently. Preserve live invocation checks and the coherent displayed cohort while improving response times.
