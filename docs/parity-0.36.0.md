# UDM 0.36.0 / signed network backend 0.3

## Delivered
- The C++/MFC Network window now uses the signed WinDivert backend. The old UdmWfp IOCTL client is removed from the desktop build.
- The desktop stays unelevated. Starting monitoring launches a separate elevated helper. Explicit current-user pipe ACL, remote-client rejection, first-instance ownership, random session identifier and reciprocal process-ID checks bind the session. Frames, nesting, process roots and stored observations are bounded.
- Process-tree IPv4/IPv6 TCP/UDP metadata reaches the GUI. Repeated timer refreshes preserve the session. Missing byte counters are shown as --, not fabricated zeroes.
- Stop and desktop exit release the helper and its filters. Diagnostics verify the pinned runtime without loading a driver or requesting elevation.
- HTTP/1 inspection now handles bounded multipart byte-range responses, including quoted boundaries, arbitrary transport fragmentation, chunked framing, close-delimited responses, and range-length validation. Bodies and credentials are not retained. Multipart observations never trigger automatic takeover.
- First-response interception immediately stops parsing subsequent pipelined responses.
- The top-level build now rebuilds and verifies the network helper; package.ps1 defaults to the matching 0.36.0 version.
- Setup packages the existing signed upstream driver, DLL, license and matching source archive. It no longer changes boot signing policy or installs a development root certificate.

## Validation on this PC
- 708 native regression checks passed.
- 24 desktop/broker acceptance checks passed, including real IPv4/IPv6 events and normal unelevated desktop-to-elevated-helper startup.
- 55 gateway checks passed, including 46 parser/relay checks.
- 38 signed-backend regression checks passed.
- One real HTTPS request passed with normal certificate validation.
- Two abrupt gateway-crash cases passed: fresh IPv4 and IPv6 connections recovered in approximately 20 ms and 10 ms on loopback. Existing proxied connections are not promised to survive a crash.
- Abrupt desktop-fixture exit released its helper in approximately 20 ms.
- Browser/native-host framing remains compatible; all seven real protocol checks passed.
- The installed GUI started monitoring three browser/UDM root process trees, displayed live events with zero dropped observations, and stopped its helper successfully. All 23 history records remained byte-for-byte unchanged.
- Test Mode was off and Windows code integrity was enabled.
- Installer compiled with official, publisher-verified Inno Setup 6.7.3. Full install/uninstall on a clean VM has not been run.

Machine-readable evidence is in drivers/signed-network/tests/*-0.3.json. Native suite output is in docs/reference/native-0.36.0.log.

## Scope and remaining differences
This is original UDM code, not IDM source or a claim of percentage identity. The signed driver binary is unchanged. The desktop integration in this release activates passive connection monitoring; browser download capture still uses the extension/native host. The TCP interception gateway remains explicitly process/port scoped and opt-in through the helper CLI, not a system-wide automatic browser redirector.

Remaining concrete work includes legacy RTMP/RTMPT/RTSP interpretation, production acceptance for arbitrary multi-interface IPv6 and VPN/filter combinations, HVCI/Secure Boot enabled acceptance, and public publisher signing of UDM's own app/installer. No evidence establishes that IDM's driver alone decrypts HTTPS or makes downloads faster.

The broader application inventory is still in docs/idm-parity-0.36.0.md: localization, live/subtitle workflows and several real-site/hardware acceptance cases are separate unfinished work. This release closes desktop signed-backend integration and installer Test Mode dependence; it does not mark every inventory row complete.

## Use
Open UDM's network integration window and choose **Start signed monitor**. Approve the normal Windows administrator prompt if displayed. **Stop monitor** releases the helper. **Driver status** verifies the installed package without elevation. No browser-extension reload is required for this desktop/driver update.

Reference semantics: [RFC 9110 multipart byte ranges](https://www.rfc-editor.org/rfc/rfc9110.html#section-14.6), [Microsoft named-pipe access controls](https://learn.microsoft.com/en-us/windows/win32/ipc/named-pipe-security-and-access-rights), [WinDivert documentation](https://reqrypt.org/windivert-doc.html).

