# UDM 0.30.0: resumable, parallel FTP

UDM's FTP transport now preserves validated partial downloads and uses multiple connections. Previously it deleted FTP partial files at startup and used one synchronous WinINet transfer. This release replaces that path with cancellable native Winsock connections while retaining the existing speed limits, quota accounting, progress rows, assembly, SHA-256 verification and publication pipeline.

## Behavior

- Up to the selected 1–32 connections, with fixed segments of at least about 1 MiB. Servers without a usable SIZE/MDTM identity or REST support use one sequential transfer. This is fixed segmentation, not HTTP's dynamic splitting.
- Paused downloads survive process restarts. The engine checks the address/account, file size, modification time and segment plan, then reconciles counters against actual part sizes. A missing part is fetched again. Oversized or unverifiable parts are preserved and rejected.
- Size and modification time are rechecked for each worker and before publication. Detected changes block publication. MDTM/SIZE are server metadata, not cryptographic proof: a server that changes bytes while preserving both values cannot be detected from those values alone. A supplied SHA-256 remains the stronger final check.
- Transient worker connection failures retry from the saved offset. Permanent FTP errors report their numeric status without including credentials or server-provided text. Pause interrupts control/data waits and retry delays. Initial server probing is not automatically retried.
- EPSV/PASV passive mode and EPRT/PORT active mode work. Passive data always uses the control server's address; foreign PASV addresses are ignored and privileged passive ports are rejected. Active data must originate from the control peer.
- Full-file completion requires EOF and a successful FTP completion reply. If all bytes arrived but the reply failed, resume re-fetches a tail before accepting completion. Interior segments stop at their assigned boundary and close their own data/control connections.
- Per-file username/password fields work with FTP. Saved credentials and the resume source/account marker remain Windows-account encrypted. A stopped FTP record can use **File → Redownload** to create a fresh paused copy without deleting the original record or its partial files.
- Valid MDTM dates feed the existing “Use server file creation date” setting. Existing HTTP, media and browser capture paths remain in place.

Implementation references: [FTP restart, size and modification time](https://www.rfc-editor.org/rfc/rfc3659), [extended active/passive FTP](https://www.rfc-editor.org/rfc/rfc2428), and [Windows cancellable DNS resolution](https://learn.microsoft.com/en-us/windows/win32/api/ws2tcpip/nf-ws2tcpip-getaddrinfoexw). The code is UDM's independent implementation.

## Release validation

The final build produced UDM, NativeHost and Monitor. Release validation covered parallel active/passive FTP transfers, pause and process restart, dropped streams, changed/truncated/oversized files, missing parts, stale counters, completion replies, login handling, malformed replies, unsafe ports, command injection, proxy rejection/bypass, SHA-256 rejection, application launches and protocol boundaries.

The local fixture observed eight simultaneous data connections. These are local acceptance measurements, not an Internet speed comparison with IDM. pyftpdlib and its dependencies were used only in the local validation artifact directory and are not bundled with UDM.

## Installation

Installed and running as **UDM 0.30.0**. All **24 installed files** match the tested deployment hashes. All **23 existing download records** match the pre-installation snapshot exactly, including after the browser test. NativeHost confirms version 0.30.0, the correct history directory and the expected download count. Replaced files and the pre-installation state are backed up in `parity-ftp-20260926/backup-before-0.30.0`.

The installed Edge integration covered exact MP4 output, recorded HLS with audio, authenticated range capture, Download Later, capture modifiers and stale iframe exclusion in an isolated profile and history. The normal user browser profile and its extension settings were not changed.

## Remaining limits

The updated [103-workflow audit](idm-parity-0.30.0.md) has **86 implemented code paths and 17 open rows**. This is not a percentage of complete IDM parity. Direct FTP transfer/resume is implemented; FTP through SOCKS, HTTP proxies and Windows proxy/PAC routing remains unsupported. Configured proxy modes fail explicitly before contacting the FTP server, unless an explicit direct bypass matches the host. Users with automatic Windows proxy detection enabled must choose a supported direct mode for FTP. FTPS/SFTP and literal IPv6 FTP URLs are not implemented; DNS names can resolve to IPv6, but that path was not exercised in these IPv4 fixtures.

Visual acceptance is still unavailable. The updated menu action compiled, but its appearance and focus behavior have not been clicked against IDM in this session.

Other open work includes wider real-site video coverage, live/subtitle/alternate-track media workflows, localization, rendered/authenticated site mirroring, signed distribution, accessibility/DPI review and repeated matched-route Internet benchmarks. Extension source remains version 0.24.0.

All initial/final logs and deployment evidence are in `parity-ftp-20260926` under the writable visualization workspace.
