# Fractional media-tick MP4 caption clocks, UDM 1.30.0

Reviewing reverse/dwell timed text exposed a bug in the existing forward mapper: it converted a WebVTT millisecond offset to whole media ticks before scaling playback rate. At track timescale 1, a 500 ms inline offset became zero ticks and its marker could disappear at the cue boundary. Non-divisible timescales also lost part of the offset. This was independent of the edit parser's supported fixed-point rates.

Under the existing ctim-to-sample anchor contract, the intended presentation time is:

```text
movieStart/movieScale - presentationOffset/offsetScale
+ ((sampleStart - editOrigin)/trackScale + (inlineMs - ctimMs)/1000)
  * 65536/rateFixed
```

The mapper now retains the inline offset as a separate rational term: numerator deltaMs * 65536 and denominator rateFixed * 1000. Bounded millisecond deltas and rate magnitudes keep those products within int64. It splits integer seconds and remainders, adds checked whole seconds to the existing sample clock, incorporates presentation/movie offsets and normalizes whole-second carries before the presentation bound and final millisecond rounding. It does not construct sampleStart + delta-in-media-ticks before subtracting the edit origin. Origin subtraction itself is guarded. Untimed cue callers retain zero inline delta through the existing helper defaults.

Worked arithmetic examples, not executed test results: with a zero sample/edit/presentation origin, a 500 ms inline offset maps to 500 ms at 1x, 250 ms at 2x and 1000 ms at 0.5x, regardless of a track timescale of 1. At timescale 3, a 500 ms offset no longer becomes a one-tick/333 ms offset before scaling. Existing missing-ctim, monotonic-marker, clipping, output-budget and bounded-presentation checks remain in place. Negative inline deltas retain signed rational remainders. Floating-point boundary equivalence, final-player animation and all transport/edit combinations remain unverified.

The same retained-parts decoder serves ordinary adaptive captions and retained live-DASH captions. Caption assembly revision 4 binds Period and joined-session outputs to this implementation. Revision-3 caption output receipts do not match and must be rebuilt from retained verified fragments; recording selection, fragment byte hashes and resource identities are unchanged. Audio/video-only output bindings remain unchanged. Recorded non-live adaptive output is assembled anew by its existing path.

Reverse/dwell timed payloads remain open. The [WebVTT specification](https://www.w3.org/TR/2026/CRD-webvtt1-20260520/) defines millisecond timestamps, increasing cue-text timestamp order and playback-dependent past/future styling. Reversing timestamp values in place would violate that ordering; removing them would discard temporal text state. Supporting these edits requires preserving the cue tree and state transitions through freezing/reversal and the chosen output/mux format, not just reversing cue endpoints. This increment fixes a clock prerequisite and does not enable those edits or establish an observed IDM behavior.

See [source-inspection evidence](reference/mp4-caption-subtick-clock-20261104.json) and [build receipt](validation/mp4-caption-subtick-clock-build-20261104.json). Runtime/parser/browser/player tests, installer creation, installation and publication remain deferred by instruction. Full IDM parity is incomplete.
