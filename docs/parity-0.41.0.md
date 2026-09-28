# UDM 0.41.0 / browser 0.30.0 verification

Firefox ordinary download capture now follows the proxy used by the browser request. Previously, UDM could receive the correct URL but use a different desktop route. A live PAC fixture also exposed a cross-site association bug: Firefox supplied originUrl while UDM matched only documentUrl/initiator. The updated observer handles that field and successfully transfers the download.

## Changes

- Capture and validate direct, HTTP and SOCKS remote-DNS routing for the exact URL. Chromium without Firefox proxyInfo retains desktop routing. Ambiguous or unsupported Firefox routes stay browser-managed.
- Negotiate native browserProxy capability before accepting a route; reject an old host without canceling the browser download. Preserve durable capture acknowledgement and recovery.
- Save the route atomically with the job using DPAPI. Preview, transfer, restart, redownload, synchronization and reviewed link refresh preserve routing. Ordinary catalog export requires recapture; protected export can retain the route for the same account.
- New default-enabled Options > Proxy control allows explicit use of desktop defaults. Proxy credentials are reused only from an explicitly matching desktop endpoint and protocol.

## Verification

| Check | Result |
|---|---:|
| Native C++ build and full suite | 824 passed, 0 failed |
| Browser JavaScript suites | 369 checks across 18 suites, 0 failed |
| Native messaging protocol | 8 passed |
| Actual Chrome form handoffs | 13 passed |
| Actual Edge form handoffs | 13 passed |
| Actual Firefox form handoffs | 13 passed |
| Actual Firefox PAC/direct/SOCKS routing | 15 passed |

The routing fixture used two independent HTTP proxies, a direct route and SOCKS5 remote DNS, while the desktop default deliberately pointed at an unavailable proxy. Each final file matched the fixture SHA-256; recorded range requests used only the expected route. No unrelated proxy credentials were sent. Browser downloads were canceled only after native acceptance. These local acceptance tests qualify routing and data integrity, not public-network speed parity.

The native settings dialog was visually reviewed with an isolated empty catalog. The new checkbox and description fit the Proxy tab. Toggling and applying it persisted the setting. The user's catalog is checked before and after activation; no test jobs are added there.

Evidence: [test results](evidence-0.41.0/summary.json), [native output](evidence-0.41.0/native-tests.log), [browser output](evidence-0.41.0/browser-tests.log), [routing results](evidence-0.41.0/firefox-proxy-results.json).

## Remaining differences

This closes part of F079, not full IDM parity. The [103-workflow inventory](idm-parity-0.41.0.md) remains 88 implemented, 12 partial, 2 unverified and 1 deferred by the user. A code path being implemented is not exhaustive behavioral qualification.

- Chrome/Edge per-request proxy inheritance is not implemented. Firefox encrypted HTTP proxies, browser proxy usernames and SOCKS local DNS are unsupported. No browser proxy credentials are extracted.
- Saved routing is bound to the exact captured HTTP(S) URL. A newly redirected URL requires recapture. Adaptive media retains desktop proxy settings. Active FTP through proxies and FTP gateway/PAC modes remain open.
- Universal video/site/session support, live recording, remaining media formats, automatic driver-level handoff, physical sleep/disk-failure qualification, matched-route IDM speed testing and broad DPI/accessibility acceptance remain open.
- This is a local build. Application/installer publisher signing, an updater and clean-machine install/repair/uninstall qualification remain open. The existing signed network runtime is included unchanged.

## References

[IDM Options](https://www.internetdownloadmanager.com/support/options.html) documents browser proxy inheritance as an option. [Mozilla response metadata](https://developer.mozilla.org/en-US/docs/Mozilla/Add-ons/WebExtensions/API/webRequest/onHeadersReceived) documents request proxy information; [Mozilla request metadata](https://developer.mozilla.org/en-US/docs/Mozilla/Add-ons/WebExtensions/API/webRequest/onBeforeRequest) describes originUrl. [Chrome proxy configuration](https://developer.chrome.com/docs/extensions/reference/api/proxy) describes a separate configuration API; this release does not add that permission. The implementation is original UDM code.

## Installed result

The existing UDM instance was closed gracefully before binary replacement, then UDM 0.41.0 was launched and verified through the real native host. Its original 23-record catalog retained SHA-256 `AAE5B9BF965F9850CC771232D988052FF5083FD809EA711B1520FB10A0ECC851`. Chrome and Edge were reloaded to extension 0.30.0; both popup connection checks reported ready. Firefox 0.30.0 was tested through temporary isolated installations.

The installer is `installer-out/UDM-0.41.0-Setup-x64.exe`, SHA-256 `4A8FFF850AD82EF3868069C2F5C5AFFE7AC290C1C60F88255E4BD1C5B266DC70`. It was built and copied, not run as a clean-machine installation test. The local source update preserved the concurrently edited README, and only changed/new source files were installed.
