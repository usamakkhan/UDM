# Responsive listed scheduling and revision - UDM 9.27

UDM 9.27 / source extension manifests 0.72.91 moves the remaining listed-download and Edit download schedule entry preparation off the window thread. Before 9.27, Main's dispatcher validated command state and a deferred list selection synchronously; scheduleListedDownload then acquired Manager to check eligibility and collect the original fields before it entered the asynchronous shared wizard. A held writer could therefore prevent even the loading window from opening.

Main now checks local modal/shutdown/accepted-command guards, takes one strong cached selection with no model read, and opens the wizard. Menu eligibility remains advisory and nonblocking. The worker verifies that the selected object still belongs to the current catalog, checks listed or revision eligibility and collects its target, category, description, connection/speed defaults, seven original download fields and, for revision, the actual editable queue. Categories and editor preferences are captured under the same model lock. The caller reads no live job fields and returns from Cancel before an unrelated writer releases.

A typed preparation mode distinguishes File Info/new-download defaults, listed scheduling and revision. Listed and revised forms populate only from a successful current result. Revision's existing queue name, UTC start/stop anchors and option defaults are installed before controls are built. An ineligible or missing download remains in the loading/error form with navigation disabled, Retry and Cancel available. Loading and editing retain the modal shutdown owner.

The private original download and queue snapshots travel with the owned save draft. Accepted writes retain the canonical scheduleExistingDownload/reviseDownloadSchedule methods and their current eligibility, original-field and queue comparisons. Changes to the seven compared download fields or the revision queue are rejected; moving preparation off the window thread does not authorize stale saves. File Info and new-download callers retain their existing staged drafts and defaults.

## Validation

Five optimized native products report file/product version 9.27; Chromium/Firefox source manifests are 0.72.91. All 607 frozen repository inputs, required cores and product hashes are verified. Ten backend suites pass 536 assertions across twenty C:/D: runs. The preparation suite passes 33 assertions per drive, including coherent listed/revision context, current eligibility, UTC window retention and canonical rejection of stale download/queue snapshots. The earlier candidate's 33-assertion preflight is retained separately and excluded from those totals.

Owned Win32 controls pass 752 Main/Options, 257 pending File Info, 327 presentation, 46 first-paint and 239 wizard assertions (1621 UI assertions). Two new cases invoke the real Main command dispatcher with a selected row and historyRefreshDeferred set while a real writer holds Manager. They observe the loading wizard, disabled Next, enabled Cancel, pulse/paint and guarded Main close, then verify Cancel and command return before releasing the writer. This covers the dispatcher and deferred-selection gates as well as the shared wizard.

The wizard's eleven preparation cases include listed/revised Cancel, readiness and eligibility changed by the held writer, alongside direct/File Info preparation and explicit initial-error Retry. They verify that only successful reads construct editable controls, original listed fields are retained, and folder/date navigation and per-file defaults remain responsive under a second held writer. Eight further cases retain new/listed/revised/File Info successful save and real catalog-checkpoint failure/retry coverage, without duplicate queues or lost job identity.

No personal desktop or browser was controlled. Owned desktops were not switched to or given physical user input. These targeted correctness/responsiveness checks do not establish fresh installed browser/IDM timings, full backend-suite qualification or complete parity.

[Build](validation/listed-schedule-9.27/build.json), [backend assertions](validation/listed-schedule-9.27/backend-checks.json), [Main/Options](validation/listed-schedule-9.27/modal-checks.json), [pending](validation/listed-schedule-9.27/pending-checks.json), [presentation](validation/listed-schedule-9.27/presentation-checks.json), [first paint](validation/listed-schedule-9.27/first-paint-checks.json), [wizard assertions](validation/listed-schedule-9.27/wizard-checks.json), [source changes](validation/listed-schedule-9.27/source-change-audit.json) and [product versions](validation/listed-schedule-9.27/product-versions.json).

## Superseded candidate

9.26 / extension 0.72.90 builds five products and passes 536 backend plus 239 wizard assertions. Its Main suite stops at assertion 569, before invoking either new command: the fixture tried to select a newly added row while Main was hidden, so deferred history correctly kept the displayed list unchanged. 9.27 shows and restores the owned test frame and waits boundedly for its row before acquiring the writer. It retains the real dispatcher, deferred-selection and held-model assertions. Production scheduling code is unchanged from 9.26 apart from version constants. Preserve the candidate's build, backend, wizard and failed Main records separately; it was not installed or fully qualified.

[Candidate Main failure](validation/listed-schedule-9.27/superseded-9.26-modal.json), [candidate backend](validation/listed-schedule-9.27/superseded-9.26-backend.json) and [candidate wizard](validation/listed-schedule-9.27/superseded-9.26-wizard.json).

## Cleanup and next work

Removed 52 owned fixture/compiler roots and 65 temporary build files (666.0 MiB logical bytes). Preserve current products/required cores, qualified 9.25 products, installed 8.68 / extension files 0.72.32, personal catalogs and shared media tools. [Cleanup](validation/listed-schedule-9.27/cleanup.json) and [preservation](validation/listed-schedule-9.27/final-preservation.json) record the boundaries.

No installer, installation, commit, push or release is included. Next address File Info's synchronous session-validation and Download Later queue-prompt reads, including the queue selector's own preparation. Continue other Main/modal/shutdown paths in the architecture backlog, followed by fresh installed browser capture, complete-file checks and IDM timing comparisons. Full parity remains unproven.
