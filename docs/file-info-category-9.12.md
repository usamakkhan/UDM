# Responsive File Info category changes — UDM 9.12

UDM 9.12 / source extension manifests 0.72.76 moves category-folder preference reads and default-path preparation off the File Info window thread. Rapid category selections coalesce into one latest pending choice, while filename and destination editing stay available. This change does not claim measured IDM latency equality.

## Owned read and draft merge

FileInfoCategoryTask owns one lazy persistent worker and one outstanding future. The UI keeps at most one newer queued request; a new choice cancels advisory acquisition for the previous one. A monotonically checked revision and selected category reject stale results. The reader copies only folder/category preferences, validates current category membership, releases Manager before deriving the folder, and rejects relative defaults. It never writes settings, retargets a saved download or opens an HTTP request.

A bound dialog reads current settings on the worker. An unbound prepared dialog can use its owned category/preferences snapshot without acquiring Manager. Results publish before a weak Form task notification; callback failures cannot undo readiness. The owner joins before releasing Manager, and can restart the reader after a failed parent cancellation leaves File Info open.

A late result applies the selected default folder to the current filename. A newer manually chosen destination folder wins. A newer remember-path checkbox/folder edit also wins. Programmatic result application suppresses the destination-change rule synchronization that would otherwise overwrite that newer rule draft. Category captions change immediately without waiting for settings.

## Start, Later and Cancel

Start and Download Later retain their intent while category preparation is outstanding; neither commits the old destination. The category-properties button and both scheduling controls stay disabled until a selected default can be used. Ordinary metadata and prefetch ownership retain their existing flow; category result wake-ups do not start another polling/read loop.

Cancel discards a newer queued category and cancels the advisory reader before the existing joined cancellation command drains it. The window remains alive while any required saved-download cleanup waits for Manager. Read failures preserve the draft and require another valid category choice before Start/Later or scheduling. No default-folder result changes the saved download until an accepted download decision.

## Qualification

Five native products built with verified file/product version 9.12 from 597 frozen source inputs; Chromium and Firefox source manifests are 0.72.76. Four targeted backend suites passed 222 assertions across eight C:/D: runs. Optimized native Main workflow passed 442 assertions, pending File Info passed 257 and presentation passed 327. These run on owned isolated desktops without switching the user desktop or injecting personal input. The Main workflow covers rapid category changes under a held model, late filename/destination/rule edits, deferred Start/Later, cancellation and a complete authenticated loopback payload with exactly one request. This is targeted qualification, not a rerun of the full backend suite or an installed IDM timing comparison.

[Build and hashes](validation/file-info-category-9.12/build.json), [backend assertions](validation/file-info-category-9.12/backend-checks.json), [Main workflow](validation/file-info-category-9.12/modal-checks.json), [pending workflow](validation/file-info-category-9.12/pending-checks.json), [presentation workflow](validation/file-info-category-9.12/presentation-checks.json), [source changes](validation/file-info-category-9.12/source-change-audit.json) and [version records](validation/file-info-category-9.12/product-versions.json) are retained. Superseded 9.10 failures and 9.11 diagnostic runs are separate and excluded from those counts.

After evidence export, removed 27 owned fixture roots and 58 temporary build files (875.1 MiB logical bytes). Current products/required core objects and previous qualified 9.9 products remain available. [Cleanup](validation/file-info-category-9.12/cleanup.json) and [preservation](validation/file-info-category-9.12/final-preservation.json) record the boundaries. Installed app/catalog/helper hashes remain unchanged.

## Remaining work and delivery boundaries

The category-properties editor still reads/saves settings synchronously, and Use server name, scheduling, caller restoration/admission and exceptional final teardown retain other model waits. Optional first-paint/browser modes and live installed browser/IDM comparison remain separate. Full inside/out IDM parity remains unproven.

Installed UDM remains 8.68 / extension files 0.72.32. Browser runtime JavaScript is unchanged; only source manifests advance. No installer, installation, personal-desktop control, commit, push or release is included.

## Advisory menu routing

The wider Main workflow reveals blocking menu-update reads while a writer owns Manager. Advisory selection now uses the displayed cohort, and menu preference flags use a short try-lock snapshot or the existing owned appearance snapshot. A busy read preserves the current check/radio presentation. Actual command execution still requests authoritative selection checks. New real CCmdUI probes verify both paths under a held model; the existing held-content warning also runs unchanged.

## Superseded candidates

9.10 / 0.72.74 builds five products and passes 222 backend, 327 presentation and 257 pending-dialog checks, but Main stops at the held-content warning before reaching the new category scenes. 9.11 / 0.72.75 changes advisory routing; its diagnostic fixture exits without results because the new menu probe omits CCmdUI index bounds. An owned crash observation identifies an MFC CInvalidArgException. A separate /Od private probe that supplies those bounds and catches MFC pointer exceptions passes 442 checks. Those observations are preserved separately; the corrected native fixture bounds are qualified by the fresh 9.12 optimized runs. Diagnostic mixed-core /Od runs are excluded from final qualification.
