# UDM WFP network monitor — development build 0.6

The component identified in IDM is **WFP (Windows Filtering Platform)**. UDM now has an original x64 WFP driver, a bounded desktop/device protocol and a monitor client. The driver compiles, passes Microsoft C/C++ static analysis and passes `InfVerif /w`. **A separately test-signed driver and catalog are now available; neither has been installed, loaded or tested in the kernel.** The original unsigned build is preserved. This is a development component, not complete IDM integration.

## What was actually observed in IDM

| File | Version | Architecture |
|---|---|---|
| `idmtdi32.sys` / `idmtdi64.sys` | 6.32.3.80 | x86 / x64 |
| `idmwfp32.sys` / `idmwfp64.sys` / `idmwfpAA.sys` | 6.43.1.91 | x86 / x64 / ARM64 |

PE headers established architecture; Windows reported valid Authenticode signatures. The running IDMWFP service depends on BFE. Its installed System32 driver matched the bundled x64 binary by SHA-256. Imports include flow/process attribution, stream copying and stream/transport reinjection. These facts do not reveal its private browser/client protocol or prove which operations execute for a particular video. See [installer evidence](../docs/installer-analysis.md) and [deeper inspection](../docs/deep-inspection.md). No IDM code, binary, signature or catalog is included in UDM.

## Implemented in UDM

- `UdmMonitor.c`: six inspection callouts for IPv4/IPv6 flow establishment, TCP streams and UDP datagrams. Counts indicated bytes for explicitly watched process IDs, records endpoints/timestamps, and continues classification. UDP can include QUIC traffic; its content remains encrypted.
- `UdmWfpProtocol.h`: versioned, pointer-free IOCTL contract. A watch request is 144 bytes; a snapshot has a 64-byte header and at most 128 records of 96 bytes each. At most 32 PIDs can be watched. No payload, URL, cookie or kernel pointer is returned.
- The UdmWfp device is exclusive and restricted to SYSTEM and Administrators. Opening it does not start monitoring. An explicit watch list is required; closing the client clears the list.
- `src/NetworkMonitor.cs`: validates sizes, versions, records, addresses and timestamps; supplies a CLI and **Help > Network integration**. Without a driver, the desktop shows process-attributed Windows TCP connections and UDP local endpoints, including IPv6. Driver counters are not substituted for download progress or speed.
- `UdmWfp.inf`: demand-start, BFE-dependent x64 development metadata. The old `UdmWfp.c` is historical source and is not compiled.

The driver does not decrypt TLS, decode QUIC, supply media URLs, inject packets, block traffic or accelerate downloads. Browser observation and media transport remain separate parts of UDM. WFP alone does not fix the current live YouTube capture failure. Microsoft documents the classification layers in its [WFP sample](https://learn.microsoft.com/en-us/samples/microsoft/windows-driver-samples/windows-filtering-platform-sample/).

## Build and evidence

This workspace contains a private Microsoft toolchain cache; no Visual Studio or WDK was installed system-wide. The build uses MSVC **19.44.35229.0** and WDK/SDK NuGet packages **10.0.26100.6584**, with kit headers/libs under 10.0.26100.0. Microsoft lists [supported combinations](https://learn.microsoft.com/en-us/windows-hardware/drivers/other-wdk-downloads) and the [NuGet method](https://learn.microsoft.com/en-us/windows-hardware/drivers/install-the-wdk-using-nuget).

```powershell
# Current workspace's Microsoft toolchain cache:
.\drivers\build-driver.ps1 -Analyze
# Desktop, native host, monitor CLI and integration tests:
.\build.ps1 -Test
.\release\Udm.Monitor.exe --status
```

The driver build produces `drivers/out/x64/UdmWfp.sys`, symbols, INF and `build-evidence.json`. The report records source/binary hashes, compiler/kit versions, analysis, INF validation and signature status. /W4 /WX, /analyze, /GS, /guard:cf, /Qspectre, dynamic base and NX are enabled. Analysis excludes external Microsoft headers; it analyzes UDM's source. Static success is not kernel validation.

The three NuGet packages passed `nuget verify -All`. Compiler payloads passed their SHA-256 checks; cl.exe/link.exe have valid Microsoft Authenticode signatures, rechecked by the build. The downloaded channel manifest's advertised size/hash did not match the returned manifest, so that manifest is **not** counted as verified provenance. Payload/executable checks are separate. Downloaded compiler packages are not redistributed.

On another developer machine, install a supported Visual Studio C++/WDK environment and build UdmWfp.vcxproj with Release/x64. That MSBuild route and ARM64 configurations have not been executed here; only the cached x64 route above has passed. The INF deliberately offers x64 only.

## Client behavior and limits

`Udm.Monitor.exe --status` opens and validates the device protocol: exit 2 means absent, 1 means another failure, and 0 means available. After a separately signed driver has been validated and installed in a test VM, an elevated client can request a bounded sample:

```powershell
.\Udm.Monitor.exe --watch 1234,5678 10
```

Replace those example PIDs with test processes. The CLI emits JSON once per second and stops watching when it exits. The desktop selects up to 32 existing UDM/browser processes in the current Windows session, prioritizing UDM and processes with TCP connections. New browser processes require restarting monitoring or explicit CLI selection. Clients check start time to stop watching an exited/reused PID.

Only new flows established after selection receive contexts. Closed records can be reused; a full active table increments the dropped-observation counter. TCP counts are WFP-indicated stream bytes; UDP buffer lengths can include transport overhead depending on direction. They are not completed file bytes, exact wire totals or evidence of media URLs. Driver totals persist until unload. BFE restart recovery is not implemented: restart the driver and restore the watch list.

## Required before installation/release

Follow the [isolated VM validation plan](VM-TEST-PLAN.md). Callback lifetime, failure rollback, flow deletion and unload need runtime stress testing, Driver Verifier and compatibility review. Teardown waits for contexts and callout unregistration rather than unloading code while callbacks remain; the potential to hang on an unexpected WFP teardown failure must be tested.

The ordinary installer does not install the driver. UDM now has its own non-exportable development certificate in the current user's Personal store. No trusted-root store, signing-enforcement or Secure Boot setting was changed, and no UDM kernel service was created. The unsigned-development archive is retained. The new test-signed-development archive adds a signed catalog, public certificate, signature evidence and an opt-in live test executable. It does not include a private key or a Microsoft production signature. Release requires UDM's own publisher/signing process and the applicable [Microsoft signing requirements](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/kernel-mode-code-signing-policy--windows-vista-and-later-). Legacy TDI support is not implemented.
