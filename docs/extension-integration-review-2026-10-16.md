# Extension and desktop integration review

This review used REA 6.1.0 on the locally extracted IDM 6.43.1 MV3 extension. REA parsed six JavaScript files in a 27-file inventory, with no parse failures or truncated scopes. Evidence ID: `ev_03fa3a10f2e9257982e1106ed1328c2b6183fc0a034a820c92ea23f7b7da419a`. Artifact digest: `b00d0b2e5ff92c4a7170ed3ac0266b763fbc9e1c4876cacc326765f2ce14235c`.

Inspection of the inventoried background script shows persistent native messaging to `com.tonec.idm`, WebSocket connection attempts, disconnect handling, and delayed reconnect attempts. Static analysis establishes code structure, not live behavior or complete protocol equivalence. The proprietary source and full REA graph remain local research artifacts; they were not copied into UDM.

UDM already has its own persistent native messaging, correlated replies, separate player-request channels, durable download acknowledgements, and recovery receipts. This pass fixed two concrete reliability issues:

- A failed Chromium proxy-settings read was cached for 30 seconds, rejecting later downloads without a fresh read. Failed reads are now uncached; the read deadline is five seconds. Unsupported routes remain rejected, and stale callbacks cannot overwrite a recovered configuration.
- A confirmed rejected media submission hid the desktop's error behind a generic retry message. The panel now preserves the original error and explains that no download was added only after the desktop confirms release. Uncertain submissions retain their receipts and are never automatically replayed.

Both source changes are synchronized in Chromium and Firefox assets. Firefox's native proxy capture path is unchanged. The delayed-key integration test now holds Ctrl+Insert before clicking its delayed-download button; previously Playwright could finish the click only after the download had started, making the test press keys too late.

## Validation scope

Real browser/native testing uses Chrome 155, browser source version 0.63.8, and the installed native 0.92.0 app, with an isolated catalog and fresh browser profile. Native UI prompts are suppressed in this automated suite. Test-only localhost/cookie permissions are supplied in the isolated manifest; this does not validate permission prompts or extension-store installation.

The final run covers link selection and Download later, authenticated range downloads, persistent host reuse, direct MP4 integrity, HLS assembly, browser-to-desktop ownership, force/bypass keys, delayed downloads, disabled capture, document navigation, and catalog cleanup. Both resulting MP4 files also passed full video/audio decoding with FFmpeg `-v error -xerror`.

Additional passing checks: 56 browser, 25 integration, 86 proxy-routing, 6 media-error recovery, 4 proxy-read recovery, 42 YouTube audio, 24 YouTube page/ad capture, 72 ad-marker, 9 real native protocol, and 2 real browser worker-restart checks. These targeted suites are not the entire project test suite.

Earlier failed runs are retained in the validation summary: an old staged executable lacked media tools, D: ran out of space, the delayed-key test raced the download, and proxy metadata acquisition rejected a link batch. One rerun also encountered the previous run's still-closing fixture port. Final evidence must be read alongside these failures.

The fresh-profile public YouTube test was blocked by Google's unusual-traffic challenge before playback. This pass does **not** establish a completed live YouTube download, full IDM parity, native dialog behavior, or live Firefox/Edge parity. The earlier same-document YouTube navigation fix remains covered by regression tests. No CAPTCHA was bypassed.

The REA output was moved intact to the system temporary directory on C: to free approximately 100 MB on D:. Disposable profile deletion was rejected by automatic approval review; those profiles were left in place. D: remains nearly full. These extension edits require reloading the user's extension before they affect that browser; no release or installed native binary was changed by this pass.
