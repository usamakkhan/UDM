# IDM vs UDM backend differences and the closing plan

Date: 2026-10-16. Branch `content-recognition-phase-a` at 88735d5. Reference: IDM 6.43.
Scope: backend only (transfer engine, protocols, proxies, authentication, queues,
persistence, browser handoff backend, media backend, network monitor, automation,
CLI). GUI appearance is covered by the dialog comparison documents.

Sources: `idm-research-current-status-2026-09-29.md`, `backend-idm-gap-audit-20261006.md`,
`idm-research-gap-review-2026-09-29.md`, `idm-research-open-items-2026-09-29.md`,
`idm-feature-audit-2026-09-23.md`, `release-v0.87.0.md`, the capture-rule and
file-type plan of 2026-10-16, and the source at HEAD.

Status key: **M** missing, **P** partial, **U** present but unverified, **Done**.
"Fixture" says whether the repo's loopback servers can test it without external
services.

## Already present (not gaps)

Segmented, resumable HTTP with dynamic splitting; Basic and Digest login for servers
and explicit proxies; SOCKS5 login; per-server connection limits; segmented resumable
FTP (EPSV/PASV/PORT); PAC/WPAD resolution; Retry-After handling; quotas; wake timers;
sync queues; scanner verdicts; server-date timestamps; expected SHA-256; CLI switches
`/d /p /f /n /q /h /a /s`; content recognition with the web-page guard, extension
correction and category follow-up (R01); site capture rules (R02).

## Gaps

### Transfer engine and protocols

| Item | IDM | UDM | Where | Size | Fixture |
|---|---|---|---|---|---|
| Early content check | Stops on an error page after the first bytes | **Done** in this branch: the first segment is sniffed once it holds 5,000 bytes or ends | `Transfer.cpp` worker loop | S | Yes |
| Junk-prefix stripping | Removes a text/HTML prefix before the file (`exLenToSkip`) | **Done** in this branch: a detected text/HTML prefix is reported and, as a per-download opt-in, stripped before publishing with the file re-verified (`StrippedPrefixBytes`) | `Transfer.cpp`, `ContentSniff.hpp` | S | Yes |
| Rare-format recognition | Full libmagic-style database | P: UDM-owned signature table | `ContentSniff.hpp` | M | Yes |
| HTTP/2 → HTTP/1.1 fallback | Restarts without HTTP/2 after h2/TLS failures | **Done** in this branch: a transport failure during an HTTP/2 exchange (`InternetFailure`, curl body/response disconnect) retries once with HTTP/1.1 only and records `DisableHttp2` on the download; failures during request setup, before a protocol is known, still use the ordinary retry budget | `ProtocolFallback.hpp`, `Transfer.cpp`, `CurlHttp.hpp` | S–M | Decision logic and the HTTP/1.1-pinned path are tested; a real HTTP/2 failure needs TLS, so it is not |
| HTTP/3 | Unverified | M: only queried | `Transfer.cpp` | S | No |
| FTP qualification | — | **Done** in this branch: `FtpChecks.hpp` runs a loopback FTP server (EPSV/PASV, EPRT/PORT, REST, SIZE/MDTM, logins, one-time data cut) and an HTTP CONNECT proxy; the existing transport passed every scenario unchanged | `FtpChecks.hpp` | M | Yes |
| MMS | Supported | M, legacy | — | L | No |

### Proxies and authentication

