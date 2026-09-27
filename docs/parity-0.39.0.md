# UDM 0.39.0 / browser 0.29.0: interrupted handoff recovery

Date: 27 September 2026. Local build, activated and packaged; not a published release.

## Changed behavior

An automatic file capture used to depend on the browser background worker surviving from Pause through Add and Cancel. If that worker disappeared, its in-memory request and acknowledgement could disappear too, leaving a paused browser download or an uncertain native handoff.

The extension now saves an opaque identity before pausing. Native UDM records ownership of the handoff in its saved catalog. After a worker restart, the extension either resumes a download that was not submitted, cancels the browser copy of an accepted native job, or leaves an uncertain case for review. A release receipt rejects an Add that arrives late after the browser has already resumed. Repeated accepted tokens return the existing job rather than creating another one.

The popup includes **Recover interrupted downloads**. Automatic recovery also runs at startup, installation/reload and a scheduled recovery alarm. The journal stores a download ID, hashed identity, opaque token, protocol and phase; it does not store the source URL, cookies, authorization headers or POST body.

## Validation

| Checks | Passed | Evidence |
|---|---:|---|
| Native engine and application regressions | 761 | [Native results](../benchmarks/parity-0.39.0/native-tests.log) |
| Browser modules, native protocol and packaging preparation | 366 | [Browser results](../benchmarks/parity-0.39.0/regressions.log), [protocol](../benchmarks/parity-0.39.0/native-protocol.log), [preparation](../benchmarks/parity-0.39.0/prepare.log) |
| Chrome forced-worker recovery | 11 | [Results](../benchmarks/parity-0.39.0/chrome-recovery.json) |
| Edge forced-worker recovery | 11 | [Results](../benchmarks/parity-0.39.0/edge-recovery.json) |
| Chrome form downloads | 13 | [Results](../benchmarks/parity-0.39.0/chrome-post.json) |
| Edge form downloads | 13 | [Results](../benchmarks/parity-0.39.0/edge-post.json) |
| Firefox form downloads | 13 | [Results](../benchmarks/parity-0.39.0/firefox-post.json) |

All listed checks passed. The 761 native checks include 18 new receipt checks; the browser total includes 21 new recovery checks.

The live Chrome/Edge test terminates the actual service worker at four barriers: paused before submission, journaled but not sent, accepted by native UDM with its acknowledgement withheld, and acknowledged before browser cancellation. It verifies worker replacement, final file hashes, browser state, native job counts, journal cleanup and rejection of late submission. These are local HTTP fixtures in isolated profiles/catalogs, not Internet speed benchmarks.

The unit tests also cover reused browser IDs, changed source identity, legacy native hosts, concurrent handoffs, storage errors, native unavailability and failed resume/cancel operations. The form regressions preserve exact request bytes and supported encrypted native storage; incomplete/oversized/multipart requests continue in the browser. Firefox worker termination itself was not exercised live.

## Activated build

- Native UDM and native messaging host: **0.39.0**, installed in `D:\UDM\release`.
- Chrome and Edge: **0.29.0**, reloaded through their actual Extensions pages. Both popup recovery buttons returned “0 downloads recovered. No handoffs need review.” Both connection checks returned “UDM is connected and ready.”
- Firefox: packaged and verified using an isolated real Firefox profile; not claimed installed in a personal Firefox profile.
- The existing 23-download catalog remains byte-identical. SHA-256: `AAE5B9BF965F9850CC771232D988052FF5083FD809EA711B1520FB10A0ECC851`.
- Setup: `installer-out/UDM-0.39.0-Setup-x64.exe`, rebuilt with the existing signed network runtime and FFmpeg tools.
- Setup SHA-256: `43B60D3724671BA7B7D31AE65512AA42374F52D22494DB426A74A455EA399911`.

See [activation evidence](../benchmarks/parity-0.39.0/activation.json) and [package log](../benchmarks/parity-0.39.0/package.log).

## Limits and remaining parity work

Recovery does not resend a request. If the native process or disk fails between saving a reservation, saving the job and confirming acceptance, the receipt can remain uncertain; the user must compare UDM with the browser Downloads page before resuming. This is not a claim of complete transactional recovery after arbitrary power loss. A native download failure after acceptance continues to be handled by UDM's normal retry/resume workflow.

The browser journal is bounded to 128 handoffs. Native receipts are bounded to 2,048 entries; expired entries are pruned during new receipt creation after 24 hours. Missing receipts older than ten minutes are treated as uncertain, not evidence that submission never occurred. Browser privacy modes, cleared history/storage, manual actions and full machine/browser crashes need further acceptance testing.

Multipart and unsupported request bodies remain browser downloads. No new video format, driver feature or transfer-speed improvement is claimed by this release. The [103-workflow inventory](idm-parity-0.39.0.md) remains 87 implemented, 13 partial, 1 missing and 2 unverified; those counts are not an IDM parity percentage. Worker-loss continuity is improved within F067, whose broader compatibility work remains partial.

## Implementation and references

The native implementation is in [CaptureReceipts.hpp](../native/CaptureReceipts.hpp) and [Bridge.cpp](../native/Bridge.cpp); browser recovery is in [capture-recovery.js](../browser/chromium/capture-recovery.js). The [live fixture](../tests/capture-recovery.native-live.cjs) and [unit checks](../tests/capture-recovery.test.cjs) can be rerun against a new isolated fixture directory.

Browser background workers can terminate, so durable state is necessary for continuity: [Chrome extension service-worker lifecycle](https://developer.chrome.com/docs/extensions/develop/concepts/service-workers/lifecycle). Browser state and resumption use the documented [DownloadItem](https://developer.mozilla.org/en-US/docs/Mozilla/Add-ons/WebExtensions/API/downloads/DownloadItem) and [downloads.resume](https://developer.mozilla.org/en-US/docs/Mozilla/Add-ons/WebExtensions/API/downloads/resume) APIs. This is independently written UDM code.
