# Browser per-file Referer editing

The native import properties workflow now supports per-file Referer settings, matching the retained IDM File Properties control 1569. Browser link review still lacked an equivalent control and always used its captured/default request context.

Chromium and Firefox 0.63.54 add an Override Referer checkbox and field under each row's More properties. Unchecked retains normal capture behavior. Checked sends a validated HTTP/HTTPS Referer, or explicitly omits the header when blank. Both the review page and background handler validate the value. The desktop independently validates the new import property and applies it after assembling browser request headers, before session validation and download admission.

The override travels in importProperties.referer, separate from the original referrer/source-page field. This keeps DownloadPage and refresh context associated with the original source while changing the outgoing header. Cookies, user agent, proxy and captured request body are unchanged. The desktop advertises browserImportReferer=1; the extension requires it when an override is present, so older desktop builds fail before admission instead of silently ignoring the setting.

The review retains original lookup data separately. Enabling custom authorization or Referer hides its size/type because those values describe a different request. Disabling overrides restores the original lookup. UDM checks the customized request when it performs its own lookup/transfer; this change does not introduce authenticated browser HEAD probes or claim an exact custom-request size in the review.

JavaScript syntax, mirrored sources, manifest versions, native compilation and packaging are recorded separately. Runtime, browser and regression testing remain deferred at the user's request; the candidate is not installed or published.
