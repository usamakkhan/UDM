# COM cold-activation follow-up — 1 October

The outstanding COM activation failure reproduces with a fresh random CLSID and the independently built IUnknown-only server. It performs no UDM work and uses no UDM type library.

Fresh test from a short D: path: HRESULT 0x80040154, 15 ms, no object. Same test under Windows System32 64-bit PowerShell: HRESULT 0x80040154, 16 ms, no object. Both removed their temporary HKCU class registration. The client is 64-bit, non-elevated and runs as the expected user.

These results rule out this failure being unique to UDM's API/type library, the old long executable path, or the bundled PowerShell executable. They do not establish the cause or prove UDM's registration is correct. The prepared machine-wide comparison remains unexecuted because the earlier administrator prompt was canceled; it needs renewed consent before retrying.

No production source, personal runtime, IDM registration or machine-wide COM registration was changed. The separate 0.75 COM candidate must not be merged as if cold activation had passed. Further diagnosis should compare permitted user/machine registration visibility with an explicit controlled machine-wide test or a separate clean Windows environment, rather than repeatedly changing UDM's CLSID/type library speculatively.

Next independent parity work can proceed while this environment-dependent qualification remains open.
