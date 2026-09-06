# IDM deeper static analysis — 2026-09-20

This pass inventories 44 native files and analyzes five core components in Ghidra. It recovers concrete browser/driver communication, native media parsing, and download-allocation paths. The saved projects and address-based evidence make the findings reproducible.

This investigation uses the installed IDM files as read-only references. It analyzes machine code with Ghidra and reads the installed extension as text. Decompiled output is analysis pseudocode: original variable names, comments, source structure, and some types are unavailable. A successful decompilation is not proof that every inferred branch or structure is correct.

## Reproducible workspace

- Full PE inventory: `benchmarks/reverse-2026-09-20/inventory/pe-analysis.json` (44 EXE/DLL/SYS files).
- Input copies: `benchmarks/reverse-2026-09-20/inputs/`.
- Persistent Ghidra projects: `benchmarks/reverse-2026-09-20/projects/`.
- Function/call-graph indexes, string/symbol indexes and selected pseudocode: `benchmarks/reverse-2026-09-20/decompiled/`.
- Original analysis script: `benchmarks/reverse-2026-09-20/scripts/ExportReference.java`.
- Formatted extension and source hashes: `benchmarks/reverse-2026-09-20/extension/source-provenance.json`.

The reference copies, formatted extension, project databases and pseudocode remain under the excluded `benchmarks/` directory. They are not incorporated into the UDM executable or portable package. The source inspector now supports `--all --output <directory>`; the default ten-file report remains available.

