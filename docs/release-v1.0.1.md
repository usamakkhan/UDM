# UDM 1.0.1

UDM 1.0.1 is a maintenance release that completes the authentication and media-preview work merged after 1.0.0.

## Changes since 1.0.0

- **NTLM proxy authentication:** The bundled curl library is built with Windows SSPI and NTLM enabled. A saved proxy login can now complete NTLM authentication, while the existing current-Windows-login option remains available for suitable intranet routes.
- **PAC proxy validation:** A loopback PAC fixture now covers ordered proxy choices, DIRECT and SOCKS results, failover, and unreachable configuration behavior.
- **Accurate media sizes:** Video and audio menus show exact sizes when a selected resource reports a trusted content length. UDM does not present bitrate-and-duration calculations as exact bytes.
- **Faster, safer handoff:** Browser/native media handoff preserves selected metadata and reported sizes, recovers cleanly from receipt errors, and rejects stale, expired, foreign, or untrusted captures before a download starts.

The release is distributed as one Windows setup executable containing the desktop application, driver runtime, browser integrations, and bundled media tools.
