# Current batch commands and metadata ownership — UDM 9.38

UDM 9.38 / source extension manifests 0.73.2 closes the command-dispatch and superseded-result gaps recorded in 9.37. Disabling a native control previously did not stop Form from executing a delivered WM_COMMAND carrying its registered action ID. Batch list/change notifications could likewise reach their bindings during asynchronous work. Refresh results were applied by source index without first checking whether the request that produced them still matched the live draft.

## Reproduced 9.37 behavior

The new FormCommandUiTests component runs against every frozen qualified 9.37 repository input and linked core hash, plus that one test source. It creates actual owned native controls and delivers their command messages within an isolated Win32 desktop.

Fourteen assertions are recorded; eight fail. A disabled button runs both its ordinary and originless command. Another child can borrow its action ID. A disabled owning dialog still dispatches the button, disabled checkbox/static-link bindings still execute, and a destroyed child can invoke its remaining action binding with or without a notification origin. Enabled routing checks and normal close pass. This is a source/Win32 reproduction, not an installed IDM comparison or a claim that every such message naturally occurs from a physical click.

The corrected run uses exactly the same test bytes. [Baseline](validation/batch-command-9.38/excluded-preflight/baseline-9.37-command.json), [baseline input inventory](validation/batch-command-9.38/excluded-preflight/baseline-source-inputs.json) and [baseline scope](validation/batch-command-9.38/baseline-scope.json).

## Registered action dispatch

Form::OnCommand now checks the current child for a registered click binding. The child must still exist, the owning Form and child must both be enabled, and a supplied notification origin must be that current child. A rejected click is consumed without running its action.

An enabled existing child keeps originless command routing, which existing programmatic/shortcut paths use. Enabled native checkboxes and static hyperlinks retain normal routing. The change does not add a blanket filter to Form change bindings, or replace the other dialogs' OnOK/OnCancel implementations.

## Batch input ownership

ImportSelectionForm has an explicit blocked-input state for the interval in which refresh/preparation/admission owns its draft. Freezing records Cancel's current monotonically assigned control ID before disabling controls. Restoring clears the block before reenabling the saved control states; returning to idle recomputes highlight-dependent Edit availability.

While blocked, delivered command/change notifications other than Cancel are consumed. Checkbox, selection and column-click list notifications are also consumed. Owner-data display/find callbacks still run, so paint and virtual-cell reads remain available. Cancel routes through the existing task-specific cancellation guard; an accepted save keeps its original rule that it cannot be abandoned.

| Stage | Edit/selection commands | Cancel | View updates |
| --- | --- | --- | --- |
| Ready or inline Retry | Current enabled controls | Available | Normal metadata and view updates |
| Policy read, refresh projection or batch preparation | Blocked | Available | Paint/timers/display callbacks continue |
| Accepted save | Blocked | Save cannot be abandoned | Advisory publication remains deferred |
| Successful refresh or rejected/failed preparation | Restored | Available | Current metadata or inline Retry |

## Request identity before metadata publication

BatchRefreshProjection retains its source count, changed-slot descriptor and batch headers. Each prepared value already owns its address, explicit name and protected request overrides. batchRefreshMatches checks all of those against the current rows and headers, rejects out-of-range/duplicate identities and newly added rows, and requires a complete non-added bulk cohort or exactly the declared changed slot.

applyBatchRefresh validates the entire cohort before consuming any prepared metadata or moving any row field. A rejection leaves earlier rows unchanged and keeps the prepared evidence unconsumed. Destination, description and explicit checkbox changes are deliberately not request-identity mismatches; successful publication preserves those live fields.

The UI verifies ownership before attaching a returned preview. It discards a rejected bulk owner, or cancels only the rejected changed slot on the existing owner. Suppression remains in place, the virtual cache is invalidated so newer address/name fields are visible, and inline Retry is shown. It does not silently launch a new refresh. A requested Retry captures the newer draft and may then publish a matching result.

## Current-build validation

Five products report file/product version 9.38. Both source extension manifests are 0.73.2. The serialized build freezes 627 repository inputs and generated COM inputs, audits reused core dependencies, recompiles Bridge for its version dependency and verifies hashes after compilation.

