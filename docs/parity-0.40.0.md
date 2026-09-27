# UDM 0.40.0 verification

Native application, native host and monitor: **0.40.0**. Browser integration: **0.29.0**, unchanged in this release. The application is running from `D:\UDM\release\UDM.exe`. English-only remains the user's requested scope.

## Implemented in this release

- Options > Proxy > **Settings by protocol** selects separate HTTP, HTTPS and FTP routes, bypass hosts and encrypted logins. Omitted protocols inherit the default. A configured route replaces all proxy-specific fields, preventing credentials from an unrelated default route from being reused. HTTP redirects select the destination protocol's route. Invalid saved overrides fail validation.
- Passive FTP can tunnel both control and data connections through an HTTP CONNECT proxy. Includes remote destination DNS, separate proxy/site credentials, parallel segments, resume, retries and cancellation. PASV addresses do not redirect the proxy away from the original FTP host. Refused or malformed tunnels fail without switching directly to the FTP server.
- Virus checking settings recognizes **Microsoft Defender**, **clamscan** and **clamdscan**, offers editable recommended parameters and interprets their documented exit codes. Unknown scanners and modified parameters retain generic result handling. Defender exit 0 means no malware or successful remediation; it is not an unconditional safety verdict. Exit 2 can indicate a detection or a scan error.

The proxy/scanner implementations are original UDM source. Reference behavior was checked in the installed IDM Proxy/Socks dialog and [IDM Options documentation](https://www.internetdownloadmanager.com/support/options.html). Parameter/result mappings use [Microsoft Defender documentation](https://learn.microsoft.com/en-us/defender-endpoint/command-line-arguments-microsoft-defender-antivirus), [ClamAV usage](https://docs.clamav.net/manual/Usage/Scanning.html) and its [command manual](https://raw.githubusercontent.com/Cisco-Talos/clamav/main/docs/man/clamscan.1.in). CONNECT handling follows [RFC 9110](https://www.rfc-editor.org/rfc/rfc9110.html#name-connect).

## Verification performed

| Check | Result | Evidence |
|---|---:|---|
| Final native suite | 793 passed, 0 failed | [Native results](evidence/0.40.0/native-final-tests.log), [machine-readable](evidence/0.40.0/native-test-evidence.json) |
| FTP HTTP CONNECT, actual native transfer engine against local fixture servers | 24 passed, 0 failed | [Results](evidence/0.40.0/ftp-connect-final-results.json) |
| Protocol routing, actual native HTTP engine | 6 passed, 0 failed | [Results](evidence/0.40.0/protocol-proxy-final-results.json) |
| Existing FTP SOCKS4/4a/5 regressions | 31 passed, 0 failed | [Results](evidence/0.40.0/ftp-socks-live-results.json) |
| Actual native messaging host framing | 8 passed, 0 failed | [Log](evidence/0.40.0/native-protocol.log) |
| Chrome 0.29.0 extension → native 0.40.0 form download | 13 passed, 0 failed | [Results](evidence/0.40.0/post-chrome-final-results.json) |
| Edge 0.29.0 extension → native 0.40.0 form download | 13 passed, 0 failed | [Results](evidence/0.40.0/post-edge-final-results.json) |
| Actual UDM → installed Microsoft Defender, harmless downloaded text | 3 passed, 0 failed | [Results](evidence/0.40.0/defender-app-02-results.json) |
| Native UI review | Protocol switching and Apply persistence confirmed; scanner dialog visually reviewed | [Observation record](evidence/0.40.0/ui-evidence.json) |
| Installed app/host activation | Both report 0.40.0; original 23 records retained | [Activation](evidence/0.40.0/activation-result.json) |
| Existing Chrome and Edge sessions | Both popup connection checks report “UDM is connected and ready” | [Observation record](evidence/0.40.0/browser-activation.json) |

These are test-case counts, not a parity percentage. Local transfer fixtures exercise real native transport code but are not internet speed comparisons. The HTTPS routing test proves selection and failure isolation using a deliberately rejected CONNECT request; it does not qualify a successful TLS transfer through every proxy. SOCKS tests ran before the final added malformed-settings validation check; final CONNECT/protocol/native suites ran on the delivered native build.

Defender integration downloaded a new benign loopback fixture, verified its SHA-256, launched the installed signed MpCmdRun.exe with the preset, persisted the interpreted result and checked the same file hash afterward. No antivirus configuration was changed. ClamAV preset and exit mapping are covered by native tests; a real ClamAV installation was not exercised. GUI review was at this PC's current DPI, not an accessibility matrix.

Two test-harness issues were corrected before the final passing runs: the CONNECT fixture initially treated a Set of allowed ports as an array, and the Defender test initially expected a full executable path where the scanner result deliberately stores its basename. The failed development logs remain in the staging folder; final result files above come from clean reruns.

## Activation and recovery

Before replacement, the installed UDM had **23 records and 0 active downloads**. It exited through Tasks > Exit. Existing binaries and edited source files were backed up under the stage's `backup` directory before replacement. Source updates were guarded by their pre-edit SHA-256 values.

The saved catalog remains byte-identical:
`AAE5B9BF965F9850CC771232D988052FF5083FD809EA711B1520FB10A0ECC851`.

Installed package: `D:\UDM\installer-out\UDM-0.40.0-Setup-x64.exe`  
SHA-256: `8BE2D341625BC7C7F98698D94D9E8FD61F93A4074C1AA69622F87D1C2578A8AE`.

The setup compiled successfully and includes the previously verified signed network runtime. This is a locally built installer, not publisher signing or clean-machine install/upgrade/uninstall qualification. The driver itself is unchanged in this release.

## Remaining parity work

The [103-workflow inventory](idm-parity-0.40.0.md) records **88 implemented, 12 partial, 2 unverified and 1 deferred by the user**. “Implemented” denotes a working code path with stated evidence; it does not certify every IDM dialog or undocumented behavior.

- **Media and browser behavior:** cross-site panel coverage; unsupported YouTube sessions; live recording; nested/external DASH indexes; additional subtitle/audio paths; multipart/oversized/incomplete browser POST handling and ambiguous native crash recovery.
- **Networking:** browser-request proxy inheritance; FTP HTTP-gateway/PAC modes; active FTP through proxies; automatic driver-level capture and broader VPN/HVCI compatibility.
- **Downloader workflows:** physical sleep/hibernate wake; real disk-exhaustion/removable-device recovery; rendered authenticated site mirroring/exploration.
- **Product acceptance:** repeated matched-route IDM speed trials; complete GUI comparison; mixed DPI, screen readers and high contrast; publisher signing, updater and clean-machine installation/repair/uninstallation.
- **Deferred:** localization, because the user accepted English-only.

Full IDM parity is **not established** by this release. No universal speed or video-site equivalence is claimed.
