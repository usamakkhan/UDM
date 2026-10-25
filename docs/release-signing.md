# Release signing (2026-10-24)

UDM's setup and binaries ship unsigned until a publisher certificate exists. The signing
steps are in place so that supplying one is the only remaining action.

## What signs what

| Artifact | Signed by | Notes |
|---|---|---|
| `UDM.exe`, `Udm.NativeHost.exe`, `Udm.Monitor.exe`, `Udm.SetupHelper.exe`, `Udm.Automation.exe`, `network\Udm.Network.exe` | `installer\sign.ps1` through `signtool sign /fd SHA256 /tr <timestamp> /td SHA256` | SHA-256 digest and RFC 3161 timestamp, so signatures outlive the certificate |
| The setup and its uninstaller | Inno Setup's `SignTool=udm` and `SignedUninstaller=yes`, defined by `build-setup.ps1` when a certificate is supplied | The `.iss` keeps both directives under `#ifdef SignSetup`, so unsigned builds compile as before |
| `WinDivert.dll`, `WinDivert64.sys` | Never | The vendor's Microsoft-signed files; `Udm.Network.exe` pins their hashes and re-signing would break that check |
| `Udm.Automation.tlb` | Never | A type library cannot carry an Authenticode signature |

## Supplying the certificate

Either a code-signing certificate in the user's or machine's `My` store, by thumbprint:

```powershell
.\package.ps1 -Version 1.0.5 -SignThumbprint <40 hex characters>
```

or a PFX file (the password by `-SignPfxPassword` as a SecureString, or in the
`UDM_SIGN_PFX_PASSWORD` environment variable for an unattended build):

```powershell
.\package.ps1 -Version 1.0.5 -SignPfxPath C:\keys\udm.pfx
```

`-TimestampUrl` changes the timestamp authority (default DigiCert), `-SignToolPath` points
at a particular `signtool.exe` (the cached SDK's one is found first, then installed Windows
Kits, then PATH), and `-NoTimestamp` is for offline test runs only. Nothing here creates,
imports or trusts a certificate.

Azure Trusted Signing works through the same path: its `signtool` dlib is passed with
`-SignToolPath` and the certificate selection with the thumbprint it exposes.

## Dry run

```powershell
.\package.ps1 -SignThumbprint <sha1> -SignDryRun
```

prints the signtool, the certificate (subject, thumbprint, store or PFX, expiry), the
timestamp authority, each release executable with its current signature status, and how
the setup will be signed. It signs and builds nothing. `installer\sign.ps1 -Thumbprint
<sha1> -DryRun` does the same for the binaries alone, and `installer\sign.ps1` without
`-DryRun` signs them without building a setup.

## Checks

`installer\test-signing.ps1 -RunRoot <new folder>` is the end-to-end check without a real
certificate: it creates a temporary self-signed code-signing certificate in the user store,
signs copies of the built executables, verifies each signature by signer thumbprint, shows
the dry run changes nothing and a second run is idempotent, exercises `-Force`, the PFX
path and a wrong password, checks the Inno definition and the sign set's exclusions, and
removes the certificate afterwards (17 checks). A self-signed signature verifies as
untrusted by design; what the check proves is that the pipeline signs with the supplied key.

## After the certificate

SmartScreen reputation accrues from downloads of a signed setup over weeks; an EV
certificate or Azure Trusted Signing starts with reputation. The updater
(`docs/parity-decisions-2026-10-24.md`) is the next item and depends on this one.
