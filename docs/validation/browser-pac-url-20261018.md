# Browser PAC URL handoff — candidate, 2026-10-18

The candidate adds HTTP/HTTPS PAC URL capture to the Chromium integration.
The download carries a URL-bound `type: pac` configuration with a `pacUrl`;
this is configuration inheritance, not a claim to retain Chrome's selected
endpoint or failed-proxy cache. The native PAC resolver fetches and evaluates
the script. Existing fixed endpoint and Firefox request-route behavior remains
separate.

The extension requires the desktop's `browserPacUrl: 1` capability before
handoff. Older desktops keep the browser download. Script URLs are stored with
the existing encrypted browser proxy record. Applying the captured configuration
clears unrelated desktop proxy credentials, bypass lists, protocol overrides,
auto-detect and transport metadata. Existing request-URL binding prevents
redirects from silently reusing a capture for a different URL.

This implements the URL-PAC part of the
[reference configuration trace](idm-proxy-config-20261018.md). Inline scripts,
WPAD/auto-detect capture, and live IDM fallback equivalence remain unfinished.
Script-resolution errors use the native resolver's existing failure behavior;
the candidate does not adopt Chrome's optional direct fallback on PAC errors.

## Validation

Browser checks completed:

- Chromium configuration: 95 passed.
- Desktop capability and handoff: 16 passed.
- Firefox request proxy metadata: 13 passed.
- Transient settings-read recovery: 4 passed.

The backend, native test executable, and desktop UI compiled successfully using
the configured cached toolchain and vendor libraries. Native PAC checks passed
16/16; the connection suite passed 464/464. Capture transaction (127), recovery
(27), and native protocol (9) checks also passed.

Live acceptance passed nine checks each in isolated Chrome and Edge profiles.
Each browser handed off two 4,194,335-byte files: one using the script-selected
HTTP proxy and one using DIRECT. All four output files matched the fixture
payload hash. Browser ownership was released only after native acknowledgement;
unrelated proxy credentials were not transmitted. The script URL was absent
from plaintext desktop and extension storage.

The candidate desktop SHA256 is
`967E80E2345CF186D12AF7BED971F5E9B0ADE38D3536E6F4953CF2CD6792E317`;
both extension manifests are version `0.63.18`. Sanitized checks and file
hashes are in [the validation record](browser-pac-url-20261018.json).
This candidate is not installed.
The installed build remains the verified content-detection/Columns update.

The new native cases cover malformed configurations, clearing unrelated
settings, encrypted capture, restart, File Info size, completed download bytes,
and redownload. The isolated browser fixture adds `UDM_PROXY_PAC_ONLY=1` to
`tests/chromium-proxy.native-live.cjs`: it checks a proxied and a DIRECT result,
browser cancellation after native acknowledgement, exact output bytes, and
absence of plaintext PAC URLs from persisted browser/desktop state.
