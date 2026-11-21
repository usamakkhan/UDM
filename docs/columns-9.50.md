# Columns responsiveness — 9.50 / extension 0.73.14

The Columns dialog no longer waits on the catalog before opening or blocks the interface while saving. Five production binaries are built. The installed app remains 8.68, with installed extension files 0.72.32; this increment was not installed or published.

## Behavior

Initial preparation runs on an owned worker. Incomplete controls remain disabled while Cancel stays usable. Hidden widths come from the settings snapshot; visible widths and ordering come from the current list. Read failure exposes Retry without closing the dialog.

Save validates a complete immutable draft, reserves the decision guard, and merges only column settings into the current catalog. Conflicting controls freeze after acceptance. Persistence failure restores model state and retains edits for retry. Successful application uses the owned result without acquiring the catalog again. Forced modal closure joins accepted work and applies a committed result, preventing later list bookkeeping from overwriting it.

## Evidence

102 backend assertions passed across six suites, including 25 dedicated Columns task checks. 1,346 native UI assertions passed: Columns 56, main menus 459, modal workflows 831. They exercise real controls, cancellation, injected read/save failures, durable rollback, unrelated-setting preservation, held-model result application, and forced closure.

The frozen 9.49 control failed the same visibility assertion as expected: opening took 2,585.81 ms while a catalog writer held the model. The current cancellation case opened in 61.70 ms and returned in 71.53 ms before that writer released. Other current cases opened in 48.49–56.88 ms. These are controlled native fixtures, not installed IDM benchmarks, and no claim of zero latency follows.

The initial current Columns fixture passed 56 checks. Two test-only source epochs changed the inspection timer owner and queued failure cleanup; assertions and production binaries stayed unchanged. Final Columns and menu runs use epoch 3; modal qualification uses epoch 2, whose only different source is the unused Columns fixture. The completed baseline uses the final fixture with only its App include redirected and a test-only desktop-attestation include added to frozen App source.

## Desktop-launcher correction

Previous private Python launchers assigned an arbitrary lpDesktop attribute to subprocess.STARTUPINFO. That does not populate the native Windows startup structure. Earlier separateDesktop fields are evidence of intent, not verified isolation. Historical assertion results remain recorded, but claims of isolated execution and hardware timing require revalidation.

The replacement launcher calls native CreateProcessW with an explicit desktop. Each test queries its own thread desktop before UI initialization and writes a restricted owned attestation file. All current runs and the standalone launcher probe matched their requested desktop names. No desktop switch or physical input was used. Foreign-thread desktop handles are not used as proof.

Earlier baseline hangs included synchronous assertion-failure cleanup during dialog initialization. Queuing WM_CLOSE resolved that fixture failure path; those hangs do not establish a product deadlock. Private dumps are excluded from tracked evidence and deleted during cleanup.

## Remaining work

Installed/browser IDM comparisons, the prior matched total-time gate, and complete inside/outside parity remain open. The new standalone fixture uses the private dependency-aware build path; general build-profile integration remains to be completed. The extension increment keeps source versions aligned; this change adds no new browser capture behavior.

Sanitized evidence, source inventories, compiler logs, desktop-scope correction, source epochs, baseline, preservation and cleanup records: [validation folder](validation/columns-9.50/status.json).