Ghidra 12.1.3 was downloaded from the [official NSA release](https://github.com/NationalSecurityAgency/ghidra/releases/tag/Ghidra_12.1.3_build), with SHA-256 `93a5d11a9ad510622acaaf908c556a7b9b764d338e78a7567f3689bf5081fd54`. The first launch encountered an Apache Felix canonical-path error; using project-local application settings/cache directories resolved it. A related Windows/Java issue is documented in [Apache Felix PR 501](https://github.com/apache/felix-dev/pull/501). An initial exporter API-name error was corrected before re-exporting the saved analysis.

## Extension protocol: directly read from version 6.43.1

Line numbers refer to the formatted, hash-identified reference files in the workspace; minified original line numbers differ.

### Browser-to-desktop transport

`background.js:727–828` implements a connection cycle through `127.0.0.1:1001`, `0.1.0.1:1001`, and Chrome native messaging (`com.tonec.idm`). The WebSocket URL carries `cid` and a random value, and uses subprotocol `plugin.v3.internetdownloadmanager.com`. The connection-open timer is 1,500 ms. These are static paths, not a determination of which transport the running installation selected.

The serializer at `background.js:240` uses this frame grammar:

```
MSG#sequence#type#subtype#flags:positional-number:positional-number,field=value;
```

String fields carry an explicit UTF-8 byte length before a colon; numeric and boolean fields have direct numeric representations. Binary handling depends on the negotiated protocol version and transport. Native messaging carries a JSON string containing this grammar; it does not simply send a JSON object with named media properties. Both transports converge on the same parser. Pending acknowledgments are tracked by sequence number. No undocumented commands were sent to the installed application during this inspection.

### Native-provided site rules and media identity

At `background.js:1759`, incoming message type 18 installs a site profile. The receiver accepts domain information in fields 101/102 and processing parameters in fields 111–149. Selected fields become regular expressions according to the profile table. Profile 1 is explicitly associated with YouTube. In `content.js:181`, profile-specific initialization replaces selectors, regular expressions, player-script patterns and property paths supplied by the desktop.

The content layer reads matching inline scripts and selected page properties. Its YouTube-specific path can fetch a matching player JavaScript file and extract configured fragments; see `content.js:1046` and `content.js:1116`. This means observing the browser's current video request is only part of IDM's workflow. The extension also supplies information used to interpret available media. The native analysis below identifies format parsing and one explicit advertisement-state path; complete format-selection and advertisement-exclusion rules remain unresolved.

`document.js` instruments fetch/XHR and body-reader paths. Correlation identifiers carried in request markers connect extracted page response data with browser request events. `background.js:2573` recognizes markers such as `pxr` and `pfm`, linking these to records indexed by browser request ID. Those records also carry tab and frame identity, request method, redirects and associated page information.

At `background.js:2398`, the request serializer sends message type 13/subtype 1 with positional request ID, content-port ID, timestamp, status code, protocol, classification, tab ID and frame ID. Fields include URL (6), status line (12), request headers (11), response headers (13), redirect-related headers (18), request bodies (14/19), filename/type hints and page context. When headers are unavailable, separate referrer (50), cookie (51) and user-agent (54) fields can be supplied. This is source-level protocol analysis; no user's cookies or browsing payloads were collected.

The response classifier (`background.js:2264`) gates on status 200/206/304, MIME type, content disposition, filenames and native-supplied site rules. It recognizes manifest extensions such as M3U8 and MPD. One explicit ASF branch excludes the `DCLK-AdSvr` server header. This isolated rule does not establish universal advertisement filtering, especially for YouTube.

### Panel placement and player association

`content.js:369` assigns media-element IDs and maintains separate mappings to media identity and source URLs. It uses MutationObserver, ResizeObserver and IntersectionObserver. Geometry updates are debounced by 200 ms (`content.js:938`), and geometry calculation accounts for clipping ancestors, visibility, opacity, minimum dimensions and scale (`content.js:536`). Iframe association is handled separately (`content.js:520`).

The player-selection function (`content.js:452`) prioritizes matching identities/URLs, focus or pointer association and then suitable visible players. It considers video, audio, object and embed elements. Content message 41 announces media identity and initial geometry; message 23 updates visibility/geometry. The desktop receives this information. The inspected content script is primarily a player detector/geometry reporter; the native panel rendering path needs separate tracing.

## Comparison implications for UDM

UDM already has direct browser capture, current-video checks, adaptive media support, range subdivision and a C++/MFC desktop. Its current implementation should be compared against the details above, rather than treating the presence of a driver as a complete explanation.

- **Capture reliability:** preserve an explicit relationship between request, page/frame, media element, resource identity, headers, redirects and lifetime. IDM's richer handoff is evidence for this requirement, not a reason to copy its private wire format.
- **Placement:** compare UDM's viewport-based placement against clipped/hidden players, changing player size, iframe identity and overlapping UI. UDM's current `content.js` has no ResizeObserver/IntersectionObserver; its periodic scan can delay position updates.
- **Performance:** the prior live trace proved that IDM uses fewer, longer HTTP requests on the tested fixture. Native function analysis below determines which parts of the scheduling and transport code can be substantiated; switching libraries alone is not an established optimization.
- **Driver:** distinguish traffic observation, stream inspection, redirection and reinjection. UDM's current research driver has a deliberately smaller interface. A driver does not itself recover encrypted HTTPS URLs or implement the browser's media catalog.

No new throughput result is claimed by this static investigation. See `docs/stress-testing-0.10.1.md` and `docs/reverse-engineering-0.10.md` for existing runtime measurements and their limits.
## Native findings verified through functions and call sites

The saved analysis covers five components. Across these components, 862 selected functions have usable pseudocode exports, including library/runtime helpers. This is not a count of fully understood application functions. The exact coverage, timeouts and hashes are in [decompilation-2026-09-20.json](reference/decompilation-2026-09-20.json).

### The native host is a bridge, and the monitor includes media parsing

`IDMMsgHost.exe:0x402830` opens standard input/output and a session-specific `\\.\pipe\IDMNetworkMonitor.<session>` endpoint, sets up events and two worker threads, and relays data using overlapped I/O. `0x403410` and `0x4034A0` contain the read/write operations. This is now code-level evidence of the browser-host-to-monitor bridge, beyond the previous import-table inference.

`IDMNetMon.dll:0x10013680` binds a TCP listener to `127.0.0.1` on an automatically assigned port, reads the assigned port with `getsockname`, and uses `WSAEventSelect`. `0x10013740` creates the named-pipe server. `0x100338F0` parses the WebSocket handshake, including `Sec-WebSocket-Protocol`, `Sec-WebSocket-Key` and `X-IDM-ProcessId`; it converts the latter to a stored numeric value. The extension's public-facing port and this internal listener should therefore not be assumed to be identical.

RTTI identifies actual `YouTubeParser` and `YouTubeRecord` classes. The parser vtable is at `0x1006D8C8`, with eight recovered function pointers. Relevant paths:

| Function in IDMNetMon.dll | Direct code evidence | What this establishes |
|---|---|---|
| `0x10047400` | Searches HTML/script content for `ytInitialPlayerResponse` and older player configuration names | Native parsing of browser-supplied player data exists. |
| `0x100489B0` | Reads `videoDetails`, `videoId`, `formats` and caption data | A media catalog is built from supplied data and identity; the quality menu is not merely a fixed list of possible resolutions. Exact final menu assembly remains untraced. |
| `0x1004CE50` | Iterates format objects and interprets URL, itag, FPS, content length, initialization ranges and other properties | The native parser constructs format records from actual entries. |
| `0x10047940` | Tests `advideo` and `el=adunit`, sets an ad-state byte, passes it into subsequent format processing, and conditions a content-registration branch on it | At least this legacy path explicitly distinguishes advertising. The instruction sequence at `0x10047C19–0x10047C95` corroborates the branch. It is not proof of every modern advertisement path. |

This answers why reading only the extension was incomplete: meaningful media interpretation is in the native monitor library. IDM does parse and interpret media information; the lack of a visible resolving stage does not mean it does no resolution work. Static analysis does not measure when each operation runs or its latency.

`IDMVMPrs.dll` is separately structured: `Parse1` at `0x10002FC0` prepares a parser object, skips to XML markup and invokes a parser helper. `GetMPDParserInfo` at `0x10001ED0` returns two stored metadata words. Its exports do not make this DLL the sole location of YouTube logic.

### The WFP driver has an explicit integration contract

The driver entry at `idmwfp64.sys:0x14000F8A0` creates `\Device\IDMWFP` and `\DosDevices\IDMWFP`, registers process/image notifications, and installs an IRP device-control handler at `0x1400108B0`. The dispatch contains eleven recognized control codes in the `0x12C004–0x12C030` interval; unused values reach the default case.

`IDMNetMon.dll:0x10030D60` opens `\\.\IDMWFP`, queries the driver, and performs version/session negotiation. Matching client-side calls and driver switch cases establish the following byte-level interface observations:

| Control code | Observed client input / output bytes | Native call site |
|---|---|---|
| `0x12C004` | 4 / 24 | `0x10030D60` |
| `0x12C008` | variable / 0 | `0x100310E0` |
| `0x12C00C` | 24 / 24 | `0x10030D60` |
| `0x12C010` | 4 / variable | `0x10030FB0` |
| `0x12C01C` | 24 / 0 | `0x10031840` |
| `0x12C020` | 24 / 0 | `0x10014AF0` |
| `0x12C024` | 12 / 16 | `0x100149B0` |
| `0x12C02C` | 44 / 0 | `0x10031930` |
| `0x12C030` | 48 or 168 / 0 or 20 | `0x10031A50`, `0x10031B30`, `0x10017CE0` |

The driver additionally handles `0x12C014` and `0x12C018`. The table records observed callers, not an assertion that these are the only valid buffer sizes. The dispatch validates lengths and session-related state. Several inferred Ghidra types are wrong—for example, a request buffer is displayed as `IMAGE_DOS_HEADER`—so those generated field names must not be treated as a recovered ABI.

The most concrete browser-integration path is `0x14000EF40`: it recognizes a WebSocket upgrade aimed at `127.0.0.1:1001` or `0.1.0.1:1001`, constructs an `X-IDM-ProcessId` header, and sends the inserted bytes through its stream-processing path. This matches the header reader in `IDMNetMon.dll:0x100338F0` and the extension's endpoint choices.

`0x140010520` reads `RedirectAddr` and `RedirectPort` from the driver's Parameters key. The initialization function `0x140001AA0` supplies address value `0x00010001` and port 1001 as defaults. This evidence concerns local integration/redirection; it does not show HTTPS decryption or a download-throughput multiplier.

The exact GUID bytes were matched against the local Microsoft WDK 10.0.26100.0 `fwpmk.h`, with both input hashes recorded in [idm-wfp-layer-map.json](reference/idm-wfp-layer-map.json). Registration call sites in `0x140010170` establish:

- receive/accept authorization for IPv4 and IPv6;
- established-flow callbacks for IPv4 and IPv6;
- stream callbacks for IPv4 and IPv6;
- an IPv4 connect-redirect path on the newer-Windows branch;
- IPv4 authorization/outbound-transport alternatives on an older-Windows branch.

The registered stream callback is `0x140004150`. Stream reinjection calls are present at `0x1400195D0` and `0x140019770`; a transport-reinjection path is at `0x140003E10`. Therefore, IDM's driver does considerably more than UDM's current passive flow/byte-count monitor. Implementing equivalent integration would require original session/flow state, redirection, stream handling and a tested client protocol—not just installing a driver with a similar name.

### Download transport and range management

`IDMan.exe` exposes 19,604 functions to the analyzer, including statically linked runtime/library functions. Its selected transport paths now have decompiled control flow and disassembly:

| Address | Evidence |
|---|---|
| `0x5964D0` | Creates a nonblocking TCP socket. |
| `0x58B180`, `0x5AE640` | Build HTTP requests with bounded and open-ended Range forms, then send through plain or TLS-capable paths. |
| `0x59AC70` | Runs a select-based loop over active connection records, dispatches their processing, handles timeouts/restarts and reports progress. |
| `0x584490` | Contains a completed-connection path that can take over an adjacent chunk under conditions checked against the neighboring connection's progress/state. |
| `0x5F2DA0`, `0x5F1320`, `0x5EFFA0` | Send/read wrappers and dynamically bound TLS functions corroborate the custom socket/TLS path. |

The adjacent chunk-policy functions narrow the allocation logic further:

- `0x559960` traverses the chunk list, considers unfinished entries and retains the candidate with the largest computed available remainder. For the ordinary byte-offset branch, the calculation uses end, start, downloaded bytes and an additional per-connection term multiplied by the caller parameter; the complete meaning of that last term remains unassigned. This function sits next to the exception handler bearing `ChunksList::FindMaxFreeChunk`.
- `0x55ACB0`, adjacent to the `ChunksList::NeedNewConnection` exception handler, calls that selector and permits another connection when the candidate remainder is at least `2 ×` the stored 64-bit threshold at chunk-list offset `0xC0`, plus 3. A separate state flag permits an immediate positive result. The comparison is confirmed in assembly at `0x55AD03–0x55AD2E`.
- `0x559EA0` contains another plan-building path that clamps a computed size to `0x100000–0xA00000` (1–10 MiB), checks that sufficient room remains and generates consecutive ranges. The clamps are present in the assembly. This is not evidence that every IDM request has that size; the earlier live trace includes longer overlapping ranges that close early.
The reassignment path checks adjacency, reconciles byte accounting, closes other connection state and updates the chunk list. This is stronger evidence than the previous request-count observations. It still does not establish one universal split threshold or prove that every server/protocol uses the same policy. Some `recv` sites initially selected from the imports handle protocol-specific command framing, so they should not be called the universal HTTP body reader.

A progress calculation at `0x5BB350` also merits careful measurement: the assembly at `0x5BB50B–0x5BB51E` adds one tenth of a computed integer to itself, optionally applies a limit, then includes the value in progress messages (`0x1471`/`0x1394`). This proves that the reported value has an adjustment in this path. Correlating the fields with the exact visible speed label still requires runtime tracing; it is not sufficient evidence to say every displayed IDM speed is 10% high. Comparative benchmarks should continue using identical file bytes, wall-clock completion, and final hashes.

## What is complete, and what remains

This pass completes a full native-file inventory and a deeper, saved analysis of the main executable, browser host, network/media monitor, parser and x64 WFP driver. All 44 inventory entries parsed without errors. Each analyzed snapshot hash matches the corresponding installed file. One monitor function (`0x1001CA80`) exceeded the initial decompiler time limit; its disassembly remains available from the trace. Function extraction is not equivalent to complete semantic recovery.

The remaining work includes exact modern media-token/SABR handling, all advertisement paths, full chunk-scheduling decisions across server conditions, native panel rendering, complete IPC schemas, and runtime correlation of the recovered paths. The persistent projects and indexes provide starting points for those tasks without repeating the inventory.

This pass changes analysis tooling and documentation. The running UDM 0.10.1 release, driver installation, signing settings and Windows Test Mode were not changed. No new speed or full feature-parity claim is made.