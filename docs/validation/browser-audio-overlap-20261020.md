# Concurrent streaming-audio discovery — extension 0.63.24

Streaming audio previously waited for the desktop capability reply before asking
the page for its captured playback session. The two bounded, read-only checks
now run concurrently. Submission still requires supported desktop capabilities,
a valid session, and a fresh document/track validation. Direct audio is unchanged.
Promise.all observes failures from both checks; a late reply cannot submit a job.

Both browsers passed 49 video transport, 43 direct-audio, 31 streaming-audio and
192 ad/page checks: 630 total. Deferred-reply tests prove both checks start before
either resolves, submission waits for both, and late success after either failure
does not create a download. Results: `browser-audio-overlap-20261020.json`.
No live audio latency improvement is claimed from these tests.

Installation completed successfully. All four changed Chromium/Firefox files
were SHA-256 verified against the staged files. Receipt:
`browser-06324-installed-20261020.json`. Runtime Chrome reload and live audio
verification remain pending; version 0.63.24 includes the earlier 0.63.23 change.

## Live Chrome verification

After the user confirmed reloading 0.63.24, the reference video's default English
M4A selection opened File Info without the main download list. Before Start,
File Info reported exactly 5908172 bytes, prefetched and verified. Start continued
the prefetch and completed a file of exactly that length. FFprobe found one AAC
audio stream, no video, and duration 365.273107 seconds. Full audio decode using
FFmpeg returned exit code 0. Evidence: `browser-audio-06324-20261020.json`.
This verifies the workflow and size, not an end-to-end latency improvement.
