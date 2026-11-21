# Owned category results through caller return — UDM 9.17

UDM 9.17 / source extension manifests 0.72.81 removes the remaining synchronous settings copies around the shared category-properties editor in Main and Options. Category commands also skip the unrelated displayed-download selection validation that could wait while retained history was stale.

The editor returns an owned CategoryPropertiesResult containing the accepted name, fresh category cohort and whitelisted category settings captured by its save worker. File Info rebuilds from that result. Main opens New/Edit without preparing preferences. Options opens its child without a pre-read, then synchronizes only CustomCategories, HiddenBuiltinCategories, CategoryRules, CategoryTypeOverrides, CategoryPaths and CategoryRememberLast into both its draft and original baseline. It reloads the category controls from owned preferences. Unrelated unfinished Options edits and their original baseline are retained; later Options saves do not reapply acknowledged category changes merely because that child was used.

The child's accepted Save still owns the global decision guard through acknowledgement, and Cancel during loading still stops only the advisory read. Actual download commands keep their existing authoritative selection validation. The category command change does not weaken download selection checks.

## Qualification

Five native products built with file/product version 9.17 from 599 frozen source inputs. Chromium/Firefox source manifests are 0.72.81. Five targeted backend suites pass 260 assertions across ten C:/D: runs. Optimized Main workflow passes 515 assertions, pending File Info 257 and presentation 327. New Main New/Edit cases open and cancel under a held catalog with stale selected history. Options New/Edit cases open under a held catalog, then acknowledge Save before a second writer acquires Manager; caller return, six-field draft/baseline synchronization, category selection and page repaint all complete while that newer writer remains held. Unrelated unfinished Options choices and their original baseline remain intact. The actual accepted new/renamed category folder is retained after Options closes. These run on owned isolated desktops without switching the personal desktop or injecting user input. No installed timing or IDM parity claim is made; the prior broad backend baseline is not relabeled as current.

[Build](validation/category-callers-9.17/build.json), [backend assertions](validation/category-callers-9.17/backend-checks.json), [Main/Options assertions](validation/category-callers-9.17/modal-checks.json), [pending assertions](validation/category-callers-9.17/pending-checks.json), [presentation assertions](validation/category-callers-9.17/presentation-checks.json), [source changes](validation/category-callers-9.17/source-change-audit.json) and [product versions](validation/category-callers-9.17/product-versions.json) retain the evidence.

After exporting results, removed 29 owned test roots and 59 temporary build files (922.7 MiB logical bytes). Current products/required cores and qualified 9.13 products remain available. [Cleanup](validation/category-callers-9.17/cleanup.json) and [preservation](validation/category-callers-9.17/final-preservation.json) record the boundaries. Installed files, personal catalogs and helpers remain unchanged.

## Advisory command routing

OnUpdateAppCommand now uses the existing nonblocking snapshot path for availability checks that read Manager. A busy read returns no new advisory decision, preserving the displayed enabled/disabled state. Selection-only advice uses displayed rows. Actual command invocation retains fresh authoritative checks. This extends the earlier nonblocking preference-flag and selection-only menu work to commands that inspect Manager state.

## Superseded candidates

9.14 / 0.72.78 builds native products and passes targeted backend, pending and presentation suites, but Main stops at its new row setup because the earlier hidden Main window was not restored and Other has no dedicated navigation node. The original 461-assertion result includes that failure and is not qualification. 9.15 / 0.72.79 is an /Od diagnostic with restored navigation; it fails two entry assertions without control-readiness diagnostics. 9.16 / 0.72.80 adds nonblocking model-backed menu advice and two held-writer menu checks; its /Od driver passes the first main command but sees the next category window before OK/Cancel exist. Recorded state has held=true, expired=false, ok=false and cancel=false. 9.17 waits for visible, constructed controls and keeps the writer-hold assertions. All superseded and /Od records remain separate from current optimized qualification.

## Remaining work

Save To's direct Browse, remember-path, server-date and temporary-folder callbacks still commit settings synchronously; this increment covers category-properties New/Edit and their caller return. File Info's server-name action, scheduling/admission and exceptional teardown retain additional waits. Optional browser fixtures and installed IDM comparisons remain separate. Full inside/out parity is unproven.

Installed UDM remains 8.68 / extension files 0.72.32. Browser runtime JavaScript is unchanged. No installer, installation, personal-desktop input, commit, push or release is included.
