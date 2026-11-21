# Finishing through background work: 8.21

## Endpoint

The earlier first-exclusive-file-open observation can fall into a handle-free interval before final writeback, advisory SHA-256 and the final completion checkpoint. It does not establish that finishing is done. The new tracked [comparison runner](../benchmarks/speed/compare-completion.py) retains that observation as a secondary diagnostic and measures application process exit as the primary conservative endpoint.

Both applications receive `/d`, `/p`, `/f`, `/n`, `/q`. IDM documents `/q` as exiting after a successful download, for the first application instance; see [IDM command-line documentation](https://www.internetdownloadmanager.com/support/command_line.html). The runner requires no existing IDM instance in the current Windows session. It tracks the launched process plus any observed same-session IDMan processes, and leaves other sessions alone. Each process must be signaled exited before reading its Windows exit timestamp through [GetProcessTimes](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getprocesstimes). A precise system-clock/performance-counter mapping is checked for drift.

This includes all work in the observed application processes, final transfer/catalog work and automatic shutdown. It also includes each application's quit scheduling and shutdown overhead. It is a conservative completion bound, **not** the exact internal job-complete instant, GUI dialog visibility, or a measurement of antivirus/system services and hardware activity after the app exits. Windows desktop control fails to initialize with `failed to write kernel assets ... os error 3`, so a fresh live IDM dialog comparison remains unavailable.

Use three alternating cold-launch paired trials per case, deterministic in-memory loopback data, 8,000,000 bytes/s per connection, 20 ms response latency and configured connections 1/1/8 for 4 MiB sequential, 64 MiB sequential and 64 MiB split. Preserve and restore both IDM connection registry values and types after each run and in the outer finally. Do not hash payloads until the app exits. Verify the final file independently, and require UDM's final persisted Complete state and SHA-256 to match. Retain all outliers. UDM uses an empty private catalog; IDM retains its existing profile. Actual split request schedules differ.

## Baseline: installed 8.20

| Case | UDM median full-exit tail ms | IDM median full-exit tail ms |
| --- | ---: | ---: |
| 4 MiB sequential | 322.184 | 481.060 |
| 64 MiB sequential | 667.055 | 376.273 |
| 64 MiB split | 765.347 | 549.584 |

All 18 outputs and final UDM SHA-256 values match. Settings were restored. Chrome restarted during the baseline, so background activity is not isolated and the before/after difference must not be attributed entirely to the source change. [Full baseline](validation/finishing-8.21/baseline-8.20.json).

## Implementation

1. The worker's final transfer wake-up now checks CLI completion immediately instead of waiting exclusively for the 500 ms main timer. It still requires the job to be present, Complete, inactive, scanner-accepted and exactly represented by the saved catalog. Disabled owner, pending File Info or Options work defers the action. Reset the target only after successfully posting Exit. The timer remains a retry path. Early publication must not bypass final background work.
2. Separate the small sequential near-end writeback milestone from the half-body milestone. A 4 MiB response now requests its second flush at 3.5 MiB instead of consuming both requests at 2 MiB. A 64 MiB sequential response retains the 62 MiB near-end point. Operations remain bounded and coalesced; in-flight work joins before publication preparation, and required final writeback remains.
3. Record `TransferBackgroundTotal` and `CleanupParts` after the transfer's final writeback, advisory hash and parts cleanup. The former starts when the transport function returns; transport-worker/prefinish join time precedes it. The final Manager checkpoint and application shutdown follow it and are covered by the process-exit endpoint. Existing `Total` remains the earlier publication phase for historical compatibility.

Desktop 8.21, both browser manifests 0.71.85. Browser runtime JavaScript is unchanged. Native build/tests, installation and comparison results are recorded separately in [qualification](validation/finishing-8.21.json). No installer package or release is requested.

## Repeating the measurement

Run the tracked Python runner with a fresh C: output folder, `--label`, and the verified installed UDM SHA-256 through `--expected-udm-sha256`. Default application paths target the installed products; `--udm` and `--idm` can override them. It never terminates processes or deletes files. On timeout inspect the recorded process before retrying. Remove only verified owned payloads after retaining results. Do not use the runner's process-exit results as a substitute for a live completion-dialog timing test.

## Installed 8.21 outcome

All 1,763 focused native checks passed. All seven installed replacements are hash-verified and both personal catalogs are unchanged. The three-trial full-exit medians are 155.691/485.981 ms for UDM/IDM 4 MiB, 656.544/402.749 ms for 64 MiB sequential and 484.377/445.522 ms for 64 MiB split. All 18 output hashes and final UDM SHA-256 values match. The target remains unmet. All outliers are retained. [Results](validation/finishing-8.21/comparison.json).

The 4 MiB median improves substantially under the conservative endpoint. The remaining sequential 64 MiB trace shows 265.875 ms of transfer background work in one trial and 675.916 ms through exit, leaving roughly 410 ms for completion checkpoint/UI dispatch/shutdown. Code review identifies another forced catalog save when the Manager destructor calls stop after the window already stopped it. Version 8.22 addresses that duplicate only with actual-file, exact-snapshot and pending-outcome guards. It does not remove the first forced shutdown save.
