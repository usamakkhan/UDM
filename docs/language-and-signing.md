# Language evidence and driver test signing

## IDM implementation language

Read-only inspection of the installed IDMan.exe found an x86 native PE, linker version 9.00, a zero CLR/COM descriptor, and Microsoft C++ RTTI names including `.?AVCWinApp@@`, `.?AVCDialog@@`, `.?AVCWnd@@`, `.?AVCObject@@` and `.?AVCDialogBar@@`. These correspond to Microsoft's [MFC C++ classes](https://learn.microsoft.com/en-us/cpp/mfc/framework-mfc?view=msvc-170).

This strongly supports native C++ with MFC for the inspected desktop executable. It is a binary inference, not access to IDM's original source, and does not establish the language of every component. Its browser extension uses JavaScript. No proprietary source, driver or assets were copied.

UDM 0.7.0 now uses native x64 C++17/MFC for its desktop and download engine, C++ for its browser host and network monitor, JavaScript for the extension and C for its WFP driver. The native PE has a zero CLR/COM descriptor and static MFC/CRT linkage; it does not forward work to the retained C# implementation. Language matching does not supply missing browser session support or establish a speed advantage. See native-conversion.md for validation and remaining limits.

## Completed signing work

`drivers/sign-development.ps1` preserves the original binary and creates `drivers/out/x64-test-signed`. It signs the driver, generates its catalog through Microsoft's Inf2Cat, and signs that catalog using a non-exportable RSA development key. The public certificate and `signing-evidence.json` accompany the output. Both CMS signatures verified. No private key is exported or included in archives.

The public Windows trust store does not trust this self-signed development identity. This is expected and is recorded explicitly; this is **not Microsoft production signing**. No certificate was added to Trusted Root/Trusted Publishers. No boot setting was changed. Inf2Cat's own internal Microsoft signature also lacks a public trust chain; its executable was instead verified against the successfully signature-verified Microsoft WDK NuGet package before execution.

## Local readiness and pending tests

The read-only elevated check succeeded and reported administrator access, Secure Boot off, kernel Code Integrity on, Test Mode off, no installed UDM driver and no configured Hyper-V VMs. Administrator rights alone do not allow a locally test-signed driver to load under the current policy. Microsoft's [test-signing documentation](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/the-testsigning-boot-configuration-option) explains the separate boot policy.

Enabling Test Mode and restarting the PC is a distinct, disruptive next step. It has not been performed. It would allow nonproduction test-signed kernel drivers, not just UDM. Restore the original mode after testing; do not present a test-signed package as a public release.

`drivers/build-smoke.ps1` compiles `Udm.DriverSmoke.exe`. With no arguments it refuses to run kernel tests; its local TCP/UDP fixture was run successfully. After the driver is separately loaded in an appropriate test environment, `--run` checks empty scope, excluded-process traffic, IPv4/IPv6 TCP/UDP attribution, byte counters, scoped snapshots and exclusive-handle cleanup. These **kernel checks are pending**, along with unload/failure stress, Driver Verifier and the broader [VM test plan](../drivers/VM-TEST-PLAN.md).

The ordinary UDM installer does not install the driver, trust a certificate, enable Test Mode or restart Windows.
