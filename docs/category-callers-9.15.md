# Owned category results through caller return — UDM 9.15

UDM 9.15 / source extension manifests 0.72.79 removes the remaining synchronous settings copies around the shared category-properties editor in Main and Options. Category commands also skip the unrelated displayed-download selection validation that could wait while retained history was stale.

The editor returns an owned CategoryPropertiesResult containing the accepted name, fresh category cohort and whitelisted category settings captured by its save worker. File Info rebuilds from that result. Main opens New/Edit without preparing preferences. Options opens its child without a pre-read, then synchronizes only CustomCategories, HiddenBuiltinCategories, CategoryRules, CategoryTypeOverrides, CategoryPaths and CategoryRememberLast into both its draft and original baseline. It reloads the category controls from owned preferences. Unrelated unfinished Options edits and their original baseline are retained; later Options saves do not reapply acknowledged category changes merely because that child was used.

The child's accepted Save still owns the global decision guard through acknowledgement, and Cancel during loading still stops only the advisory read. Actual download commands keep their existing authoritative selection validation. The category command change does not weaken download selection checks.

## Qualification

An isolated /Od diagnostic using verified 9.14 core objects failed after 463 passing and two failed assertions (465 total). The restored Main fixture reached category-command setup but did not complete its entry assertions; that run has no control-readiness diagnostics and does not establish the exact reason. It is excluded from native-product qualification. Later 9.16 diagnostics show a timer observing a category window before its OK/Cancel controls exist, while the catalog writer remains held and unexpired. These fixture readiness issues are separate from the blocking model-backed menu update discovered by source audit. No normal 9.15 product build, installation or timing comparison occurred.

[Diagnostic records](validation/category-callers-9.17/diagnostic-preflights.json) retain the original assertions and later readiness observations; these are excluded from optimized qualification.

## Remaining work

Save To's direct Browse, remember-path, server-date and temporary-folder callbacks still commit settings synchronously; this increment covers category-properties New/Edit and their caller return. File Info's server-name action, scheduling/admission and exceptional teardown retain additional waits. Optional browser fixtures and installed IDM comparisons remain separate. Full inside/out parity is unproven.

Installed UDM remains 8.68 / extension files 0.72.32. Browser runtime JavaScript is unchanged. No installer, installation, personal-desktop input, commit, push or release is included.
