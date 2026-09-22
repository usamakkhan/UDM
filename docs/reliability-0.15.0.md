# UDM 0.15.0 — download reliability and browser capture

This release improves recovery from busy servers, command-line handoffs and media association after embedded-frame navigation. It contains original UDM code. These changes do not establish complete IDM parity or a general throughput advantage.

## HTTP recovery

Ordinary HTTP/HTTPS downloads retry temporary HTTP responses (408, 425, 429, 500, 502, 503 and 504) during the initial range probe and individual range transfers. Permanent responses such as 401, 403, 404 and 410 stop immediately. Expired-link errors keep the existing Refresh download address guidance and saved partial bytes.

Retries obey the queue's attempt budget. Valid `Retry-After` seconds and HTTP dates are honored, using at least the normal exponential delay. A throttle delays new requests by other workers in the same download. Pausing interrupts both probe and range cooldowns. A requested delay over five minutes reports a recoverable error instead of retrying sooner than the server requested. Malformed values fall back to normal backoff. This follows [HTTP Retry-After semantics](https://www.rfc-editor.org/rfc/rfc9110.html#name-retry-after).

This policy applies to the ordinary HTTP range engine. It is not a claim that every adaptive-media or FTP transport now shares that retry policy.

## Browser media identity

Video offers include the browser document identity and navigation timestamp as well as the existing tab, frame, player, source and load-epoch checks. A frame ID can survive navigation, so its old playlists are no longer sufficient evidence that a stream belongs to the current document. The extension also rechecks direct offers immediately before native handoff.

Where document IDs are unavailable, observed playlists must belong to the current page/origin and must be newer than its navigation timestamp. The fallback is covered by a simulated API test; live Firefox validation remains outstanding. API fields follow [Chrome's scripting documentation](https://developer.chrome.com/docs/extensions/reference/api/scripting#type-InjectionResult) and [message sender documentation](https://developer.chrome.com/docs/extensions/reference/api/runtime#type-MessageSender).

Ads within the same current document still require player-specific identity information. This change does not claim universal ad exclusion or universal platform support.

## Command line

First launches and requests sent to an already-running UDM use the same destination and start-mode handling:

```powershell
.\release\UDM.exe --add "https://example.test/file.zip" --folder "D:\Downloads" --name "file.zip" --paused
.\release\UDM.exe /d "https://example.test/file.zip" /p "D:\Downloads" /f "file.zip" /n
```

`--paused` saves the job paused. `--silent` or `/n` skips File Info and queues a new download. Without either option, File Info is shown. `--background` keeps the main window hidden. Duplicate choices still protect existing files and records; silent mode does not automatically authorize replacement. Relative folders are resolved by the launching process before forwarding.

The `/n` alias previously and incorrectly meant paused; scripts that need pause should use `--paused`. Browser native messages cannot issue the local-only command that specifies arbitrary launch destinations. Existing browser confirmation preferences remain separate.

## Validation

- Native regression suite: **216 passed, 0 failed**, including 22 new retry and launch checks.
- Browser and media API suites: **71 passed** (30 browser, 31 cross-site, 4 early capture, 6 UMP capture).
- Live cold/warm launch tests: **five passed against the installed release**, using isolated state and a local byte-exact transfer.
- Real installed Chrome 153 integration: **six passed** — native ping, exact MP4, HLS with verified video/audio, automatic 8 MiB file interception, iframe navigation isolation, and no page-script errors.
- Production native-host ping passed; all three installed binaries exactly match the tested build. The app is running. All ten existing UDM download records are unchanged.

See [machine-readable release evidence](reference/reliability-0.15.0.json).

Reproduction:

```powershell
.\native\build.ps1 -Test
node .\tests\media.test.cjs
node .\tests\browser.test.cjs
node .\tests\capture.test.cjs
node .\tests\ump.test.cjs
$env:UDM_APP_EXE = "$PWD\release-native\UDM.exe"
node .\tests\launch.native-live.cjs .\benchmarks\launch-new-run
node .\tests\browser.native-live.cjs .\benchmarks\browser-new-run
```

The browser test needs Playwright, installed Chrome, the registered UDM native host, and the media helpers. It grants localhost access only in a copied fixture extension, uses fresh temporary browser/state directories, and never changes the user's extension preferences. Test directories must be new; tests must not share the browser fixture port concurrently.

## Competing downloaders and current desktop access

Chrome network logging revealed that IDM replaced the local ZIP response with `HTTP/1.0 204 Intercepted by the IDM Advanced Integration`. This happened even in an isolated Chrome profile without the UDM extension. It explains why the normal Chrome download event failed before UDM could take over.

The final capture test uses a synthetic `.udmtest` file with explicit matching rules in both the isolated extension and isolated native settings. Its 8 MiB output matched the fixture exactly. IDM interception preferences were left unchanged; ZIP interception while both downloaders compete remains unresolved. IDM created local fixture history entries 45 and 46 during the diagnostics; those entries are retained.

The updated production extension files are reachable through the existing installed-folder junction. Reloading the extension in everyday Chrome could not be verified: the Windows control helper repeatedly returned “foreground window did not report a process id,” and no Chrome window was targetable. The successful browser tests used isolated profiles, not the everyday Chrome session.
