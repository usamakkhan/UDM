# UDM browser 0.40.0 with native UDM 0.61.0

This update fixes a reproducible capture-cleanup race: a late tab URL notification could erase a playlist already captured from the new document. The earlier Edge iframe timeout showed this symptom; the exact ordering in that historical run was not fully traced. The regression reproduces the failure sequence against the legacy cleanup and passes against the new implementation.

## Changes

- Use committed document identity to retire old playlists, raw audio/video URLs, SABR request context and authenticated download context. Fresh responses arriving before a commit notification survive cleanup.
- Track iframe descendants, preserve sibling players, and allow a restored document to capture again. A frame ID alone is no longer treated as a document identity.
- Serialize menu-offer writes with capture cleanup. Offers carry document IDs, and retired documents or closed tabs cannot restore captures through delayed asynchronous work.
- Retain bounded document identity across service-worker restarts. Current frame records do not expire merely because a video has played for more than three minutes.
- Keep same-document route changes conservative. Browsers lacking document IDs retain conservative committed-navigation cleanup; environments without webNavigation retain the old URL-change fallback.
- Generate the same lifecycle implementation for Chromium and Firefox. Installer filenames now include both desktop and browser versions, preventing a browser-only release from replacing a prior package name.

The added **webNavigation** permission observes document transitions. Its saved session metadata contains frame/document IDs, relationships and timestamps, without URLs, headers or request bodies. Existing credential opt-in, player identity, encrypted-media and ad-state checks remain in place.

## Verification

**687 fresh checks passed:** 639 browser regressions (including 25 new lifecycle checks), 8 real native-protocol checks, and 20 extension/native acceptance scenarios in each of Chrome and Edge.

The real browser tests download generated MP4, recorded HLS and live HLS through native UDM, verify authenticated range requests, browser capture gestures and native messaging, then check repeated top-level and iframe navigation, same-URL reload, Back, current-document menu contents and tab-close cleanup. Each browser exercised 20 capture checkpoints in the navigation sequence. Generated local media and isolated profiles are used; this does not establish public-site compatibility.

Initial unrestricted-process tests were needed because the sandbox rejected native host/preparation child processes with EPERM; both checks then passed. A new test fixture initially used a changing timeOrigin and was corrected to model the browser's fixed per-document value. No production change was made to suppress a failed assertion. Navigation-aborted network requests during deliberate page replacement are expected; current-page captures and menus were independently checked.

[Evidence](evidence-browser-0.40.0/summary.json) · [Existing native release](parity-0.61.0.md) · [103-workflow inventory](idm-parity-0.61.0.md).

## Activate

Reload **UDM Browser Integration** in Chrome/Edge's Extensions page and refresh video tabs. If prompted, review the newly added navigation permission. The unpacked extension keeps its existing ID and preferences. Native UDM remains 0.61.0; no download-history migration or driver reinstall is needed.

## Remaining limits

Firefox live acceptance was not rerun. Same-document SPA transitions still invalidate conservatively and may require fresh playback or Refresh. The historical Edge failure's exact cause cannot be proven from its older trace, though the reproduced cleanup race is fixed and the fresh Chrome/Edge suites pass. Personal-browser activation and native visual acceptance remain unverified because the control tool fails before initialization with a missing kernel-assets path.

Full IDM parity remains unfinished: public video-site and ciphered YouTube coverage, further adaptive transports, driver/legacy handoff equivalence, multipart POST, some proxy modes, physical power/removable-media qualification, same-server speed comparison, signed distribution/updater/clean-machine acceptance, native GUI/DPI/accessibility and remaining Grabber workflows. The prior workflow inventory is not a parity percentage.

## Technical references

Chrome explicitly does not define ordering between webRequest and webNavigation. The implementation therefore retires only previously committed document IDs rather than assuming a URL notification is a network barrier. See [Chrome webNavigation](https://developer.chrome.com/docs/extensions/reference/api/webNavigation) and [tabs.onUpdated](https://developer.chrome.com/docs/extensions/reference/api/tabs#event-onUpdated).
