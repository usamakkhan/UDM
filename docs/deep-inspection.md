# IDM component and layout inspection — 2026-09-18

The installed reference is IDM 6.43 build 10. This inspection reads local files and observes its windows; it does not modify the reference installation. UDM 0.6 implements original controls using the measured layout. It is not a complete replacement yet.

## Reproducible evidence

`tests/inspect-reference.cjs` reads ten PE files without loading or executing them. [The metadata report](reference/pe-analysis.json) records each file's SHA-256, sections, imports, exports, resource counts and dialog control geometry. It decoded **105 main-executable dialog templates with no parser errors**. Existing [component inventory](installed-components.json), [signature metadata](binary-metadata.json), [installer findings](installer-analysis.md) and [live workflow observations](idm-workflow.md) provide separate evidence.

The report contains metadata and dialog text, not executable sections, icons or proprietary implementation code. Import/export presence establishes capabilities and interfaces; it does not establish which path is used for every download.

## What the components reveal

| Component | Directly observed evidence | Implication and limit |
|---|---|---|
| IDMan.exe | 105 dialog templates; Tahoma 8-point resource fonts; download, scheduler, grabber and configuration pages | Provides actual control order and dialog-unit geometry. Runtime theme, layout and behavior still require observation. |
| IDMMsgHost.exe | Standard input/output imports, process identity, registry and COM functions | Browser/native communication is a separate component. Its full private protocol was not reconstructed. |
| IDMNetMon.dll / IDMNetMon64.dll | `ControlMonitoring` export; process enumeration, memory-query/protection, named-pipe and device-control imports | Network integration has substantial native support beyond a browser popup. These imports alone do not prove a particular interception mechanism. |
| idmnmcl.dll | `InitNMC`, `InitNMC2`, `GetCks`, `FreeCks`, download-result, repair and unblock exports | Defines a native monitoring/client interface. Parameter layouts and complete semantics remain unknown. |
| IDMVMPrs.dll / IDMVMPrs64.dll | `GetMPDParserInfo`, `Parse1`, XML/HTML parsing exports | Manifest/parser functionality is independently packaged. This is not evidence that its complete YouTube logic has been recovered. |
| idmvconv.dll | AAC/TS conversion, FLV/TS-to-MP4, MP4/MKV/TS muxing and WebM audio normalization exports | Downloading and container processing are separate stages. UDM currently uses its own transfer engine followed by local FFmpeg/FFprobe. |
| idmwfp64.sys | WFP callout/filter registration, flow context, stream copying, stream/transport reinjection and process-related imports | Includes operations beyond UDM's passive flow/TCP/UDP monitor. Exact policy and driver/client protocol remain unverified. |

The WFP imports include `FwpsCopyStreamDataToBuffer0`, `FwpsStreamInjectAsync0`, `FwpsInjectTransportSendAsync0` and injection-handle lifecycle functions. Installing a renamed driver would not implement UDM integration. No IDM driver or signature is included in UDM.

## Browser extension findings

The installed Chrome extension version 6.43.1 was read as text. Its background component connects to `com.tonec.idm` and observes navigation, request bodies, request headers and response headers. Its page component observes fetch/XHR, response-body readers, inline scripts and selected page properties. Request markers correlate page information with browser request events. Native-provided patterns configure parts of the site-specific processing.

These observations support a browser-observation → request-correlation → native-processing architecture. The browser bundle does not reveal the complete native media-selection algorithm. No reference extension code or native-host protocol was copied into UDM.

UDM's independent extension already observes player responses, tracks current video identity, separates catalogs by video ID, and requires matching usable media URLs. The current live failure is explained in [browser capture](browser-capture.md): the earlier build lacked SABR transport. An original experimental implementation now passes synthetic tests, but a usable current live handoff is still unverified.

## Layout measurements and changes

| Reference dialog | Resource ID | Size in dialog units | UDM 0.5 work |
|---|---:|---:|---|
| Add URL | 148 | 373 × 57 | Compact address/history row, authorization group, right-side OK/Cancel |
| Download File Info | 130 | 380 × 128 | URL/category/save path, remembered folder, description, size area and three bottom actions |
| Download progress | 103 | 353 × 250 | Status/limiter/completion tabs, aggregate bar, pause/cancel row, range map and connection list |
| Download complete | 286 | 303 × 132 | Byte count, address, saved path, Open/Open with/Open folder/Close and suppression checkbox |
| File Properties | 135 | 318 × 246 | Compact properties sheet implemented; complete parity unverified |
| Scheduler | 308 | 434 × 286 | Compact queue tree/pages implemented with dated/manual schedules and queue ordering; complete parity unverified |

The main window now renders at the observed **746 × 444** size on this desktop. It has the compact toolbar, nested category tree, continuous table grid, narrow Tahoma type, and 29 original icon assets. Search and extra details remain available through View. The UDM logo and name are original. There is no trial or activation system.

Settings now use a compact tabbed dialog with eight pages. It does not yet reproduce all nine reference pages or all their controls. Desktop scaling was corrected during visual review; mixed-monitor DPI behavior has not been verified.

## What full parity still requires

1. Live validation and completion of the experimental SABR/UMP transport, plus validated handling of ordinary adaptive URLs, expiration and server rejection. Merely hiding a resolving message cannot supply missing streams.
2. Dynamic subdivision of slow active segments, broader HTTP/FTP/proxy coverage, recovery and comparative speed measurements.
3. Complete scheduler, properties, batch, grabber/project and options behavior and layout; category rules, synchronization queues and completion actions.
4. VM validation, signing, lifecycle recovery and a production service/installer for the original WFP monitor. Its x64 driver now compiles, passes static analysis/INF validation, and has a bounded desktop protocol; it has not been kernel-loaded.
5. Browser compatibility testing, accessibility and DPI review, signed installation, updates, rollback and distribution.

See [the feature matrix](analysis.md) for the broader acceptance list. This analysis narrows the work; it does not establish complete internal reverse engineering or complete application parity.
