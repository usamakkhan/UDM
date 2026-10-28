# Explicit proxy negotiation before HTTPS form submission

The earlier Negotiate-only proxy support excluded every POST request. It now permits the same one-time explicit-credential negotiation when an HTTPS form download is rejected during CONNECT, before the origin request can be sent.

All conditions are required:

- The original transfer is complete, with no callback failure.
- Curl successfully reports CONNECT status 407 and the adapter has received no origin status.
- The destination is HTTPS.
- Curl successfully reports exactly zero uploaded bytes, including for an empty form.
- The completion code, advertised authentication methods and SPNEGO feature meet the existing Negotiate-only gates.

The rejected tunnel is essential: zero body bytes alone cannot prove that an empty POST request was not submitted. Plain HTTP POST, origin responses, ambiguous failures, failed information queries and nonzero uploads remain excluded. Mixed Negotiate/NTLM proxies retain the existing method preference. The same body, explicit credentials, headers, deadline, cancellation and single-attempt guard remain in place.

The decision uses the documented [CONNECT status](https://curl.se/libcurl/c/CURLINFO_HTTP_CONNECTCODE.html), [uploaded-byte counter](https://curl.se/libcurl/c/CURLINFO_SIZE_UPLOAD_T.html), and [proxy tunnel behavior](https://curl.se/libcurl/c/CURLOPT_HTTPPROXYTUNNEL.html). This is protocol-based source reasoning, not runtime evidence of Kerberos acceptance or IDM equivalence.

## Delivery and remaining scope

The production build and setup record is `validation/proxy-post-connect-build-20261028.json`. Runtime checks remain deferred at the user's request. Pending work includes domain acceptance, plain HTTP form authentication policy, mixed-method Kerberos preference, and NTLM regression qualification. The browser extension is unchanged at 0.63.55; installation and publication are not performed.

The subsequent [explicit method selector](proxy-authentication-choice-20261028.md) adds a configured Negotiate path for plain HTTP forms and mixed-method proxies. This document still describes Automatic mode; runtime qualification remains pending for both paths.
