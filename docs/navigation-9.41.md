# Consistent requested history views — UDM 9.41

Keep valid requested history filters when queues/projects/categories are added, renamed or reordered. Fix the first-refresh mismatch where the tree selected a new queue but the list showed All Downloads. Retain deleted-node ancestor fallback and nonblocking busy reads. Build five 9.41 products from 634 frozen inputs; pass 68 focused backend and 35 current UI assertions. The legacy menu suite passes the previously failing queue-view assertion, then stops after 413 assertions at: Active selection enables Stop. Source extension manifests advance to 0.73.5; installed 8.68 remains unchanged.

## Reproduced cause

This increment classifies the queue-view failure retained in 9.40. Its shared snapshot resolver honored a valid requested view only when the old and new navigation structures matched. Adding a queue changes that structure. A programmatic selection has no matching old caret ancestry, so the snapshot fell back to All Downloads.

The native tree already selected the new queue correctly through its node insertion logic. The list was populated from the snapshot's All Downloads rows, leaving the selected tree and displayed files inconsistent. No tree-selection rewrite or forced synchronous refresh is needed.

The new console component reproduces this against frozen 9.40 inputs and cores: its first owned All Downloads check passes, then the requested new-queue filter is incorrectly returned as all with two records. The new GUI component also reproduces it: the initial list passes and the actual tree selects the new queue, but the third assertion sees two displayed rows instead of its one member. Both baseline processes end; the owned UI desktop closes. These are source reproductions, not installed IDM comparisons.

[Backend baseline](validation/navigation-9.41/excluded-preflight/backend-baseline.json), [GUI baseline](validation/navigation-9.41/excluded-preflight/navigation-ui-result.json), [scope](validation/navigation-9.41/baseline-scope.json).

## Production change

HistoryReadSnapshot.hpp now resolves a currently valid requested key before comparing navigation structure. If the requested key is missing, unchanged-navigation requests still fall back to All Downloads; structural removals still walk the surviving selected ancestors. The queues-root alias still resolves to all while preserving its native caret.

The snapshot reader retains its existing try-lock behavior. Busy reads neither wait for Manager nor replace the rendered rows or caches. No new download/metadata request, transfer operation, persisted setting or background worker is introduced. App.cpp is unchanged; the corrected snapshot now agrees with its existing tree logic.

## Current build and checks

Five native products report file/product version 9.41. Both source extension manifests are 0.73.5. The optimized native build freezes 634 repository inputs and the generated COM inputs before compilation, verifies reused core hashes, and recompiles Bridge for the version input. Dependency parsing is cached within this build controller; compiled production dependencies remain explicitly recorded.

All 68 focused backend assertions pass on D: 13 new navigation checks, 32 full history snapshot checks, nine tooltip checks and 14 close-policy checks. They cover the first new-queue cohort, valid requests without an older tree, new projects/categories, unrelated structural changes, explicit request priority, invalid keys, repeated snapshots and removed-node fallback. Existing snapshot checks retain locked-writer behavior, immutable row records, search caches, hidden history and browser-offer reads.

The new native navigation component passes all 12 assertions. It checks the actual tree caret, real list count/text and Job identities on the first new-queue refresh, subsequent convergence, busy-read retention and recovery, new/renamed/deleted projects, Finished during unrelated queue changes, and the queues-root caret alias. Its owned records are synthetic; it does not download media.

The current close component passes all 23 assertions with the rebuilt App/header/core cohort. Those results do not resolve or disprove the unexplained 400 ms aggregate watchdog event retained in 9.40. No universal timing guarantee is claimed.

The unchanged legacy menu component passes the previously failing queue-view assertion, then stops after 413 assertions at: Active selection enables Stop. The aggregate passing-assertion count includes a menu run only when the whole run passes; failed assertions are never counted as successful.

[Build](validation/navigation-9.41/build.json), [inputs](validation/navigation-9.41/dependency-inventory.json), [changes](validation/navigation-9.41/source-change-audit.json), [versions](validation/navigation-9.41/product-versions.json), [backend](validation/navigation-9.41/backend-checks.json), [native navigation](validation/navigation-9.41/navigation-checks.json), [close](validation/navigation-9.41/main-close-policy-checks.json), [menus](validation/navigation-9.41/main-menu-checks.json).

## Excluded preparation errors

The first GUI test draft used diagnostics as a namespace variable, colliding with an existing Core function. Renaming only that test variable preceded the GUI baseline and corrected run; the compile log is retained. The backend baseline compiled and ran independently and is unaffected by that unused GUI draft.

The native build controller initially included a nonexistent optional NavigationTreeSnapshotTests target. Its missing-source compiler log is retained. The corrected controller resumed against the unchanged frozen source/generated-input hashes, rebuilt the selected core unit and every declared target, and completed the actual four-suite backend set. No failed or unbuilt optional target is counted.

## Cleanup, limits and next work

Removed 11 owned fixture/compiler roots and 17 temporary build files (391.8 MiB logical bytes) after exporting sanitized evidence. Current products and required cores, the previous 9.40 native products, installed 8.68 / extension files 0.72.32, both personal catalogs and shared media tools remain intact. No media helper was copied. [Cleanup](validation/navigation-9.41/cleanup.json), [preservation](validation/navigation-9.41/final-preservation.json).

No installer, installation, commit, push or release is included. Checks were confined to owned native test desktops; no interactive desktop switch or physical input occurred. This is not a full backend-registry run, live browser capture test, completed-media validation or fresh IDM performance comparison.

The original inside/out parity goal remains open. Continue classifying any later menu failures from actual displayed/worker state. Then measure and stage actual Exit, whose layout capture and worker joins remain synchronous; investigate the prior timing/login test variability without weakening latency assertions or making asynchronous production work block to satisfy older fixtures. Broader startup/list performance, extension visuals/capture and installed IDM comparisons still require their own evidence.
