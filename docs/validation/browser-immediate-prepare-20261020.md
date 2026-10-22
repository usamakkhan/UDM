# Immediate check after playback preparation — extension 0.63.23

When neither direct links nor a captured session were ready, the media handoff
asked the page to prepare playback and then always slept 400 ms before checking
again. It now performs that first check immediately. Subsequent unsuccessful
checks retain their 400 ms spacing and the existing eight-attempt bound.

Document, quality and final transport validation remain unchanged. A pending
native player lookup can still supply the direct pair. No extra download is
submitted by preparation or a late lookup result.

Both browsers passed 49 transport handoff, 43 audio, 27 streaming audio and
192 ad/page capture checks: 622 total. The new regression records timer requests
and verifies that a session available immediately after preparation reaches the
desktop without a 400 ms timer. This is a control-flow test, not a live latency
measurement. An initial test invocation used a nonexistent filename; the corrected
streaming-audio test passed. Output: `browser-immediate-prepare-20261020.json`.

The desktop capture-priority build and four changed extension files were installed
and SHA-256 verified. Receipt: `capture-priority-installed-20261020.json`.
The installed native host/desktop pipe round trip passed. Chrome must reload the
extension before live verification of 0.63.23; no live result is claimed yet.
