# Disabled-tab toolbar feedback

Chromium and Firefox integration 0.63.43 now display an `OFF` badge and
`UDM — capture disabled on this tab` tooltip for the existing per-tab capture switch.
Re-enabling clears the tab-specific badge override, allowing the shared error badge
to show again. The normal tooltip comes from the extension manifest.

Indicators are restored from session state when the background worker starts and
refreshed on tab activation and navigation. Presentation calls are serialized per
tab and handled independently from capture and handoff. Closed-tab API failures
are contained and do not interrupt downloads.

The implementation uses UDM's existing switch, not IDM code or artwork.
The browser API specifies that null badge text removes a tab override:
https://developer.chrome.com/docs/extensions/reference/api/action

Both browser sources are kept identical. JavaScript syntax, manifest versions,
and whitespace checks are the only validation in this batch. Browser behavior and
regression tests are deferred at the user's request. This extension is not installed.
