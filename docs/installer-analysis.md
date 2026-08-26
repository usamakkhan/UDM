# IDM setup and driver inspection

Inspected September 18, 2026. The user supplied `idman643build10.exe` and manually advanced its elevated wizard; the computer-use tool could read its pages but its input did not advance them.

## Setup identity and pages

- Version: **6.43.10.1**, 12,537,704 bytes; Windows Authenticode reported a valid Tonec signature.
- SHA-256: `A79C0E275750BD9B15F83F1B2D71F4A09EED11A9D903E534448B322DB2ED392B`.
- Welcome: install IDM; close browsers before continuing.
- Agreement: acceptance checkbox and Next. The user accepted it.
- Destination: `C:\Program Files (x86)\Internet Download Manager`, Browse, and **Create an icon for IDM on Desktop**, checked by default.
- Ready: **Start Installation of Internet Download Manager**; the text explicitly says the next Next click begins installation.

The user chose to proceed with installation to observe the driver. Afterward the wizard disappeared before a completion page could be captured. No driver-specific popup was captured. This was not a clean-machine installation and no low-level installer trace was collected, so the observed files cannot prove every operation attempted by setup or whether an unchanged driver was rewritten.

## Driver package and the active service

| Property | Verified value |
|---|---|
| Package configuration | `idmwfp.inf` |
| Driver version | **6.43.1.91**, INF DriverVer June 13, 2026 |
| Active x64 package binary | `idmwfp64.sys` |
| Installed binary | `C:\Windows\System32\drivers\idmwfp.sys` |
| Installed size | 187,064 bytes |
| Service | **IDMWFP**, Running, Automatic |
| Dependency | Base Filtering Engine (`BFE`) |
| Registry ImagePath | `\SystemRoot\System32\drivers\idmwfp.sys` |
| INF/registry service type | 2 (`SERVICE_FILE_SYSTEM_DRIVER`) |
| Signature | Valid; Microsoft Windows Hardware Compatibility Publisher |

The installed binary's SHA-256 is:

```text
8ACFFB0181146E96C44A94C5B364D657B775936EBB4D0FCCE591068F74803C4A
```

It matches the bundled x64 binary exactly. It also matches the baseline taken before the user started installation. The service remained Running with the same version and configuration in the bounded observation samples. These results identify the installed WFP driver, but do not establish that a different/new driver was added during this run.

The INF maps x86 `idmwfp32.sys`, x64 `idmwfp64.sys`, and ARM64 `idmwfpAA.sys` to the common installed name `idmwfp.sys`. It defines automatic startup, the BFE dependency, installation/removal sections and an accompanying `idmwfp.cat` catalog. Legacy TDI binaries `idmtdi32.sys` and `idmtdi64.sys`, version 6.32.3.80, are also present in IDM's folder. Only IDMWFP was found among the inspected IDM service registry entries; file presence does not mean the TDI drivers are active.

[Machine-readable inspection](idm-setup-analysis.json) and [bounded service/file samples](idm-install-driver-observation.json) preserve the evidence without copying the driver.

## Implications for UDM

The working browser-to-UDM flow uses native messaging and a restricted same-user pipe. Its transfer speed comes from actual HTTP range workers; a WFP driver was not involved in the measured UDM downloads. The observations do not establish that IDM's driver causes its download rate.

UDM now has an original compiled x64 WFP flow/TCP/UDP monitor and a bounded desktop-to-driver interface. Static analysis and INF validation passed. It remains unsigned and has not been kernel-loaded. Production signing, isolated VM validation, lifecycle/rollback testing and installer integration remain pending. See [drivers/README.md](../drivers/README.md).

UDM's existing installer remains a per-user PowerShell copy/protocol/shortcut script. It has no activation or trial-expiry step, but it does not yet reproduce IDM's graphical wizard, upgrade/uninstall flow or driver installation.
