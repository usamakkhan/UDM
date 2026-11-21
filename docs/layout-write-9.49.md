# Nonblocking list-layout bookkeeping — UDM 9.49

Remove blocking acquisition from list-layout bookkeeping for every caller. A held-model sort command takes 1,997.3 ms on frozen 9.48 and 0.31 ms on 9.49; all 11 sort choices return in 0.25–0.40 ms in the controlled fixture. Preserve latest widths, order and sort through timer catch-up or normal Exit before catch-up. Build five native 9.49 products and pass 77 backend plus 1,390 native UI assertions. Both source browser manifests advance to 0.73.13; installed 8.68 remains unchanged. The prior matched total-time gate and installed IDM parity remain open.

## Confirmed wait

The sort-menu dispatcher updates its chosen column and then calls rememberLayout(). Before this increment, that helper defaults to a blocking model lock even though the main timer already supports a nonblocking attempt and normal Exit has its own final-layout preparation. A busy model can therefore freeze sorting before history refresh gets a chance to defer.

The expanded real-window fixture renders two records, selects a row and runs a history refresh under a separate model writer. Against frozen 9.48 App, MainFeatures, Version and cores, the first Arrange choice takes 1,997.3 ms and returns only after the two-second writer watchdog expires. The fifth assertion fails. Later baseline choices and geometry cases are not run after that failure. Current and baseline use identical fixture logic, with only the private App include redirected for the frozen build.

[Failing baseline](validation/layout-write-9.49/baseline/result.json), [frozen source and fixture proof](validation/layout-write-9.49/baseline/input-audit.json), [actual baseline version](validation/layout-write-9.49/baseline/product-version.json).

## Production behavior

rememberLayout now always captures the UI-owned column state before one nonblocking model-lock attempt. When the model is available, it replaces only Settings.ListLayout in memory. When busy, the controls retain the newest values. The next available timer captures them again. Normal Exit captures the latest controls in its existing owned final-layout stage and waits for the canonical Manager checkpoint before closing.

The helper no longer accepts a switch that can restore a blocking path. The timer and one existing modal fixture move to the no-argument API. The sort dispatcher needs no special-case fallback or new worker. The change adds no intermediate save, transfer pacing, disk-flush shortcut, cache or global mutex change.

Rows still wait for a coherent model snapshot before the requested sort is rendered. The improvement is prompt command return and retained layout intent during contention; it does not claim the entire history reorders while the model remains locked.

## Current native behavior

| Controlled operation | Before | 9.49 |
| --- | ---: | ---: |
| First identical sort command | 1,997.3 ms | 0.31 ms |
| All 11 current sort choices | Only the first baseline choice runs | 0.25–0.40 ms |
| Three width/order captures | Not run after baseline failure | 0.26–0.27 ms |
| Exit before layout catch-up | Not run after baseline failure | 10.47 ms to return to the UI |

Every current operation returns while the writer is still held and before its watchdog expires. The fixture checks each requested sort column, timer/paint responsiveness, unchanged catalog contents during contention, timer catch-up, selected-download identity, repeated geometry edits and the latest final layout. A separate mode exits before any timer catch-up, then verifies that the canonical final catalog retains the newest widths, order, descending sort and unrelated writer setting. The Exit return time is not full shutdown time; the fixture also waits for actual window destruction, Manager stop and final-state verification.

The previous 35 dispatch/export checks remain intact. Removing only the new modes and final assertions reproduces their exact 9.48 source hash. Reversing the modal fixture's single API-call edit also reproduces its old source hash; no modal assertion changes. The expanded dispatch fixture passes 80 assertions. Together with current menu, modal and Exit suites, 1,390 native UI assertions pass. 77 focused owner, policy and reader backend assertions pass. UI tests use separate owned desktops without switching the interactive desktop or injecting user input. Existing menu workflows still verify their real resumed files.

[Expanded layout/dispatch checks](validation/layout-write-9.49/layout-write-ui-checks.json), [unchanged-assertion proof](validation/layout-write-9.49/fixture-extension-audit.json), [all UI checks](validation/layout-write-9.49/ui-summary.json), [backend checks](validation/layout-write-9.49/backend-checks.json).

## Build and cleanup

Five normal native products are 9.49 / source browser manifests 0.73.13, built from 647 frozen inputs. Qualification adds one explicit test-source epoch after the first broad-menu run fails before Stop: the initial fixture called one nonblocking refresh and then tried selecting a newly added batch row. The revised fixture waits up to five seconds for a completed cohort containing both intended rows, adds a readiness assertion, and retains the original selection and Stop assertions. Its final count is 459. The failed 436-check attempt is retained and is not added to the current passing total. Only that test source recompiles; dependency auditing proves that production products and the other fixture binaries do not include it. Their original source inventories remain intact. Only the version-dependent Bridge core recompiles; the remaining 41 core objects preserve their verified identities. Normal products contain no UI-test define. Installed 8.68 / extension files 0.72.32, both personal catalogs, shared media helpers and previous qualified products remain unchanged. Remove 203 exact owned temporary files (495.3 MiB logical bytes) after sanitized evidence export. No setup package, installation, commit, push or release is created.

[Initial menu failure](validation/layout-write-9.49/initial-menu-failure/result.json), [test-source epoch and reuse proof](validation/layout-write-9.49/qualification-inputs-epoch-2.json), [build inventory](validation/layout-write-9.49/build.json), [five versions](validation/layout-write-9.49/product-versions.json), [cleanup](validation/layout-write-9.49/cleanup.json), [preservation checks](validation/layout-write-9.49/final-preservation.json).

## Remaining work

The Columns customization dialog still has synchronous initial settings and acceptance paths; this increment covers normal list-layout bookkeeping, not all settings dialogs. Remaining row/global commands, session-end behavior, recent UI-fixture build profiles and installed browser/IDM comparison stay open. The 9.47 matched total-time gate remains recorded as open. These contention fixtures do not establish universal timing or full inside/out parity.
