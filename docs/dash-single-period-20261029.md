# Single-Period DASH timing integration

Known-duration recorded DASH catalogs now use the existing Period presentation contract even when the MPD contains only one Period. The browser carries declared per-track clocks, Period duration, identity and selected metadata through the same audio/subtitle selection and index-expansion path as multi-Period recordings. Unknown-duration catalogs and the legacy direct parser API retain their previous flat-plan path.

A new desktop capability, dash-single-period-timing, gates this handoff. Chromium and Firefox are version 0.63.73. Single-Period plans may omit declared media codecs because no cross-Period matching is required; multi-Period matching still requires them. The native assembler still probes actual output codecs and stream composition.

The existing Period assembly applies per-input presentation offsets, retains timestamps, bounds output to the Period duration, handles already-normalized MP4 WebVTT without subtracting its offset twice, and checks output streams and coarse duration. Single-Period video without a separately selected audio track now also maps its optional embedded audio, preserving the legacy merger's behavior. This permits zero or one embedded audio stream, while explicit audio selection and multi-Period rules remain unchanged.

Recovery uses the existing Period identity, which includes duration and track clocks. Display-only durationMs remains excluded from legacy selection identity. A legacy flat selection is not silently upgraded during URL refresh; incompatible selection contracts preserve saved data and require a new capture. Existing exact-size publication still measures the assembled file that is retained before confirmation.

The prior [contract review](dash-single-period-contract-review-20261029.md) is superseded for known-duration browser catalogs by this implementation. Sample-accurate boundaries, decoder continuity, synchronization, unknown-duration timing, and installed browser behavior remain unverified. Stream-copy trimming is packet-granular; duration checks are coarse and do not prove every decoded sample is correct.

All four changed JavaScript files pass syntax checks. Both native builds passed, including the final embedded-audio handling, and installer packaging passed. See [build evidence](validation/dash-single-period-build-20261029.json) for source and installer hashes. The production bundle manifest records all 147 payload files. Runtime, parser, browser and decoder tests remain deferred at user request. No installation or publication is performed.
