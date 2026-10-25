# Parity program: the decisions left (2026-10-24)

Everything in the parity program that a check in this repository can establish is done:
the GUI audit has no partial row left except intentional branding and the Images
category, the backend backlog's code items are closed, and the loopback and public
speed benchmarks against IDM 6.43 are level or ahead (`docs/speed-benchmark-2026-10-23.md`).
What remains cannot be decided by code alone. Each item below says what IDM does, what
UDM has today, what closing the gap requires, and a recommendation.

## 1. HTTPS response takeover by the network monitor

**IDM.** Its driver and browser modules take over downloads that IDM's extension did not
see, including HTTPS responses, because its browser integration runs inside the browser
and sees plaintext there.

**UDM today.** Browser-side capture is complete for Chromium and Firefox through the
extension (files, forms up to 64 MiB, HLS/DASH/SABR media, queues, exclusions). The
network monitor (`Udm.Network.exe`, WinDivert, documented in `signed-network-backend.md`)
captures plaintext HTTP on port 80 by default and hands authenticated candidates to the
desktop. HTTPS flows are visible as endpoints only.

**What it would take.** Reading HTTPS bodies off the wire means terminating TLS: a
UDM-generated root certificate installed in the user's Trusted Root store, the monitor
re-encrypting toward the origin, certificate pinning and HSTS failures handled, and an
honest security notice, since every program on the machine would then trust that root.
That is the antivirus "HTTPS scanning" design, with its known failure modes.

**Recommendation: do not build it.** The extension already sees every browser download
in plaintext, which covers the case IDM's takeover exists for. The remaining uses
(downloads started by other programs over HTTPS) are rare and are served by the
clipboard monitor, the Explorer verb, the `udm:` scheme and the command line. Record it
as a deliberate difference from IDM in the README rather than an open gap.

## 2. Live qualification of the driver path (VPN, HVCI, sleep and power, `.ef2` round trip)

**IDM.** Ships a Microsoft-signed driver that its installer loads on every machine.

**UDM today.** The WinDivert-based backend passed its 46 live checks with the vendor's
Microsoft-signed driver under normal Code Integrity, and the installer never loads a
driver, trusts a certificate or changes boot settings (`language-and-signing.md`). What
was never run: coexistence with VPN clients, Memory Integrity (HVCI) on, suspend and
resume with monitors active, physical network changes, and IDM's `.ef2` export imported
and re-exported through a live run.

**What it would take.** A dedicated test VM with elevation, Hyper-V or similar, a
snapshot to restore, and a short scripted matrix: HVCI on and off, a VPN adapter up and
down during capture, sleep and resume during a transfer, and the `.ef2` round trip. The
checks exist (`drivers/build-smoke.ps1 --run`, the live suites behind the backend
report); only the environment is missing. Roughly a day of machine time once the VM exists.

**Recommendation: do it before calling the network monitor a release feature, and
keep the monitor opt-in until then.** No code is blocked on it; the shipped default
(browser extension plus plaintext HTTP monitoring on port 80) is already qualified.

## 3. Code signing, updater and ARM64

**IDM.** Signed binaries and installer, built-in update check, x86 only.

**UDM today.** Unsigned x64 binaries and setup; `Help > Check for Update` opens the
project page rather than downloading anything; no ARM64 build.

**What it would take.**

- Signing: a publisher certificate (an OV certificate on a hardware token, or Azure
  Trusted Signing, which is cheaper and avoids the token) and `signtool` steps in
  `installer/build-setup.ps1` and `package.ps1` for the executables, the setup and the
  automation DLL. Half a day once the certificate exists; SmartScreen reputation then
  builds with downloads over weeks.
- Updater: a signed `latest.json` on the release host, a check on startup honouring the
  existing Tip-of-the-Day-style opt-out, download through UDM itself, signature check of
  the setup before it runs. Two to three days, after signing, since an unsigned updater
  is worse than none.
- ARM64: the native code has no x86-specific paths (the toolchain is cached per
  architecture in `.media-cache`); WinDivert has no ARM64 driver, so the network monitor
  would be x64-only. A CI runner or cross-compile target plus a smoke run on an ARM64
  machine. A day for the build, unknown for the acceptance machine.

**Recommendation: signing first, then the updater, ARM64 last.** The signing steps are
in place as of the same day (`docs/release-signing.md`): `package.ps1 -SignThumbprint`
or `-SignPfxPath`, a dry run, and an end-to-end check with a temporary self-signed
certificate; only the publisher certificate is missing. Signing is the one item
users hit on every install (SmartScreen), and the other two depend on it.

## Items that stay as documented limits

- Negotiate/Kerberos with an explicit login to proxies: needs a domain-joined machine to
  test; curl's SPNEGO path is wired. Left as a documented limit until such a machine exists.
- Bundled verified rule list: empty by policy; a UDM-owned rule is authored only when a
  public site is shown to need one.
- MMS: legacy protocol, out of scope.
- Images category: UDM's extra built-in category stays a user choice (removing it would
  re-route existing profiles).
- Title bar and product names: intentional branding differences.