| Item | IDM | UDM | Where | Size | Fixture |
|---|---|---|---|---|---|
| NTLM/Negotiate to servers | Integrated Windows login | **Done** in this branch: a 401 offering Negotiate/NTLM uses the saved site login for that scheme, or the current Windows login for intranet hosts (WinHTTP autologon policy MEDIUM, `WindowsLoginIntranet`); unanswered challenges are reported with their scheme for a manual login | `Transfer.cpp`, `DownloadPreview.cpp`, `Core.hpp` | M | Yes: `WindowsLoginChecks.hpp` runs a loopback Negotiate server through the Windows security API |
| NTLM / current-user proxy login | Supported | **Done** in this branch: a configured proxy login goes through the curl transport as Basic, Digest or NTLM (the vendored curl is now built with `CURL_WINDOWS_SSPI=ON` and `CURL_ENABLE_NTLM=ON`, since curl 8.22 leaves NTLM off by default); with no proxy login configured, a proxy that offers Negotiate/NTLM receives the current Windows login through WinHTTP when the intranet option is on. Negotiate/Kerberos with an explicit username is not offered: curl's SPNEGO sent no token for a non-domain login against the loopback proxy | `Transfer.cpp`, `CurlHttp.hpp`, `build-curl.ps1`, `ProxyLoginChecks.hpp` | M | Yes: `ProxyLoginChecks.hpp` runs a loopback forward proxy demanding Basic, Digest or Negotiate; for NTLM with a configured login the proxy validates the handshake through SSPI and, since the test login is not an account on the machine (`SEC_E_LOGON_DENIED`), accepts the NTLM authenticate message naming the configured user |
| Active FTP through proxy; FTP gateway modes | Supported | **Done** in this branch: `FtpProxyType` (Options › Proxy) selects CONNECT (default) or a gateway convention — USER user@host, SITE host, OPEN host — with an optional gateway login; active mode works through gateways, while CONNECT/SOCKS tunnels still require passive mode | `Ftp.cpp`, `ProxyPolicy.hpp`, `OptionsPages.hpp` | M | Yes |
| WPAD/PAC acceptance | Follows system auto-config | **Done** for scripts in this branch: `PacChecks.hpp` serves a loopback PAC script that the Windows resolver fetches and evaluates, checking route order (PROXY then DIRECT), DIRECT and SOCKS directives, 404/unreachable scripts failing without a direct fallback, a download through the script-selected proxy with the configured login, failover to the next alternative, and no direct fallback when the only proxy is down. WPAD discovery (DHCP/DNS) cannot be simulated on a loopback and stays U | `PacProxy.cpp`, `PacChecks.hpp` | S | Scripts yes; WPAD no |
| Browser PAC/auto-detect route for captures | — | **Done** in this branch: the extension reports a `pac_script` setting with a script address as `{type:"pac",pacUrl}` and `auto_detect` as `{type:"auto_detect"}` (inline script data and unknown modes stay unsupported); the desktop validates the script address (HTTP/HTTPS, length-limited, no control characters) and maps the route to "Use automatic configuration script" with that address or WPAD discovery, so `resolvePacRoutes` fetches and evaluates the browser's script itself. A configured proxy login is reused only when it was entered for the same script (or for auto-detect), the same rule as fixed routes | `chromium-proxy.js`, `BrowserProxy.hpp`, `BrowserProxyChecks.hpp`, `PacChecks.hpp` | M | Yes: a captured `pac` route downloads through the loopback script-selected proxy while the desktop's own HTTP override points at a closed port; a login configured for another script is not sent |

### Browser handoff backend

| Item | UDM | Where | Size |
|---|---|---|---|
| Stream rules (`V`/`A`/`C`) routed to media; `webRequest`-level `S` rules; diagnostics bundle | **Done** in this branch (see the capture-rule plan); a bundled verified rule list remains open | `browser/*/capture-rules.js`, `browser/*/background.js` | S–M |
| One-use POST/response after a browser crash | P (Firefox re-issues GET) | `CaptureTransactions.hpp`, `OriginalResponsePolicy.hpp` | M–L |
| POST bodies over 4 MiB, non-UTF-8 producers | P | `BrowserRequest.hpp` | M |

### Network monitor / driver

| Item | UDM | Where | Size |
|---|---|---|---|
| HTTPS response takeover | M; needs interception certificates, out of scope | — | L |
| New desktop process reattaching to a running helper | M | `Network.cpp`, `DesktopBroker.hpp` | M |
| Live elevated end-to-end, VPN/HVCI coexistence | U; needs elevation | — | — |

### Automation and CLI

