# Modal size refresh, 8.69 / browser 0.72.33

The main window's nested refresh path updated visible size cells behind a modal dialog by taking the Manager mutex with a blocking lock. An unrelated catalog save could therefore block the same UI thread that dispatches File Info and other modal controls.

`refreshModalSizes` now calls `tryModalSizeChanges`. A busy model returns no changes, leaving the real controls and cached text untouched. A subsequent refresh reads the current visible range and applies changed size text after the snapshot lock ends. The range is clipped against both lists without overflow; hidden history is not scanned. This is display refresh only: no Start, Cancel, persistence or other user command is discarded when a lock is busy.

## Build and deterministic evidence

Native definitions and all five product resources are **8.69**; both source manifests are **0.72.33**. App recompiles with the helper. Bridge is the only changed core object versus 8.68; Transfer and all other core hashes remain identical. [Build identities](validation/modal-size-refresh-8.69/build.json).

Ten backend suites on C: and D: pass **1,606 assertions**. The thirteen new assertions per drive hold the actual Manager mutex on another thread and prove the refresh returns while that hold remains. No formatter/live-row read or stale change is emitted; existing display data survives and later converges to the updated value. Tests also cover visible-range clipping, cached/live row-count disagreement and maximal/empty ranges. [All backend assertions](validation/modal-size-refresh-8.69/backend-checks.json).

The actual MFC main-window fixture passes **ten additional assertions**. It creates its own visible frame/list on a separate owned Windows test desktop, disables the frame as a modal owner, enters the production nested `refresh()` path and holds the catalog on another thread. The call returns before release and retains the real size cell/cache, unrelated text and selection. After release it updates the cell/cache and converges. The fixture exits normally and closes its desktop, without a desktop switch or mouse/keyboard injection. This is an owned component test, not automation of the user's browser or IDM. [UI assertions and source/binary identities](validation/modal-size-refresh-8.69/ui-checks.json).

The initial fixture compile failed with C3538 because one `auto` declaration mixed `std::future` and `std::shared_future`. It was corrected before the final build/runs; the failed receipt is not counted as validation. [Failure record](validation/modal-size-refresh-8.69/initial-fixture-build-failure.json).

## Delivery and remaining parity work

**8.69 is built and component-tested; installed UDM remains 8.68 / manifest files 0.72.32.** This step performs no installation, approval request, browser activation or new download-speed comparison. The unchanged Transfer hash preserves the 8.68 implementation but is not a fresh 8.69 timing result. Its recorded installed comparisons remain in [the 8.68 report](reception-hash-8.68.md).

This closes one nested main-window timer stall. Normal history refresh, navigation snapshots, authentication/review handling and other synchronous model-dependent commands retain separate lock acquisitions. A fresh personal-browser/IDM File Info timing comparison remains open. Next snapshot normal main-window read dependencies without retaining model locks across control messages or modal calls; state-changing commands must still validate current model state.

All twenty backend fixture roots and the owned UI fixture root were removed after preserving evidence. Twenty-two temporary test executable/object files were removed. Production/core hashes, installed 8.68 bytes and both personal catalog hashes were verified unchanged. No new download payload, setup, commit, push or release was created. [Cleanup](validation/modal-size-refresh-8.69/cleanup.json).
