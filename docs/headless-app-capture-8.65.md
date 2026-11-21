# Headless App capture runner, 8.65

`benchmarks/speed/capture_app_finishing.py` prepares the real MFC App/normal-core observation without desktop automation. Its default is read-only preflight. `--run` is additionally required to start a capture; it never opens an elevation prompt or changes the installed app. Desktop control remains stopped after the user's Escape interruption.

## Admission before any capture or payload

The runner checks the staged collector, qualified App entry/normal core and native observer identities. It requires a live filtered 8 KiB fixture made with that same collector, independently recomputes its selected-prefix analysis, checks its process/image and integrity receipts, and requires correlated physical flush completions inside both successful native fixture calls. A cached/staged `passed` flag alone cannot admit a larger run.

The worker must have matching trust, unexpired readiness, a live process and a creation time matching its ready record. Capture and origin space margins are checked. Only after admission does the controller create fresh marked roots. It requests the existing fixed collector, validates its hash and ready PID/parent, and then creates the loopback origin and background App child. It uses an isolated catalog, unique instance tag and suppressed dialogs, with no Windows UI input.

The existing App wrapper supplies a controlled MFC-entry boundary. The qualified native observer remains loaded until actual child exit. The completed file must match length/hash/catalog status and pass a separate exclusive post-exit flush. Kernel/native records and controller-gate/CRT boundary limits remain explicit; transport/integrity success is not labelled schema qualification, speed parity or a kernel cause.

## Failure and cleanup behavior

Process-aware record waits observe the original process or terminal outcome and tolerate a partially written record. They do not restart a request because observation times out. If hook setup fails before the download gate is released, the wrapper receives its abort byte rather than a newline that would start MFC. After a released entry, cleanup waits for real child exit and leaves the observer loaded until then.

Owned work cleanup verifies its root/marker, rejects reparse entries and hard links outside the tree, preserves bounded compact JSON metadata and removes the generated work tree/App copy. Internal hard-linked payload names are permitted only when their entire link group is inside the owned tree. No personal catalog, installed binary or ordinary IDM window is a cleanup target.

## Verification completed and still missing

- **23 admission/wait controls pass**, covering failed/stale fixture evidence, incomplete disk correlations, missing processes, terminal outcomes and partially written records. No real child is launched by these controls. [Results](validation/scheduling-filter-8.65/app-runner-policy-checks.json).
- **Five cleanup/abort controls pass**, including marker rejection, external-hard-link rejection, foreign-image rejection and correct internal-link cleanup with compact metadata preservation. One existing wrapper process exits with its expected abort code 5, prints READY without DONE and creates no App-return report: its MFC entry is not released. No origin or download starts. [Results](validation/scheduling-filter-8.65/app-runner-cleanup-abort-checks.json).
- The actual `--run` invocation **refuses admission** because no live filtered-fixture qualification exists. It starts no child, opens no approval and leaves the capture area's entries unchanged. [Preflight evidence](validation/scheduling-filter-8.65/app-runner-preflight.json).
- Small owned control files are removed after the checks. Source/compact receipts remain and the original wrapper binary is unchanged. [Cleanup](validation/scheduling-filter-8.65/app-runner-control-cleanup.json), [source identities](validation/scheduling-filter-8.65/app-runner-source-identities.json).

**Subsequent live captures completed.** A fresh approved worker, the 8 KiB filtered fixture and its linked disk-flush gates admitted two real 64 MiB App captures: sequential and eight connections. Both passed payload hash/catalog/normal-exit checks and removed their generated work trees/App copies. [Receipts](validation/scheduling-filter-8.65/live-app-captures.json). This exercises the positive path, but does not establish an App speed improvement, full-schema qualification or IDM parity. Source/native and browser versions remain 8.65 / 0.72.29; no installer or deployment is included.

The [App phase analyzer](app-finishing-phase-analysis-8.65.md) reproduces final-flush selection in all five retained App observations and joins the two subsequent live captures within its selected-prefix model. Live use exposed and corrected a controller error: the existing `exit_ticks` helper returns `None` for a running process, not zero. It also exposed a transient report-sharing denial; analysis resumed against the completed original capture rather than starting another fixture. The final performance requirement still includes comparable IDM behavior and all stated background/durability boundaries. The complete schema and a complete causal explanation of the long App flush remain unresolved; neither failed full-schema gate is waived by this runner.
