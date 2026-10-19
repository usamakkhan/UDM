# IDM Chromium proxy configuration trace — 2026-10-18

The previous browser/native comparison proves a Chrome-versus-WinHTTP routing
difference. It does **not** prove an IDM-versus-UDM difference for the same
failure history. This follow-up corrects that distinction before implementation.

## Reference callback probe

`tools/idm-research/probe-chromium-proxy.cjs` executes only the proxy settings
callback from the locally extracted, hash-pinned reference extension. It does
not load the extension, connect to IDM, or alter proxy settings. Seven checks
passed. The source is retained in ignored local research evidence, not copied
into UDM.

| Browser setting | Callback output |
| --- | --- |
| Direct | Mode 2 |
| Auto-detect | Mode 3 |
| System | Mode 4 |
| PAC URL | Mode 5 plus script URL |
| Mandatory PAC URL | Same mode 5 and script URL |
| Inline PAC | Mode 6 plus script text |
| Inline script longer than 50,000 characters | Mode 4 |

These are observed protocol values, not proposed UDM protocol identifiers.
The inspected request serializer passes this configuration through field 122
for Chromium; its Firefox branch uses request `proxyInfo` instead. The callback
does not encode the PAC `mandatory` flag. This does not establish all desktop
error/fallback behavior or imply that UDM should discard that flag.

Reference `background.js` SHA256:
`3590408894c701937ffa51048c14c3e6d1b534e9616b76d99e4af523925294d7`.

## Native trace

The existing full Ghidra export for the installed executable identifies
`0x004b8810` as a handler that invokes function pointers resolved from
`InternetInitializeAutoProxyDll`, `InternetGetProxyInfo`, and
`InternetDeInitializeAutoProxyDll`. A later branch loads WinHTTP entry points,
including `WinHttpGetProxyForUrl`, for a script-URL lookup. This establishes
native PAC evaluation capability, not which branch a particular live Chromium
download follows. The original assembly/export remains in ignored research
evidence. No native source was copied into UDM.

Installed executable SHA256:
`03cc62e9adb77a380f9dc12f67ccaaee5106f12844aa73ce32c914ddd16d607c`.

A fresh REA MCP run successfully opened the installed executable and confirmed
that same hash. Its `analyze_function` request for `0x004b8810` timed out after
240 seconds; the client then exited. The native findings above rely on the
existing hash-matched full Ghidra export, not a successful fresh REA decompilation.

## Implementation direction and remaining checks

The existing native `--pac-checks` suite was also run: **10 passed, 0 failed**.
It covers script fetching, PROXY/DIRECT/SOCKS results, real proxied downloads,
unreachable-first-proxy failover, and errors without an invented direct route.
This verifies the desktop URL-PAC foundation, not browser configuration handoff
or inline scripts.

UDM currently rejects Chromium PAC/auto-detect capture and supports separately
configured desktop PAC URLs. Add an explicit browser **configuration** contract,
distinct from its existing request-bound endpoint contract. It needs URL PAC,
inline PAC, and auto-detect variants, capability negotiation with older desktop
versions, encrypted persistence, configuration-change invalidation, and tests
for script errors, cancellation, redirects, and proxy fallback. Inline scripts
must run in a bounded PAC resolver rather than the extension or application
JavaScript context. Do not claim exact browser-route preservation from this
contract.

Live IDM acceptance under an isolated PAC setup remains required before
claiming matching fallback policy. Chromium's configuration API and Firefox's
request route API are different inputs and must stay distinct.

Sources: [IDM proxy options](https://www.internetdownloadmanager.com/support/using_idm/options.html),
[Chrome proxy API](https://developer.chrome.com/docs/extensions/reference/api/proxy).
