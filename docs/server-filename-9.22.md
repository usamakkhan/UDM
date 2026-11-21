# Responsive server filename adoption — UDM 9.22

UDM 9.22 / source extension manifests 0.72.86 moves File Info's **Use server name** preparation off the window thread. It checks the current advertised filename, automatic category, download reservations and existing destination on an owned advisory worker. It releases Manager before checking the filesystem. Clicking the action no longer waits for those checks on the dialog thread.

The completed result applies only to the exact displayed suggestion and captured editor context. Late changes to the filename, destination, selected category, remember rule, login draft or preview/category generation invalidate it. Automatic category inference retains the existing redirected-host policy and only selects a visible category. Manual destination folders remain selected. Filename-only adoption also preserves a custom remember-rule folder; an actual automatic category change adopts that category's own folder and remember preference.

Start and Download Later defer until preparation settles. A stale/error result clears that intent and leaves the edited draft available for review. Cancel cancels acquisition and joins in-flight work on the existing decision worker. Already-running filesystem calls may still need to drain. Final Start/Later continues to perform authoritative destination validation; the advisory check is not a reservation or permission to overwrite.

## Validation

Five normal optimized native products have file/product version 9.22, with 603 frozen repository inputs. Seven backend suites pass 388 assertions across fourteen C:/D: runs. Actual owned Win32 controls pass 738 Main/Options assertions, 257 pending File Info assertions, 327 presentation assertions. The separate early 40-assertion preflight and failed three-assertion optional first-paint diagnostic are excluded from those totals. The latter stops at the reported-size label assertion before reaching server-name cases; it remains unresolved. Cold direct invocation now paints its shell before the owned initial read, while this older fixture expects its labels on that first paint. Investigate that route and the browser-provided initial snapshot without weakening the assertion. Owned test desktops were never switched to or given personal input.

The new UI cases cover automatic category adoption, manual folders, a custom remember rule, edits during preparation, a changed server name, an existing destination, cancellation, Download Later and Start. A real writer holds Manager while the action/pulse/paint and queued decisions return. Start retrieves the complete expected authenticated fixture in one request; advisory-only cases issue no request. This is targeted regression evidence, not a live browser/IDM timing comparison or proof of full parity.

9.21 / extension 0.72.85 is a superseded candidate. Its new redirect-category fixture incorrectly used an empty extension list; both its early run and first normal backend run stopped at assertion 9. The existing policy requires an explicit extension match. 9.22 corrects the fixture to ZIP; production category matching is unchanged. Retain those failures separately from passing qualification.

[Build](validation/server-filename-9.22/build.json), [backend](validation/server-filename-9.22/backend-checks.json), [Main/Options](validation/server-filename-9.22/modal-checks.json), [pending](validation/server-filename-9.22/pending-checks.json), [presentation](validation/server-filename-9.22/presentation-checks.json), [failed first-paint diagnostic](validation/server-filename-9.22/first-paint-diagnostic.json), [source changes](validation/server-filename-9.22/source-change-audit.json) and [product versions](validation/server-filename-9.22/product-versions.json).

After exporting evidence, removed 22 owned fixture roots and 50 temporary build files (466.6 MiB logical bytes). Preserve current products/required cores, qualified 9.20 products, installed files, personal catalogs and media helpers. [Cleanup](validation/server-filename-9.22/cleanup.json) and [preservation](validation/server-filename-9.22/final-preservation.json) record the boundaries.

## Remaining work

Installed UDM remains 8.68 with extension files 0.72.32. No installer, installation, commit, push or release is included. Other synchronous File Info actions, initial admission and exceptional teardown waits still need review. Continue the architecture backlog and fresh live browser/IDM comparisons before claiming parity; these tests establish responsiveness under owned controlled contention only.