All 838 targeted backend assertions pass across 34 C:/D: runs and 17 suites. The new apply suite contributes 21 assertions per drive. It covers changed URLs/names/protected context/batch authorization, source-count changes, added rows, invalid/duplicate/incomplete identities, wrong changed-slot descriptors, reordered valid result values, single-slot application and all-or-nothing rejection. Matching updates preserve destination/description/selection, and catalog/command ownership stay unchanged.

All 1,480 current UI assertions pass: 14 Form dispatch, 53 large-list/refresh, 261 loading/layout transitions, 190 opening/refresh/late-metadata, 131 admission and 831 Main/Options.

The expanded batch tests deliver both owned and originless selection commands while a real writer holds policy acquisition. They also deliver checkbox/column notifications and an edit-change notification to disabled controls. They verify that checks, row order and bindings remain unchanged, while pulse, paint and Cancel continue working. Preparing and accepted-save cases exercise queued selection attempts as well.

A test-only hook deliberately changes the owned fixture's address and explicit name immediately before publication. It is compiled only under the UI component-test define and is absent from the five normal products. The real dialog rejects that older result under the later held writer, keeps both sizes Unknown, shows the newer fields, preserves edited draft/checkbox values and exposes Retry. One explicit Retry then prepares the new request and completes. This is deliberate fixture supersession, not a personal-browser race reproduction.

The per-file browser edit still makes exactly one authenticated metadata request. All ten accepted authenticated files still match their complete payloads, with one transfer request each. Large batches retain actual virtual rows, source identity, selection/filter behavior and a 10,000-row refresh. All six owned UI desktops close without switching the interactive desktop or sending physical user input.

[Build](validation/batch-command-9.38/build.json), [inputs](validation/batch-command-9.38/dependency-inventory.json), [changes from 9.37](validation/batch-command-9.38/source-change-audit.json), [versions](validation/batch-command-9.38/product-versions.json), [backend](validation/batch-command-9.38/backend-checks.json), [dispatch](validation/batch-command-9.38/form-command-checks.json), [large lists](validation/batch-command-9.38/batch-large-checks.json), [transitions](validation/batch-command-9.38/batch-transition-checks.json), [refresh and supersession](validation/batch-command-9.38/batch-opening-checks.json), [admission](validation/batch-command-9.38/batch-admission-checks.json), [Main/Options](validation/batch-command-9.38/modal-checks.json).

These are targeted checks, not the entire backend registry, installed-browser validation, current IDM timings, full transfer/background-flush timing or proof of complete IDM parity. [Large refresh measurements](validation/batch-command-9.38/refresh-measurements.json) are individual local observed-hint runs.

## Preliminary boundaries

The baseline and developing-header tests link qualified 9.37 cores. Corrected preliminary checks pass 14 dispatch, 21 apply, 190 opening and 131 admission assertions, all excluded from the current-build totals. The early corrected Form snapshot predates the later cohort test hook and expanded batch tests; its raw inventory is retained rather than relabeled as the final 627-input snapshot. Its resource was still the baseline 9.37 resource. Current-build UI executables use the freshly compiled 9.38 resource and full frozen inventory.

[Boundaries](validation/batch-command-9.38/preliminary-boundaries.json), [corrected preliminary dispatch](validation/batch-command-9.38/excluded-preflight/form-command-preflight-ui-result.json), [preliminary apply](validation/batch-command-9.38/excluded-preflight/owned-backend-preflight/results.json), [preliminary refresh](validation/batch-command-9.38/excluded-preflight/batch-opening-ui-result.json), [preliminary admission](validation/batch-command-9.38/excluded-preflight/batch-admission-ui-result.json). The expected failing baseline, source inventories and build logs are retained alongside them.

## Cleanup and next work

Removed 44 owned fixture/compiler roots and 46 temporary build files (501.1 MiB logical bytes) after retaining sanitized evidence. Current products/required cores, qualified 9.37 products, installed 8.68 / extension files 0.72.32, both personal catalogs and shared media tools are preserved; no media helper is copied. [Cleanup](validation/batch-command-9.38/cleanup.json), [preservation](validation/batch-command-9.38/final-preservation.json).

No installer, installation, commit, push or release is included. Next scope imported per-file request edits to one preview slot: the current editor still calls full policy refresh for an import-row request change, while browser rows already use a single slot. Then continue auditing credential/validation work, very large sorting/filtering, exceptional teardown and Main/modal/shutdown joins. Actual owner/minimize/multi-monitor behavior, installed extension capture, complete media output and live IDM comparisons remain separate work. The original inside/out parity goal remains open.
