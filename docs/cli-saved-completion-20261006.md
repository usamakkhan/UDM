# Saved completion gate for CLI actions — 6 October 2026

A transfer can finish in memory while replacing `state.json` fails. The worker records the save error and continues running UDM, but the existing `/q` and `/h` checks considered only the in-memory `Complete` status. That could close the application or disconnect a dial-up/VPN connection before the completed download was durable in the catalog.

Both CLI actions now share one readiness check. It requires an inactive completed worker, an acceptable scanner result, and a matching completed record in the manager's last successfully written catalog snapshot. A pending action remains armed across a save failure. The existing periodic checkpoint retry can write the completion; the action becomes eligible only after that succeeds. This does not execute any actual RAS disconnection in tests.

The focused queue regression locks the test catalog against replacement. It checks the disk record and action state before and after a failed save, then releases the lock, retries the save and verifies exactly one `/h` delivery and `/q` readiness. Separate real-app CLI checks cover first-instance exit, forwarding boundaries, queued and failed downloads, and exact output bytes.

Validation passed: **40 focused native checks** and **10 isolated real-app CLI checks**. The desktop app compiled from the same changed backend objects. The real-app run used `UDM_TEST_HANGUP_GUARDS=1`, so paused and failed `/h` requests were checked without disconnecting any network connection. The focused result, app test result, build logs, source and binary hashes are retained under `candidates/cli-saved-completion-20261006`. A portable summary is at [validation/cli-saved-completion-20261006.json](validation/cli-saved-completion-20261006.json).

The change has not been installed or added to a release. Actual RAS disconnection, browser runtime, and a full native suite were not retested for this targeted change.