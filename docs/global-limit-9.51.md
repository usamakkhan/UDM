# Speed Limiter settings responsiveness — 9.51 / extension 0.73.15

Five native production binaries are built. Speed Limiter settings now opens without first acquiring the shared catalog and saves without blocking its controls. Installed UDM remains 8.68, with installed extension files 0.72.32. No installer, installation or publication was performed.

## Interaction and ownership

Keep the established 302 by 89 dialog-unit layout, rate field, KB/s label, startup checkbox, reference warning, and OK/Cancel positions. While loading, disable incomplete fields and OK, keep Cancel usable, and show preparation status in the existing warning area. On success, restore the warning and choose the caption from the current limiter mode. A failed read offers a Retry button in the unused lower-left area.

The owned worker returns a settings snapshot; no controls or window pointers cross the worker boundary. Weak task notification wakes the existing dialog pulse. Cancellation wakes the pending read, and owner cleanup joins it without waiting for the catalog writer.

After validation, Save accepts an immutable rate/startup draft and reserves the existing decision guard before waiting for the catalog. Freeze all conflicting controls, including Cancel, until acknowledgement. Merge the edited fields into current settings while holding the model lock and use canonical durable persistence. In particular, preserve the latest enabled/disabled state and limiter mode rather than restoring the initial snapshot. This also preserves unrelated current settings.

Prepare the owned result before committing. Persistence failure rolls back the settings and leaves the user's edits available for explicit retry. After success, close using the result without reading the model again. Even forced modal closure joins an accepted save before destroying its owner. No fabricated progress or transfer rate is introduced.

## Qualification

134 backend checks passed across seven suites: 32 GlobalLimitSettingsTask, 25 ColumnSettingsTask, 11 ManagedModelWork, 17 ManagerModelWork, 19 MainExitLayoutTask, 14 MainClosePolicy and 16 DownloadMenuTask checks.

The new backend checks cover held-model read cancellation, duplicate work, another command's guard, accepted-save cancellation, weak notification failure, active and remembered rates, live on/off state after initial preparation, mode preservation, invalid and boundary rates, unrelated settings, a real durable checkpoint failure and rollback, retry, forced owner cleanup and publication after model unlock.

581 actual UI assertions passed: 66 Speed Limiter settings checks, 459 main-menu checks and 56 Columns checks. The limiter workflow exercises loading Cancel, injected read failure/retry, enabled and disabled edits, save failure/retry, and forced modal closure. A writer deliberately acquires the model after a successful commit, proving that UI acknowledgement does not reacquire it. Menu regressions include On/Off remembered-rate behavior and resumed byte-exact downloads.

Each current fixture used native desktop assignment and self-attested its own thread desktop before UI initialization. All requested and attested names matched; desktops were closed without switching to them or injecting physical user input. The historical launcher scope correction remains documented in [9.50](columns-9.50.md).

## Controlled comparison

The same final fixture on frozen 9.50 failed its expected visibility assertion: the dialog appeared only after the held writer released. Opening took 2,542.21 ms and the cancellation workflow 2,558.13 ms. On 9.51, the cancellation case opened in 52.37 ms and returned in 61.46 ms while the writer was still held. The other five cases opened in 33.35–47.89 ms. These are single controlled native-fixture observations, not a distribution or an installed IDM benchmark.

The baseline uses byte-verified frozen App, MainFeatures and Version inputs from 9.50. Only the fixture's App include is redirected; no product attestation include is added because frozen 9.50 already contains it. The private baseline directory retains a legacy 9.49 suffix, but its executable's verified version is 9.50. A private wrapper initially expected the old Columns assertion text; the completed baseline record and cleanup were valid, and the corrected expectation was checked against that existing record without rerunning the fixture. All current qualification uses one frozen source inventory, without production or test source overrides after compilation.

## Remaining work

The main-menu Turn On/Turn Off actions and other remaining synchronous preference paths still need separate responsiveness work. This increment changes settings-dialog ownership, not the transfer pacing algorithm, and does not establish real-network throttling accuracy. General build-profile integration for the standalone fixture remains open. Installed/browser IDM comparisons, the earlier matched total-time gate, and complete inside/outside parity remain unproven.

Sanitized source inventories, compiler records, assertions, baseline reconstruction, desktop evidence, product versions, cleanup and preservation records: [validation status](validation/global-limit-9.51/status.json).
