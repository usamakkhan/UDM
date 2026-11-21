# Owned category-properties editing — UDM 9.13

UDM 9.13 / source extension manifests 0.72.77 moves category-properties loading and accepted saves off the dialog thread. File Info's plus button opens the editor without first reading Manager; a successful save returns its fresh category cohort for parent rebuilding without another synchronous catalog read.

The shared category editor shows a loading shell with Cancel available. It reads current category preferences rather than trusting an older caller snapshot. Editing becomes available only after loading; a failed read offers Retry. A removed original category cannot be edited. The worker copies only editor preferences, including custom file-type overrides, rather than unrelated credentials or browser data.

OK captures owned strings and flags before starting a save. An accepted Save owns the existing global decision guard, freezes editing and rejects duplicate OK and Close until the command settles. Read cancellation cannot discard an accepted mutation. A validation failure preserves the user's controls for correction; the existing category transaction restores settings on failure. Successful Save returns the actual name and category list before the owner releases Manager. Weak task notification follows result publication; teardown joins before releasing the model.

## Qualification

Five native products built with file/product version 9.13 from 599 frozen source inputs. Chromium/Firefox source manifests are 0.72.77. Five targeted backend suites pass 260 assertions across ten C:/D: runs. Optimized Main workflow passes 460 assertions, pending File Info 257 and presentation 327. The added actual-dialog scenarios hold a real catalog writer during initial loading, Cancel and accepted Save; pulse/paint return, duplicate OK/Close cannot discard the accepted mutation, and the parent receives the fresh saved category cohort. These run on owned isolated desktops without switching the personal desktop or injecting user input. No installed timing or IDM parity claim is made. The previous broad backend baseline is not relabeled as current.

[Build](validation/category-properties-9.13/build.json), [backend assertions](validation/category-properties-9.13/backend-checks.json), [Main dialog assertions](validation/category-properties-9.13/modal-checks.json), [pending dialog assertions](validation/category-properties-9.13/pending-checks.json), [presentation assertions](validation/category-properties-9.13/presentation-checks.json), [source changes](validation/category-properties-9.13/source-change-audit.json) and [product versions](validation/category-properties-9.13/product-versions.json) are retained.

After exporting evidence, removed 13 owned test roots and 22 temporary build files (303.9 MiB logical bytes). Current products/required cores and qualified 9.12 products are preserved. [Cleanup](validation/category-properties-9.13/cleanup.json) and [preservation](validation/category-properties-9.13/final-preservation.json) retain the boundaries; installed files, personal catalogs and helpers remain unchanged.

## Remaining work

Main-window and Options entry points still prepare their own caller preferences synchronously; Options also resynchronizes its draft after category save. File Info's server-name action, scheduling/caller admission and exceptional teardown retain additional waits. The shared category mutation can still take time on its worker; this change keeps the dialog thread responsive rather than eliminating filesystem latency. Optional browser fixtures and installed IDM comparisons remain separate.

Installed UDM remains 8.68 / extension files 0.72.32. Browser runtime code is unchanged; source manifests advance with this desktop improvement. No installer, installation, personal-desktop control, commit, push or release is included.
