# One capability lookup per handoff

Source review confirmed that handoff counts advance only after the desktop returns an ok reply. Cosmetic reporting already schedules its storage and badge work separately, so it was not changed.

The handoff path could independently request desktop preferences for proxy compatibility, custom import properties, captured POST limits and browser-session support. A link using all these features could therefore make four identical bridge requests before admission.

Chromium and Firefox 0.63.53 now lazily share one promise for desktop capabilities within a single handoff. The first feature needing the data requests it; later checks reuse the same response. Handoffs that do not need capabilities make no new request. Each new handoff creates a fresh promise slot, so desktop upgrades or changed support are rechecked on the next link. A rejected capability request remains rejected within the current handoff instead of being silently retried.

All feature gates and their error paths remain in place. Cookie-consent checks, request capture validation and final desktop acknowledgement are unchanged. This removes up to three redundant native bridge round trips; no measured latency improvement is claimed while runtime testing is deferred.

JavaScript syntax, mirror, manifest and installer checks are recorded separately. Native code is unchanged; the candidate remains uninstalled and unpublished.
