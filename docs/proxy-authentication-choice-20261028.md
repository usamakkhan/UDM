# Explicit HTTP proxy authentication choice

The existing Options > Proxy / Socks > Advanced / Socks dialog now includes an HTTP(S) login method for the default route and each protocol override: Automatic, Negotiate, NTLM, Digest or Basic.

Automatic preserves the previous Basic/Digest/NTLM preference and bounded Negotiate-only retry behavior. Selecting Negotiate sends that method to curl from the beginning, including for form requests and mixed-method proxy challenges. UDM does not add its method-switching retry for any forced selection; curl owns the selected scheme's normal challenge exchange. This provides a configurable path for the previously absent mixed-method and plain-HTTP POST cases without changing all existing profiles.

The chosen method applies to explicit proxy credentials for HTTP/HTTPS. It does not change SOCKS, FTP gateway/CONNECT authentication or Windows current-user authentication. Negotiate and NTLM require their respective curl feature bits; missing support fails clearly. An explicit method is not a promise that the domain, service principal or credentials will authenticate successfully.

`ProxyAuthMode` defaults to Automatic. It is validated in global and per-protocol settings. A protocol override without the field uses Automatic rather than inheriting an unrelated global route's method. Browser-captured routes reset the choice, then copy it only alongside credentials for the matching configured proxy endpoint, route type and protocol. Get System resets it together with explicit credentials.

The extra control is in UDM's existing advanced dialog. This is a capability addition, not evidence that IDM has an identical authentication-selector control.

## Build and qualification

The production and setup record is `validation/proxy-auth-choice-build-20261028.json`. Runtime, UI, domain and NTLM regression tests remain deferred at the user's request. In particular, explicit Negotiate form handling and mixed-method authentication are implemented but unqualified. No installed preferences or extension files are changed; browser source remains 0.63.55.
