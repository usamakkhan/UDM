# Browser requests during recovery — staged candidate

Recovery previously held the data lease without publishing a browser-facing status. Native requests could attempt to launch UDM and wait for an unavailable pipe. The candidate now publishes a per-user, per-instance Windows event while the recovery center or startup rollback owns recovery. The native host returns a retryable recovery message before reading the catalog, sending requests or starting another app. It rechecks after a failed pipe connection to cover a transition into recovery.

The event is owned by the process and released on exit. The data-folder lease remains the authority that prevents concurrent catalog use; the event is a status signal, not a replacement lock. A request already in flight during a transition is not proven to receive this message.

Verification:
- 25 actual app/native-host checks passed. Sixteen actions returned recovery-busy in 31–110 ms, with normal request IDs. The hello handshake remained available.
- The private catalog hash stayed unchanged, no extra candidate process appeared, and a host ping succeeded after Open UDM returned to the main app.
- 85 browser transaction module checks passed across Chromium and Firefox, including four new recovery-busy cases. Running browser downloads continue. Already-paused handoffs remain journaled and paused until the desktop becomes available; recovery then releases them to the browser without cancellation or native admission.
- These browser checks used the actual JavaScript modules in a harness, not a live Edge or Firefox session. Personal browser extensions were not reloaded.
- Installed application, host, monitor and personal catalog hashes remain unchanged.

Source overlay and binaries: D:/UDM-Workspace/candidates/native-084-backup.
Evidence: control/browser-busy-acceptance.json and control/ui-app-flow-browser-busy.json.
Build order: control/build-recovery-host.ps1, then control/build-app.ps1 (which now ensures the host/bridge build itself). Both use the staged Bridge object. Base native83 objects are untouched.

Not installed or packaged. Remaining work includes transition races with in-flight browser transactions, live browser acceptance, close/save failure handling, disk-full and power-loss testing, versioning and installer integration. Full IDM parity is not established.
