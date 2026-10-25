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

Two routes. Both end in one `package.ps1` command; nothing in the repository creates,
imports or trusts a certificate.

### Route A: Azure Trusted Signing (recommended)

Microsoft issues a short-lived publicly trusted certificate for each signing operation
from a key it holds, so there is no token to keep, the subject is validated once, and
SmartScreen treats the result as an established publisher. Setup, in order:

1. An Azure subscription. In the portal create a **Trusted Signing account** (Basic SKU
   is enough; pick a region that offers it, for example West Europe or East US) and note
   its endpoint, `https://<region>.codesigning.azure.net`.
2. Under the account, **Identity validation > Public Trust**: submit the legal identity
   the certificate will name. Microsoft's availability for individual developers versus
   registered organizations has changed over time, so check the current rule in the
   portal; the validation itself takes from hours to a few days.
3. Once validated, create a **Certificate profile** of type Public Trust under the
   account and note its name.
4. Give the identity that will sign the role **Trusted Signing Certificate Profile
   Signer** on the account (Access control (IAM)). For signing from this machine that is
   your own user, signed in with `az login`; for an unattended build, a service principal
   whose tenant, client id and secret are exported as `AZURE_TENANT_ID`,
   `AZURE_CLIENT_ID` and `AZURE_CLIENT_SECRET` before `package.ps1` runs.
5. On this machine: the Azure CLI (`winget install Microsoft.AzureCLI`), the .NET 8
   runtime (`winget install Microsoft.DotNet.Runtime.8`), and the signing dlib from
   NuGet: `nuget install Microsoft.Trusted.Signing.Client -OutputDirectory
   .media-cache\trusted-signing` (or any folder; `-TrustedSigningDlib` points at its
   `bin\x64\Azure.CodeSigning.Dlib.dll`; the user's NuGet package cache is searched
   automatically). The cached Windows SDK signtool (10.0.26100) already supports `/dlib`.
6. Write `metadata.json` (keep it outside the repository):

```json
{"Endpoint":"https://weu.codesigning.azure.net","CodeSigningAccountName":"<account>","CertificateProfileName":"<profile>"}
```

Then:

```powershell
az login
.\package.ps1 -Version 1.0.5 -TrustedSigningMetadata C:\keys\metadata.json
```

The timestamp authority defaults to `http://timestamp.acs.microsoft.com` for this route.
Verification cannot pin a thumbprint, since the certificate changes: each signature must
be valid and chain from Microsoft's "Microsoft ID Verified" issuing CAs, which is also
how an already-signed file is recognised on a second run.

### Route B: an OV or EV certificate from a CA

DigiCert, Sectigo, GlobalSign and SSL.com issue code-signing certificates to a validated
organisation or individual. Since 2023 the private key must live on hardware: a FIPS USB
token the CA ships, or the CA's cloud HSM (DigiCert KeyLocker, SSL.com eSigner), whose
client software exposes the key to Windows as a KSP. Either way the certificate appears
in `Cert:\CurrentUser\My` with a private key, and its thumbprint is what UDM's scripts
take:

```powershell
Get-ChildItem Cert:\CurrentUser\My -CodeSigningCert | Format-List Subject,Thumbprint,NotAfter
.\package.ps1 -Version 1.0.5 -SignThumbprint <40 hex characters>
```

A token prompts for its PIN through the vendor's client (SafeNet Authentication Client for
most CAs) the first time signtool touches it; cloud HSMs authenticate through their own
client. Only a certificate that was issued as a software PFX (older issuance, or an
internal CA) uses the PFX route, with the password by `-SignPfxPassword` as a
SecureString or in the `UDM_SIGN_PFX_PASSWORD` environment variable:

```powershell
.\package.ps1 -Version 1.0.5 -SignPfxPath C:\keys\udm.pfx
```

### Common options

`-TimestampUrl` overrides the timestamp authority (DigiCert's for routes B, Microsoft's for
route A), `-SignToolPath` points at a particular `signtool.exe` (the cached SDK's one is
found first, then installed Windows Kits, then PATH), and `-NoTimestamp` is for offline
test runs only.

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
path and a wrong password, checks the Inno definition and the sign set's exclusions, builds
the Trusted Signing command line from fixture metadata and a placeholder dlib (credential,
dry run, Inno definition, refusals; no Azure call), and removes the certificate afterwards
(26 checks). A self-signed signature verifies as untrusted by design; what the check proves
is that the pipeline signs with the supplied key.

## Updater

UDM checks for a newer version at startup (at most once per 20 hours, unless "Check for
updates at startup" is off) and from Help > Check for Update. The check fetches
`https://github.com/usamakkhan/UDM/releases/latest/download/latest.json`, a signed
envelope: the base64 of a manifest naming the version and the setup (release asset URL
on this repository, SHA-256, size) plus an ECDSA P-256 signature over those bytes. UDM
verifies it against the public key built into `native\UpdateKey.hpp`; a build whose key
is all zeros verifies nothing, so the startup check stays silent and Help > Check for
Update keeps the release-page dialog.

When a newer version is offered the user can download it, skip that version, or
decide later; the download runs through UDM's own transfer with the manifest's SHA-256
as the expected checksum, so a file that does not match is never published. Before the
setup runs, UDM also requires a valid Authenticode signature chaining to a trusted root
(`WinVerifyTrust`, revocation not fetched); an unsigned or mismatched setup is left on
disk with the reason shown and is not started. The setup is launched with
`/CLOSEAPPLICATIONS` and UDM exits so it can be replaced.

Publishing a release:

```powershell
# tools\update-key.ps1 -Create was run on 2026-10-25: "UDM Update Signing" (key id da719ce21572edd7) is built into native\UpdateKey.hpp
.\package.ps1 -Version 1.0.5 -SignThumbprint <sha1>
tools\publish-manifest.ps1 -Setup installer-out\UDM-1.0.5-Browser-0.63.21-Setup-x64.exe -Version 1.0.5 `
    -SetupUrl https://github.com/usamakkhan/UDM/releases/download/UDM-1.0.5/UDM-1.0.5-Browser-0.63.21-Setup-x64.exe `
    -Notes https://github.com/usamakkhan/UDM/releases/tag/UDM-1.0.5 -Out installer-out\latest.json
```

Upload the setup and `latest.json` as assets of the same GitHub release. The private key
stays in the user's CNG store on the machine that ran `-Create` (non-exportable unless
`-Create -Exportable`); losing it means a new key and a new UDM build before clients
accept manifests again, so releases must be published from that machine.

Checks: the native suite's update checks sign manifests with an in-process key and cover
verification, tampering, foreign keys, the URL and field rules, the startup policy, the
HTTP fetch and the launch gate (an unsigned file, a hash mismatch and a missing file are
all refused). `installer\test-updater.ps1 -RunRoot <new folder> -Tests <Udm.NativeTests.exe>`
is the interop check: a temporary key from `update-key.ps1` signs a manifest with
`publish-manifest.ps1` and the native verifier accepts it and rejects tampering.

## After the certificate

With route B, SmartScreen reputation accrues from downloads of a signed setup over weeks
(an EV certificate starts with it); with route A the publisher is treated as established. The updater above depends
on it: until the setup is signed, the launch gate refuses to run a downloaded update.
