# Native host cold-start recovery — candidate 0.63.20

Follow-up investigation identified three runaway PAC evaluator workers created
by the cancellation fixtures. See [the worker-lifetime failure](pac-worker-lifetime-20261018.md).
The larger handshake allowance is not a fix for those workers or proof that the
candidate is ready to install. The whole PAC execution lifecycle remains gated.

The full Edge extension failed three initial native handshakes with a five-second
deadline, before any download command. An isolated host-only diagnostic returned
valid replies in 3.794, 2.690 and 1.473 seconds. A temporary fifteen-second diagnostic
extension completed the full Edge PAC workflow. CPU saturation was observed during
the investigation; its contribution to each failure is not independently proven.

The Chromium and Firefox packages now allow fifteen seconds for negotiation.
This is an upper bound, not a delay: an arriving `hello` immediately releases
the waiting command. No download replay or handshake retry was added. A startup
timeout explains that no download request was sent; loss of a command acknowledgement
retains the instruction to check the download list before trying again.

## Regression checks

`node tests/native-handshake.test.cjs` first failed against the old implementation:
an eight-second startup was rejected after five seconds. After the change all five
checks passed: immediate responsive startup, slow startup with one admission,
shared startup for concurrent commands, bounded missing-host failure, and no replay
after a lost download acknowledgement. These use a virtual clock and real bridge
code rather than waiting for an unpredictable loaded-machine failure.

All 25 `tests/integration-parity.test.cjs` checks also passed, including legacy
fallback, out-of-order replies, status-only reconnects and no replay of capture commits.
The integration command now imports and awaits the five handshake regressions;
the combined run passed all 30 checks. The handshake file remains runnable alone.
Chromium and Firefox bridge files have identical SHA-256:
`5752D6C56D23EF80FCF430D97879BE1C4891A8B86D6345991C6576F38F4FC43A`.

The matching 1.0.3 native host was rebuilt and passed all nine real protocol checks.
Its SHA-256 is `F710EA1AB8DFFFC8D7786AEB2C5399D3B5E1A1FE85A96890CF1DF927F28D5F26`.
The earlier installed host reports 0.92.0; rebuilding alone did not fix the timeout.

The actual 0.63.20 Edge workflow passed all nine checks with exit 0 and no
diagnostic source override. Both 4,194,335-byte downloads matched the expected
hash; native ranges followed the selected proxy or DIRECT route, browser
ownership transferred only after acknowledgement, and configuration remained
protected. See `native-handshake-edge-06320-20261018.json`.

No installation has been performed. After the runaway workers were stopped,
the complete `--connection-checks` run passed 471 checks with zero failures,
including the media checks that previously timed out. The sanitized log is
`browser-pac-connections-after-cleanup-20261018.txt`. This establishes a passing
rerun, not independent attribution of every earlier timeout to those workers.
A new `--browser-proxy-checks` entry point allows the native
capture contract to be verified independently. Its completed build passed all
59 checks; the log is `browser-proxy-focused-20261018.txt`. This does not clear
the separate PAC evaluator-lifetime defect.
