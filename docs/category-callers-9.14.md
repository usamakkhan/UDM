# Owned category results through caller return — UDM 9.14

UDM 9.14 / source extension manifests 0.72.78 removes the remaining synchronous settings copies around the shared category-properties editor in Main and Options. Category commands also skip the unrelated displayed-download selection validation that could wait while retained history was stale.

The editor returns an owned CategoryPropertiesResult containing the accepted name, fresh category cohort and whitelisted category settings captured by its save worker. File Info rebuilds from that result. Main opens New/Edit without preparing preferences. Options opens its child without a pre-read, then synchronizes only CustomCategories, HiddenBuiltinCategories, CategoryRules, CategoryTypeOverrides, CategoryPaths and CategoryRememberLast into both its draft and original baseline. It reloads the category controls from owned preferences. Unrelated unfinished Options edits and their original baseline are retained; later Options saves do not reapply acknowledged category changes merely because that child was used.

The child's accepted Save still owns the global decision guard through acknowledgement, and Cancel during loading still stops only the advisory read. Actual download commands keep their existing authoritative selection validation. The category command change does not weaken download selection checks.

## Qualification

Five native products built and passed 260 targeted backend assertions across ten C:/D: runs. Pending File Info passed 257 assertions and presentation passed 327. Main stopped after 460 passing assertions at its next setup assertion: the expected category-entry row was not displayed. An earlier scenario deliberately hid Main; this fixture omitted restoring it and selected Other, which has no dedicated navigation node. No caller-boundary assertion from the new section was reached. This candidate is unqualified for the full workflow. Its original failure and successful suites are retained separately from 9.15 qualification. No installation or IDM timing comparison occurred.

[Original Main failure](validation/category-callers-9.17/superseded-9.14-modal.json), [backend checks](validation/category-callers-9.17/superseded-9.14-backend.json) and [native build](validation/category-callers-9.17/superseded-9.14-build.json).

## Remaining work

Save To's direct Browse, remember-path, server-date and temporary-folder callbacks still commit settings synchronously; this increment covers category-properties New/Edit and their caller return. File Info's server-name action, scheduling/admission and exceptional teardown retain additional waits. Optional browser fixtures and installed IDM comparisons remain separate. Full inside/out parity is unproven.

Installed UDM remains 8.68 / extension files 0.72.32. Browser runtime JavaScript is unchanged. No installer, installation, personal-desktop input, commit, push or release is included.
