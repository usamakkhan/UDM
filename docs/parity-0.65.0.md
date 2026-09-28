# UDM 0.65.0 / browser 0.42.0

Signed-in ordinary downloads can now retain the browser's cookie attributes and accept cookie updates during metadata checks, redirects and parallel transfers. This extends 0.64's Site Grabber support to browser file downloads. The signed network runtime is unchanged.

## Behavior

- The existing **Share browser cookies** preference and browser cookie permission remain required. Neither is enabled by this update.
- Select the request tab's cookie store, including Firefox containers. For an observed request, transfer only cookies whose exact name/value, path and partition can be identified. Unsent cookies are excluded. Chromium's known frame partition is matched explicitly; unknown or ambiguous scope retains the observed static header.
- For a manually selected link without an observed request, cookie lookup is limited to an identified, regular, top-level tab on the same origin. The extension no longer guesses from its default cookie store.
- Bind the managed session to the captured scheme, host and port. Apply cookie paths, Secure and expiry, and save replacements/deletions with Windows account encryption before subsequent native requests. Recheck the page and consent before submission.
- Save the session with a new download and retain it through duplicate choices and reviewed link refresh. A different session cannot silently replace an existing active/selected download's authentication. Older desktop versions retain the compatible static-header handoff.
- Enforce the complete 16,384-byte cookie-header limit, including UTF-8 byte lengths, equals signs and separators, in both browser and native validation.

Cookie-store and partition behavior follow the official [Chromium cookies API](https://developer.chrome.com/docs/extensions/reference/api/cookies) and [Firefox cookies API](https://developer.mozilla.org/en-US/docs/Mozilla/Add-ons/WebExtensions/API/cookies/getAll). No claim is made about IDM's internal code.

## Acceptance

**2,401 fresh checks passed:** 1,669 native, 689 browser unit, 8 native protocol, 8 app/host and 27 actual browser-download checks. The focused 71 native checks are included in the full native total.

See [the recorded checks](evidence-0.65.0/summary.json) and [development notes](evidence-0.65.0/DEVELOPMENT.md). Test downloads use synthetic HttpOnly cookies, localhost servers and isolated profiles. They do not access personal accounts. A corrected before/after fixture fails with HTTP 401 on 0.64 and succeeds with byte-exact output on 0.65.

## Remaining limits

- Cookies rotated by the browser before their original attributes can be matched, ambiguous duplicate cookie values, unsupported partition contexts and Firefox first-party-isolation cookies retain the observed static-header behavior. They are not automatically upgraded to managed sessions.
- Independent downloads keep independent snapshots. Native cookie changes are not written back into the browser. Applications that invalidate concurrent sessions may require fresh sign-in or sequential downloading.
- Cross-origin redirects never receive the captured session, even when two hosts belong to one site. Media handoffs, rendered-page/localStorage authentication, browser-cache reuse and arbitrary authorization protocols remain separate work.
- Native visual/DPI/accessibility acceptance, public video-platform compatibility, repeated matched-route speed comparisons with IDM, driver equivalence and publisher signing remain incomplete or unverified. Full IDM parity is not established.
- Browser 0.42.0 requires reloading the installed unpacked Chrome/Edge extensions and refreshing their pages. Isolated acceptance does not establish activation in personal browser profiles.

## Current-PC deployment

Native 0.65.0 is installed and running with browser 0.42.0 installed in both unpacked Chromium source folders. The native bridge reports managed-session support. All 23 download records, queues and preferences are unchanged. The signed network runtime is unchanged. Personal Chrome/Edge extensions still require reload.

[Installer](../installer-out/UDM-0.65.0-Browser-0.42.0-Setup-x64.exe), SHA-256 `D6B6ADB721C0F7E080EF539421EF649A2565A3F5831193AAD11BB49883C8E89F`. [Deployment receipt](evidence-0.65.0/deployment.json).