| Item | UDM | Where | Size |
|---|---|---|---|
| COM automation server (send link, send batch) | **Done** in this branch, ported from the 0.75 candidate: `Udm.Automation.exe` (LocalServer32, per-user keys, type library from `Udm.Automation.idl`), `Automation.hpp`, pipe actions `automation-add`/`automation-batch`, the batch review dialog, installer `--register`/`--unregister`; `Udm.AutomationTests.exe` covers cold activation and an explicitly started server | `AutomationMain.cpp`, `Automation.hpp`, `AutomationUi.hpp`, `Bridge.cpp`, `App.cpp`, `installer/UDM-Setup.iss` | M |
| CLI forwarding of folder/paused fields to a running instance | **Done**: covered by `tests/launch.native-live.cjs` (cold launch and forwarded `/p` `/f` `--paused` keep the folder, name and paused state) | `Launch.hpp` | S |

### Scheduler, persistence, recovery, media

- Disk-full, removable-drive loss, sleep/wake, DST, power loss: U; only disk-full
  can be simulated locally.
- `.ef2` round trip with a real IDM: U.
- Live DASH/SABR/YouTube, encrypted media: P/M; `Adaptive.cpp`, `LiveHls.hpp`,
  `Streaming.cpp`; L.

## Closing order

Chosen for value and for being testable here, without drivers, certificates,
hardware or external servers:

1. **Early content check** (done in this branch).
2. **HTTP/2 → HTTP/1.1 fallback** (done in this branch; not exercised against a
   real HTTP/2 origin).
3. **NTLM/Negotiate login to servers** (done in this branch; see the table).
   Three things the loopback fixture showed and that the code and checks account
   for: WinHTTP completes the Negotiate/NTLM handshake for intranet servers by
   itself even with autologon policy HIGH (a raw single-send probe got 200), so
   the "off" option disables WinHTTP's automatic authentication for requests
   without a saved or proxy login instead; WinHTTP remembers an accepted login per
   host and port for the process; and loopback NTLM authenticates the calling
   account even when explicit credentials name a non-existent user, so "wrong
   saved login" is tested with a server that keeps rejecting completed handshakes.
4. **NTLM / current-user proxy login** (done in this branch; see the table).
5. **Loopback FTP fixture** (done in this branch; the transport passed unchanged).
6. **Active FTP through a proxy and FTP gateway modes** (done in this branch;
   gateway conventions are checked against the loopback server acting as the
   gateway, not against a real gateway product).
7. **COM automation server** (done in this branch). What the acceptance showed on
   this machine, and how the design accounts for it:
   - Per-user `LocalServer32` registrations are not consulted by the COM service
     here: with the keys present in `HKCU`, `HKU\<SID>_Classes` and the merged
     `HKCR` view, even a `LocalServer32` pointed at `notepad.exe` returned
     `REGDB_E_CLASSNOTREG`, while a per-user `InprocServer32` class loaded fine.
     This is the 0.75 candidate's unexplained failure; it is environmental, not a
     registration defect. The cold test records it and continues against an
     explicitly started server.
   - The running desktop hosts the class object itself (`AutomationHost` in
     `App.cpp`), so callers connect to UDM whenever it is running, with no
     registry lookup involved. Verified from PowerShell against an isolated
     `UDM.exe --background`: activation, `SendLinkToUDM`, `SendLinksArray`
     (encrypted at rest) and input rejection all worked.
   - The installer registers machine-wide (`--register --machine`, elevated) for
     cold starts; `Udm.Automation.exe --register` without `--machine` remains the
     per-user option.
8. **Capture-rule follow-ups**: stream routing, `webRequest` evaluation,
   diagnostics bundle (done in this branch).
9. **Junk-prefix stripping** (done in this branch, per-download opt-in).
10. **PAC script acceptance** (done in this branch for configured scripts;
    WPAD discovery is not testable on a loopback).

11. **Browser PAC/auto-detect routes for captures** (done in this branch: the
    desktop resolves the browser's script address or WPAD itself; inline script
    data stays browser-owned).

Still open and feasible here: a bundled verified rule list, the libmagic-style
signature stage, HTTP/3.

Not feasible under the current constraints: HTTPS takeover, live driver
qualification, publisher signing and the updater, ARM64, physical power and sleep
tests, public-site media parity, adopting one-use responses through browser APIs,
real dial-up/VPN acceptance.
