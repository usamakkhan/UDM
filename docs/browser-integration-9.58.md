# Browser Integration responsiveness — 9.58



Open standalone Browser Integration before waiting for shared settings, with cancellable background preparation, save its owned draft on a joined worker, and merge only edited browser fields into current settings. Nested menu and panel edits preserve live sibling values; unchanged effective defaults do not write the catalog. The held-model opening observation improves from 2535.8 ms on frozen 9.57 to 48.5 ms on 9.58. Build five native products and pass 292 backend plus 80 native UI assertions. Source browser manifests advance to 0.73.22. Installed 8.68 remains unchanged; installed IDM parity and the earlier total-time gate stay open.



## Problem and implementation



The standalone Browser Integration command read shared preferences before constructing its dialog and saved an entire copied settings object after editing. A busy catalog could delay the dialog, and another writer's newer settings could be replaced by the older copy. The command now opens the same native form first, with editable controls disabled while a worker obtains its initial snapshot. Cancel remains available during preparation. A weak window wake and a 40 ms fallback pulse acknowledge results on the UI thread.



The existing dimensions, checkbox, address exceptions and Keys, Browser menus and Video panels buttons remain. An in-place explanation shows loading/saving, with Retry after preparation failure. Accepted Save disables editing and cancellation until acknowledgement; forced dialog teardown joins accepted work before local state or Manager can be destroyed. Failed Save retains the draft and displays the worker's validation error for correction and explicit retry.



`BrowserIntegrationTask.hpp` owns copied JSON drafts and results, never Windows controls. Reads use a cancellable try-lock wait and respect another operation's decision guard. Save acquires its accepted-work guard before dispatch; only the worker waits for the catalog and performs persistence. Acknowledgement consumes the owned result without reacquiring the catalog. The main command returns directly afterward, avoiding an unrelated refresh.



## Edited-field ownership



A fixed 21-field allowlist covers capture enablement and exceptions, special keys, browser menus, panel types and minimum sizes, placement and visibility, reset time, player choices and selected-link behavior. Foreign edits and removals are rejected. Boolean, string, integer and cross-field rules are checked before persistence. Full Manager validation and its existing transactional checkpoint remain authoritative.



The worker compares the effective initial snapshot with the edited draft and applies only that difference to the latest shared settings. Nested objects merge at edited leaves: changing Chromium Link preserves a concurrently changed Chromium All flag and Firefox choices. Video type flags and minimum sizes follow the same rule. Explicit deletion removes the selected entry while preserving other live entries. Concurrent edits to the same leaf use the accepted dialog's value. Invalid merged combinations fail rather than silently recreating a type another writer removed.



The snapshot materializes UI defaults and effective TTML compatibility. Unchanged OK returns a distinct unchanged result: no synthetic defaults are inserted, no concurrent settings are overwritten and no checkpoint is invoked. Positive active speed-limit normalization is prepared in a successful owned result consistently with Manager.



The existing `browserIntegration(CWnd*, Json&)` overload remains draft-only for Options. A child dialog must not commit settings before enclosing Options OK. Only the standalone command uses the new Manager overload.



## Native checks and controlled comparison



- 292 backend assertions across 12 suites, including scalar browser choices, nested merges, foreign/invalid edits, live cross-field conflicts, actual checkpoint rollback and retry, guard ownership, cancellation and joined lifetime.

- 80 assertions across eight native dialog scenarios: loading Cancel, read retry, successful Save, unchanged OK, child Cancel, child OK followed by parent Cancel, save retry and forced teardown after acceptance.

- Successful Save holds another catalog writer after persistence and verifies acknowledgement can return without reacquiring that lock. Read/save gates are bounded. Tests use fresh owned catalogs, not personal downloads.

- Separate Windows test desktops are child-attested and closed afterward. They are never switched to and inject no interactive mouse or keyboard input.

- Frozen 9.57 App and owning headers supply the previous blocking command. Opening is observed at 2535.8420 ms versus 48.4618 ms for 9.58 against the same deliberate 2.5-second writer. The baseline correctly fails the assertion that the dialog appears before the writer releases.



These single observations are not a latency distribution, installed IDM comparison, actual-download finishing measurement or proof of visual identity. The first baseline compile was invalidated by a fixture edit during compilation; its measurements were discarded and the frozen rerun supplies the retained comparison. The initial backend fixture omitted its first catalog save; that failure is exported separately and excluded from passing totals. Epoch 2 initializes the fixture and gives the form a useful validation error, with affected binaries rebuilt and dependency checks for retained binaries. Existing compiler warnings remain.



## Build, evidence and cleanup



Five optimized 9.58 native products are built: UDM, native browser host, monitor, setup helper and automation host. The support helper is not a new installer. Both source manifests advance to 0.73.22; no new extension runtime behavior is claimed for this desktop change.



[Sanitized validation records](validation/browser-integration-9.58/status.json) include 673 source hashes, dependency audits, core/product hashes, resource versions, backend and native UI results, baseline reconstruction patches and compiler logs. Private build wrappers remain local; this does not close the clean-checkout build portability gap.



Cleanup removed 118 task-owned temporary files (289.7 MiB logical size), preserving current and previous native products, all referenced shared objects, installed files, both personal catalogs and shared ffmpeg/ffprobe. No installer, installation, commit, push or release was performed.



## Remaining work



Installed app 8.68 and extension files 0.72.32 have not acquired these source improvements. Live browser/IDM comparisons, full visual parity and the previous 9.47 whole-background-work total-time gate stay open. Drop-target preference saves and some toolbar discovery still perform work synchronously. Startup profiling, actual completion timing and browser handoff comparisons remain necessary; this settings increment does not establish that UDM matches IDM in every case.

