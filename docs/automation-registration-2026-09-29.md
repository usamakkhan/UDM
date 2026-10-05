# UDM automation: registration, installer and 32-bit follow-up

**Native 0.75 is still staged, not installed. Automatic COM activation remains unresolved.** This follow-up adds machine-wide registration and installer hooks, separately generated 32-bit/64-bit type libraries, and guarded registration/repair/removal. Installed UDM remains native 0.74 / browser 0.47.1.

The final automation binary passes **25 focused checks**: 19 actual per-user registration lifecycle/ownership checks, plus six assertions from real 32-bit and 64-bit PowerShell clients. Both clients connected to an explicitly started COM server, submitted one paused job each to a private catalog, and transferred zero bytes. This proves those dispatch calls across process bitness; it does not prove cold activation or the entire 32-bit API surface.

Registration now writes both type-library architectures. `--register --machine` and `--unregister --machine` target UDM's own machine-wide class/interface/type-library/ProgID keys. The installer packages both libraries and invokes these operations during setup/uninstall. This uses the [documented Windows class-registration stores](https://learn.microsoft.com/en-us/windows/win32/sysinfo/hkey-classes-root-key). No IDM registration is changed.

Repeated registration and unregistration succeed. Repair restores a missing ProgID. Registration and removal refuse a different installation's class or type-library path; type-library ownership is checked before class keys are removed. Invalid operation combinations are rejected. The retained tests cover actual HKCU writes, not merely a registry model. They do not establish machine-wide lifecycle behavior or rollback under every write failure.

The original 32 COM/transfer checks also passed after adding machine mode and the x86 library. Subsequent changes only tightened registration reads/ownership; the final binary was then exercised by the 25 focused checks above. Earlier 1,931 native, UI and protocol suites remain retained evidence; they were not rerun for these registration-only changes. Repeated runs are not counted as additional feature coverage.

## Activation investigation

- The actual test process is not elevated, not an AppContainer and has no package identity; UAC is enabled.
- UDM's registration is readable in both registry views, HKCR and the underlying user classes hive. The registered Automation proxy resolves successfully.
- Explicit IUnknown, IDispatch and UDM-interface activation all returned `0x80040154` before a server process was observed. Short commands, an AppID mapping and a shorter executable path did not fix it.
- After the x86 type-library addition, the cold native acceptance test still failed.
- A separate minimal native IUnknown-only server, using a fresh random CLSID and no UDM logic or type library, failed cold activation identically. This suggests a broader activation problem in this environment; it does not identify the cause or certify UDM's registration as correct on other machines.

The prepared machine-wide test requires Windows administrator consent. Windows reported that its prompt was canceled before the helper started, so **no machine-wide registration or client test ran**. A retry was requested and remains pending; it has not been retried. The helper is designed to register only UDM's own IDs, run clients against a private catalog, then remove its registration/processes.

## Package and retained state

The final `UDM-0.75.0-Browser-0.47.1-Setup-x64.exe` compiled successfully. All 124 package input files match their recorded hashes, including the final automation executable and both type libraries. The package uses the already deployed browser version; browser 0.49 remains a separate uninstalled candidate with its own recognition gate. This installer has not been executed or deployed.

All 235 baseline native-source files, deployed browser/network files and application receipt entries remain unchanged except authorized research documentation. Personal state is unchanged from the prior publication, including all 25 download records. A final read-only audit found all 28 checked UDM/test registry locations absent, no test processes, and the expired recognition certificate absent.

Remaining acceptance: automatic activation; real machine-wide setup/repair/uninstall and rollback; wider 32-bit calls/batches; live reference comparison; and combined browser-candidate qualification. Explorer integration and the wider IDM parity gaps remain open.

[Summary and package hashes](evidence-automation-registration-20260929/summary.json) · [registration checks](evidence-automation-registration-20260929/registration-2.json) · [32/64-bit results](evidence-automation-registration-20260929/running-crossbit-2/results.json) · [source diff](evidence-automation-registration-20260929/source-review.diff) · [minimal activation probe](evidence-automation-registration-20260929/minimal-activation-1.json) · [canceled administrator test](evidence-automation-registration-20260929/machine-1/results.json) · [cleanup audit](evidence-automation-registration-20260929/machine-cleanup-final.json)
