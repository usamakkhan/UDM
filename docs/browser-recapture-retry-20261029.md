# Browser recapture retry policy â€” 1.2.4

Automatic queue retry admission and retry-exhaustion classification now recognize RequiresBrowserProxyCapture and RequiresBrowserSessionCapture. Repeating the same transfer cannot reconstruct the missing browser context, so such failures remain available for recapture rather than consuming attempts indefinitely. They are not classified as exhausted retry work that allows queue completion actions.

The existing authentication-prompt, request/media recapture, server-backoff and content-review exclusions remain. This does not change credential storage or bypass browser routing. After recapture clears the requirement, ordinary retry/resume policy applies. The shared helper is used by direct transfer retries and synchronization metadata retries.

This functional fix advances the native version from 1.2.3 to 1.2.4 under the user-requested versioning policy. The 1.2.3 synchronization restoration publication-state fixes are included. Extension code is unchanged at 0.63.82.

All five production targets compiled and installer packaging passed for 1.2.4. See [build evidence](validation/browser-recapture-retry-build-20261029.json); the production manifest records 147 payload hashes. Runtime, network, browser and queue testing remain deferred at the user's request. Nothing is installed or published by this batch.
