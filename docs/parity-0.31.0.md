# UDM 0.31.0: independent Download File Info preview

Download File Info now shows size and type even when “Download HTTP files while File Info is open” is disabled. Previously the dialog depended on an active transfer to discover size. This release adds an asynchronous metadata lookup, a compact status line and **More → Refresh details**. HTTP prefetch also records the server Content-Type for the dialog.

## Behavior

- HTTP uses HEAD first. If HEAD is rejected with 403/405/501 or omits size, the lookup requests bytes 0–0 and validates the response headers. It does not read the response body, create part files, publish output or alter the download record. A server that ignores Range can still send data into WinHTTP's network buffers before the request closes; this is not a zero-network-byte guarantee.
- FTP uses SIZE/MDTM over its control connection. It never opens a data channel or sends RETR/REST for the preview. Existing FTP proxy limitations still apply.
- The dialog shows the server MIME type when available, otherwise Windows' extension-based file type. Unknown sizes remain unknown, rather than being reported as zero. MIME text is normalized and invalid values are discarded; file contents are not sniffed.
- Preview metadata is informational and local to the dialog. It cannot replace resume validators, segment sizes, saved addresses, output names or received-byte counters. Active/queued/completed jobs and any saved segment plan are excluded.
- POST forms, browser-recapture jobs, captured media, offline projects and unresolved duplicate prompts are excluded before network access. Opening File Info cannot submit a form.
- Refresh details uses the current login fields without saving them; Start Download/Download Later retain the existing explicit save behavior. Basic/Digest challenges on the original origin get a login hint. A redirected login challenge or browser-token challenge asks for a fresh browser capture. Cross-origin redirects strip credentials, cookies, Origin and Referer.
- The lookup has a 10-second cancellation deadline; closing/confirming/refreshing the dialog cancels its worker. Existing synchronous SOCKS proxy-host DNS initialization can still delay cancellation before WinHTTP is created; that pre-existing resolver path has not been replaced or delay-tested in this release. Normal stalled HTTP/FTP operations and SOCKS handshakes were tested.
- The private SOCKS4/4a/5 bridge now forwards HEAD, in addition to its existing GET/POST/CONNECT support. TLS validation remains enabled.

The request behavior follows [HTTP HEAD semantics](https://www.rfc-editor.org/rfc/rfc9110.html#name-head) and uses the existing [FTP SIZE/MDTM commands](https://www.rfc-editor.org/rfc/rfc3659). UDM's code is independently implemented.

## Release validation

The release build covers metadata preview through HTTP HEAD and range fallback, FTP SIZE/MDTM lookup without data transfer, proxy handoff, cancellation and timeout behavior, authentication redirects, TLS rejection, excluded requests, and unchanged download state. With prefetch disabled, File Info obtains metadata without creating output or part files and remains awaiting confirmation. This is local behavior validation, not an Internet download-speed comparison with IDM.

## Installation

Installed and running as **UDM 0.31.0**. All **33 installed files** match the deployment hashes. All **23 existing download records** match the pre-installation snapshot exactly, including after browser testing. NativeHost reports version 0.31.0, the correct history directory and download count. Rollback files and the pre-installation state are retained in `parity-preview-20260927/backup-before-0.31.0`.

Installed Edge integration covered byte-identical MP4, recorded HLS video/audio, authenticated range capture, Download Later, modifier gestures and stale iframe exclusion. Testing used an isolated browser profile and test history; normal browser settings were not changed.

## Remaining acceptance

The [103-workflow audit](idm-parity-0.31.0.md) now has **87 implemented code paths and 16 open rows**. This is not complete IDM parity or a percentage of every possible IDM feature.

Visual inspection is not claimed for the new controls' pixel layout, keyboard traversal or DPI behavior. A failed lookup does not prevent Start Download or Download Later.

Open work still includes broader video-site coverage, live/subtitle/alternate-track workflows, FTP proxy transports, localization, rendered/authenticated site mirroring, signed distribution and comprehensive accessibility/performance acceptance. Browser extension source remains 0.24.0; the desktop host is 0.31.0.

Build logs, results and deployment backups are retained under `parity-preview-20260927` in the writable visualization workspace.
