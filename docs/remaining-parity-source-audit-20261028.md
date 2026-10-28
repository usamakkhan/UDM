# Remaining parity: current-source audit

This audit records the pre-implementation snapshot. The subsequent [GET/HEAD proxy change](proxy-negotiate-downloads-20261028.md) supersedes its proxy implementation status.

This audit separates absent implementation from unqualified behavior. It does not establish complete parity or replace the deferred runtime matrix.

| Area | Current evidence | Remaining work |
|---|---|---|
| Explicit proxy Negotiate/Kerberos | `CurlHttp.hpp::proxyAuthSchemes` allows Basic, Digest and feature-gated NTLM only. DUMPBIN shows SPNEGO and SSPI identity symbols in the pinned curl library. | Define and implement negotiation/fallback policy, preserve explicit credential identity, and qualify domain and existing NTLM paths. |
| Signed updater | `App.cpp::OnUpdateResult` handles signed-manifest outcomes, version skips and update download prompts; Updates/UpdateManifest/UpdateUi source exists. | Publisher-signed delivery, installed upgrade/rollback acceptance and production feed qualification. |
| Signing | `installer/sign.ps1` and setup integration exist; latest candidate is unsigned. | Supply a real publisher-signing identity and qualify distribution. |
| ARM64 | Current build resolves x64 compiler/libraries. | Architecture-specific build/dependencies and a real ARM64 acceptance environment; network driver availability remains separate. |
| Native/browser imports | Recent request details, metadata, filtering, sorting, selection and batch work is present in source and candidate records. | Runtime, DPI, keyboard, authentication and interrupted-batch acceptance, then installed comparison. |
| Driver and transfer parity | Historical narrower measurements exist. | VPN/HVCI, physical faults, power transitions and paired current-build measurements. |

## Proxy finding

The backlog previously implied that explicit Negotiate support was already wired. It is not enabled in UDM's authentication mask. Library support alone does not make the adapter offer that scheme.

Official curl documentation says [CURLOPT_PROXYAUTH](https://curl.se/libcurl/c/CURLOPT_PROXYAUTH.html) controls which proxy schemes may be selected, and [CURLAUTH_NEGOTIATE](https://curl.se/libcurl/c/CURLOPT_HTTPAUTH.html) requires GSS-API or Windows SSPI. Curl's [current SSPI implementation](https://github.com/curl/curl/blob/master/lib/vauth/spnego_sspi.c) has distinct explicit-identity and current-user paths. That current upstream source is not treated as proof of the exact behavior of UDM's pinned binary.

Simply enabling another preferred scheme can change authentication choice on proxies advertising both Negotiate and NTLM. The next implementation needs a deliberate policy for explicit credentials and failed negotiation, with care not to replay an accepted upload. A domain machine remains necessary for full Kerberos acceptance; the user has deferred runtime testing.

## Documentation corrections

The active backlog now distinguishes implemented updater/signing code from delivery qualification. Stale statements that MIME-based filenames and metadata extension correction remain absent are removed. Driver qualification refers to the existing vendor-signed driver rather than authorizing test-signing.

No app or extension source changed in this audit. The latest successful 0.63.55 installer remains current, and no redundant build or installation was performed. The machine-readable evidence is in `validation/remaining-parity-source-audit-20261028.json`.
