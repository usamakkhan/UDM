# Main command selection scope — UDM 9.48

Avoid validating an unrelated selected download before dispatching Exit or local view commands. Preserve live row-action validation and selection-aware export. The same native Select All fixture blocks for 2,002.6 ms on frozen 9.47 and returns in 0.2 ms on 9.48 while its model writer is still held. Quick Filter returns in 27.5 ms and selected/deferred Exit in 12.3 ms. Build five native 9.48 products and pass 77 backend plus 526 native UI assertions, including all three real export entries and byte-exact resumed downloads. Both source browser manifests advance to 0.73.12; installed 8.68 remains unchanged. The prior 9.47 total-time gate and installed IDM parity remain open.

## Confirmed dispatch wait

After a busy history refresh, the existing dispatcher called selected() even for commands that never use selected downloads. With a selected row and historyRefreshDeferred, that call takes Manager's model lock to remove obsolete jobs. Exit therefore could block before reaching its responsive layout stage. Select All and Quick Filter inherited the same unnecessary validation.

The regression fixture renders two saved records, selects one through the real native list, starts a separate catalog writer and calls the real refresh path. It requires that refresh to defer without changing the selected row, then sends an actual command. Against frozen 9.47 App, Version and cores, Select All returns after 2,002.6 ms, only after the two-second writer watchdog expires, and fails the fifth assertion. That baseline stops there: its Quick Filter and Exit times are not measured by this run.

The first compile exposed a fixture name collision, which was corrected before the frozen current build. A later baseline verifier initially compared the frozen App with the advanced working tree. The successfully linked 9.47 executable was retained, its original source/version/core hashes were verified directly, and the observation resumed without recompiling or relabeling the old binary. Both diagnostics remain documented.

[Failing baseline](validation/command-dispatch-9.48/baseline/result.json), [frozen input proof](validation/command-dispatch-9.48/baseline/input-audit.json).

## Production change

The dispatcher now captures and validates selected jobs only for commands that consume them. Existing commandUsesSelection classifications retain live validation for row actions. Export, IDM export and text export explicitly retain selection capture, because they accept a selection without requiring one for command availability. Dynamic queue actions retain their separate dispatch path.

Local view commands no longer perform unrelated model validation. Exit can reach its existing asynchronous layout owner even when a displayed row awaits history reconciliation. No catalog, download, locking primitive, final-save policy or worker lifetime rule changes in this increment. Other commands that need current model state can still wait; full UI responsiveness is not claimed.

## Native behavior checks

| Actual command | Current return time | Writer still held at return |
| --- | ---: | --- |
| Select All | 0.19 ms | Yes |
| Quick Filter | 27.52 ms | Yes |
| Exit with selected row and deferred history | 12.30 ms | Yes |

Select All changes the native selection from one to both rows. Quick Filter shows its controls while the model is busy. Exit reaches its owned pending stage, services its timer, completes shutdown exactly once after release and preserves the other writer's setting in the final checkpoint. Export is tested by sending all three actual commands: each native dialog contains exactly the selected filename, selects the matching scope and requested format, and cancels without changing either record.

The existing selection-command fixture performs a real loopback Stop/Resume and verifies every final byte. Existing menu workflows retain their independently checked resumed files. Targeted owner, close-policy and menu-reader backend suites also pass. In total, 77 backend and 526 native UI assertions pass. UI fixtures use separate owned desktops without switching the interactive desktop or injecting physical input.

[Dispatch and export assertions](validation/command-dispatch-9.48/command-dispatch-ui-checks.json), [selection workflow](validation/command-dispatch-9.48/selection-commands-ui-checks.json), [all UI suites](validation/command-dispatch-9.48/ui-summary.json), [backend assertions](validation/command-dispatch-9.48/backend-checks.json).

## Build, cleanup and remaining gates

Five native products are built from 647 frozen inputs as 9.48 / browser 0.73.12. Dependency analysis recompiles only the version-dependent Bridge core; the other 41 core objects retain their verified identities. Normal products have no UI-test define. Installed 8.68 / extension files 0.72.32, personal catalogs, shared media helpers and prior qualified products are preserved. Owned fixture/compiler outputs are removed after sanitized evidence export. No setup package, installation, commit, push or release is created.

[Build inputs](validation/command-dispatch-9.48/build.json), [five product versions](validation/command-dispatch-9.48/product-versions.json).

Cleanup removes 130 owned temporary files (467.9 MiB logical bytes). [Cleanup record](validation/command-dispatch-9.48/cleanup.json), [installed/catalog/helper preservation](validation/command-dispatch-9.48/final-preservation.json). The focused dependency-aware harness validates these source fixtures; standard build profiles do not yet expose every recently added UI fixture.

This fixes a selected/deferred dispatch wait, not the earlier controlled empty-selection total-time gate. The 9.47 matched batch's 3.2 ms difference remains recorded along with its writer-hold variation. No installed IDM timing or universal inside/out parity is established here. Remaining blocking row/global actions, modal and hardware/browser behavior still need work and direct comparison.
