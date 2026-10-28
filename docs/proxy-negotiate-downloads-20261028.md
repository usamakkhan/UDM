# Explicit proxy Negotiate for downloads

UDM previously allowed only Basic, Digest and NTLM for an explicitly configured proxy login, even though its pinned curl library contains SPNEGO support. The adapter now permits one Negotiate attempt when a completed GET/HEAD request is rejected by a proxy offering Negotiate and none of those existing methods.

The initial method preference is unchanged. A proxy advertising both Negotiate and NTLM still uses the existing NTLM path. There is no fallback after a failed Negotiate attempt and no switch to the current Windows user: the configured username/password remain on the request.

The retry requires a terminal 407 from the HTTP proxy or rejected HTTPS CONNECT, a compatible curl completion code, SPNEGO availability, and the advertised-method mask. It uses a fresh connection with the same URL, headers, explicit credentials, cancellation and overall deadline. Header-size accounting covers both attempts. TLS errors, ambiguous connection failures, callback failures and origin responses do not trigger it. Requests with a POST body are excluded, including empty bodies.

The detached-handle cleanup also removes request registration when retry setup throws, avoiding stale state in a shared session.

The implementation uses curl's documented [proxy method discovery](https://curl.se/libcurl/c/CURLINFO_PROXYAUTH_AVAIL.html) and [authentication mask](https://curl.se/libcurl/c/CURLOPT_PROXYAUTH.html). This is independently written behavior, not copied IDM code.

## Scope and delivery

This closes the GET/HEAD implementation gap for Negotiate-only proxies, not all explicit Kerberos parity. POST support, preference for Kerberos on mixed-method proxies, real domain authentication and existing NTLM regression acceptance remain pending. Runtime tests are deferred at the user's request. Build and packaging status is recorded separately in `validation/proxy-negotiate-build-20261028.json`; compilation alone does not establish successful authentication.

## Subsequent HTTPS form support

The [CONNECT-stage POST extension](proxy-post-connect-20261028.md) supersedes the blanket POST exclusion above: HTTPS forms may negotiate after a confirmed rejected tunnel with zero uploaded bytes. Plain HTTP POST and broader domain qualification remain pending.
