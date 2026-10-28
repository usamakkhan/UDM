# Native DASH clock synchronization

The native manifest module can now synchronize against declared UTCTiming sources in document order. Supported schemes are direct, http-head, http-xsdate and http-iso, with the 2012 and 2014 identifiers. Direct values reuse the strict explicit-timezone timestamp parser and anchor to the caller-supplied receipt time of that MPD. HTTP body sources accept a bounded timestamp response; HEAD sources parse the Date header.

The source ordering and explicit failure behavior follow the [DASH-IF clock synchronization guidance](https://dashif.org/Guidelines-TimingModel/#clock-sync). UDM does not invent a default time service or silently treat the PC wall clock as synchronized when all declared sources fail. See also the supported schemes in [dash.js clock synchronization documentation](https://dashif.org/dash.js/pages/usage/clock-sync.html).

HTTP synchronization has a shared ten-second deadline, eight-request budget, up to four requests in a redirect chain, at most sixteen declared sources and at most eight URLs per source. Timestamp bodies are limited to 256 bytes. HTTP and HTTPS are supported; HTTPS-to-HTTP downgrade redirects/sources are excluded. Browser session state, site-login entries and automatic intranet login are removed from clock-request settings. No media authorization/cookie headers are forwarded. Proxy configuration remains available through the normal HTTP transport. Nonzero or malformed Age headers cause that response to be rejected, and requests ask for cache revalidation.

Successful HTTP samples anchor the server timestamp to the midpoint of the measured monotonic request interval. The recorded uncertainty is half that interval plus one second for timestamp resolution; this is an estimate, not a guarantee of symmetric network delay or server accuracy. Direct-source uncertainty is recorded as one second, excluding any unknown source publication/cache age. The caller must supply a fresh manifest receipt and account for that limitation. Samples expire after sixty seconds and are advanced using steady_clock, without changing Windows time.

## Integration limits

The callable synchronization implementation uses the existing HTTP transport but is not yet invoked by a running live recorder. Its output can supply server time to the native availability planner; automatic refresh, uncertainty policy, manifest freshness, transfer execution and durable recording remain unfinished. Dynamic browser MPDs remain rejected. No clock endpoint or browser download was exercised during this change.

Runtime and regression tests are deferred at the user's request. Production build/package evidence is in `validation/native-dash-clock-build-20261028.json`. Browser 0.63.61 is unchanged, and the candidate is unsigned, uninstalled and unpublished. Build success does not establish full IDM parity.
