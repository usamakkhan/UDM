# Responsive schedule preparation - UDM 9.25

UDM 9.25 / source extension manifests 0.72.89 opens File Info's Schedule wizard before waiting for Manager. Previously, File Info read the job's connection/speed defaults on the window thread before calling the wizard. The wizard then acquired categories and folder history before opening, and reacquired settings for Use category folder, source-name defaults and queue-name advice during page navigation.

One owned advisory reader now captures the current category cohort, whitelisted category policy, queue names and optional per-file connection/speed defaults. It releases Manager before filtering folder history to twenty unique absolute paths. The loading window paints immediately with navigation/submission disabled and Cancel available. Errors stay inline with explicit Retry; cancellation drains acquisition without requiring an unrelated writer to release. The wizard retains modal shutdown ownership through preparation, editing and accepted Save.

Once the result arrives, the same window constructs its editor from owned values. File Info's staged destination, filename and description survive preparation. Use category folder and source suggestions use the captured policy, and Dates uses the captured queue names for advice. No editor action reads the catalog. A new-download wizard keeps its optional per-file defaults when no existing job supplies them. A removed job cannot supply defaults. Credential settings are excluded from the preparation result.

The snapshot is advisory. Accepted writes remain on FileInfoDecisionTask and validate current category/queue and download state at commit time. A queue created after preparation is rejected by the canonical save without adding a download. Existing Save ownership, failure acknowledgement, draft preservation, explicit retry and posted completion are retained. Read cancellation does not clear another command's accepted guard.

## Validation

Five optimized native products report file/product version 9.25 from 607 frozen repository inputs; Chromium/Firefox source manifests are 0.72.89. Ten backend suites pass 512 assertions across twenty C:/D: runs, including 21 preparation assertions per drive. An early 21-assertion preflight is retained separately and excluded from those totals.

Actual isolated Win32 controls pass 738 Main/Options, 257 pending File Info, 327 presentation, 46 first-paint and 171 schedule-wizard assertions (1539 UI assertions). The wizard driver covers five preparation cases: direct and nested File Info Cancel, direct and nested readiness, and an inline initial-read failure followed by Retry. A real writer holds Manager while loading, paint/pulse, cancellation and nested window opening are observed. After preparation, another writer holds Manager while category-folder application and Destination/Dates navigation return. The Options page retains coherent connection/speed defaults. Cancel preserves the saved download; staged File Info edits remain uncommitted.

Eight further wizard cases cover new, listed, revised and File Info schedules, each with normal save and a forced real catalog-checkpoint failure followed by explicit retry. They verify one owned draft, stable existing job identity, no duplicate queue, a paused editable future schedule and posted completion after removing the polling timer. The older File Info wizard driver now waits for its actual enabled Schedule control instead of clicking the disabled loading shell.

No personal desktop or browser was controlled. Owned test desktops were not switched to or given user input. These are targeted correctness/responsiveness checks, not a new live browser/IDM timing comparison or full-suite/parity proof.

## Superseded candidate

9.24 / extension 0.72.88 is an incomplete candidate. Its new preparation fixture used an invalid saved-login record and failed validation before any assertion. The owned build and compiler were stopped after that reproduced preflight failure, before product compilation. No complete native build or application qualification is claimed. 9.25 constructs the credential fixture through the normal saved-login constructor; production scheduling behavior did not change for that correction. Preserve its source inventory, failure and stop records separately.

[Build](validation/schedule-preparation-9.25/build.json), [backend assertions](validation/schedule-preparation-9.25/backend-checks.json), [Main/Options](validation/schedule-preparation-9.25/modal-checks.json), [pending](validation/schedule-preparation-9.25/pending-checks.json), [presentation](validation/schedule-preparation-9.25/presentation-checks.json), [first paint](validation/schedule-preparation-9.25/first-paint-checks.json), [wizard assertions](validation/schedule-preparation-9.25/wizard-checks.json), [source changes](validation/schedule-preparation-9.25/source-change-audit.json), [product versions](validation/schedule-preparation-9.25/product-versions.json) and [superseded failure](validation/schedule-preparation-9.25/superseded-9.24-preflight.json).

## Cleanup and next work

Removed 31 owned fixture/compiler roots and 46 temporary build files (423.2 MiB logical bytes). Preserve current products/required cores, qualified 9.23 products, installed 8.68 / extension files 0.72.32, personal catalogs and shared media helpers. [Cleanup](validation/schedule-preparation-9.25/cleanup.json) and [preservation](validation/schedule-preparation-9.25/final-preservation.json) record the boundaries.

No installer, installation, commit, push or release is included. Listed/revised download callers still prepare eligibility and original-state comparisons synchronously before entering the shared wizard. File Info's session-validation and Download Later queue-prompt reads remain open, as do other Main/modal/shutdown paths in the architecture backlog. Continue those callers and controls before fresh installed browser/file-integrity and IDM timing comparisons; complete parity remains unproven.
