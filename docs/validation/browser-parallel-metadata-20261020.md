# Concurrent video metadata reads — extension 0.63.22

Video selection previously awaited current-tab identity, local preferences,
page formats and captured-URL storage in sequence. The extension now reads tab
identity and preferences concurrently, checks policy, and then reads page formats
and captured-URL storage concurrently. This removes two sequential browser API
waits from each metadata lookup, including the final handoff validation.

Policy still gates page injection. Document identity, current-tab revalidation,
ad filtering, selected format, and fresh transport validation remain in place.
Promise.all observes failures from both operations; neither result bypasses the
existing admission checks. Download submission is unchanged.

Tests use deferred API replies to prove that the independent reads overlap,
rather than imposing fragile timing thresholds. Each browser passed 48 transport
handoff, 43 audio, 27 streaming-audio and 192 ad/page checks: 620 total checks
across Chromium and Firefox. Raw test output is recorded in
`browser-parallel-metadata-20261020.json`.

Both source and installed manifests are 0.63.22. All four changed files were
hash-verified after installation (`browser-06322-installed-20261020.json`). Live
browser verification is pending the user's Chrome extension reload.
No end-to-end latency improvement is claimed from these unit checks.
