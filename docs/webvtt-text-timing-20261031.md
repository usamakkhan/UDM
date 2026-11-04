# Plain WebVTT text timestamp mapping — 1.6.1

Plain WebVTT cue boundaries were shifted during HLS timestamp mapping and discontinuity placement, but timestamp tags inside cue text retained the original clock. This could leave word-highlighting timestamps outside their cue or aligned to the wrong recording interval.

The shared text mapper now shifts recognized timestamp tags by the same integer-millisecond offset as the cue. It validates that recognized timestamps increase strictly inside the source cue, preserves other markup and text, and removes timestamp markers outside the final clipped interval while retaining surrounding text. Text expansion is bounded by the existing 64 KiB cue limit. Escaped angle brackets and timestamp-like text inside other tags are not interpreted as timestamp tags.

Integration covers initial plain WebVTT/HLS mapping, recorded HLS discontinuity placement, live HLS epoch placement, and plain WebVTT DASH Period placement. Repeated mapping operates on the already-transformed cue interval. Timestamp markers at or before a clipped start, or at or after a clipped end, are omitted. The helper rejects recognized malformed ordering rather than silently publishing inconsistent timing.

The ordering and interval requirements follow the [W3C WebVTT cue timestamp rules](https://www.w3.org/TR/webvtt1/). This fixes UDM's time conversion; it does not establish an observed IDM comparison. The mapper recognizes complete valid timestamp-shaped tags within the supported recording limits. It does not replace the full WebVTT text tokenizer. MP4 WebVTT payload timestamps remain unchanged because their media/edit mapping needs separate handling.

Native build: passed for all five production targets at version 1.6.1. Build-start source hashes remained unchanged; binary sizes and SHA-256 hashes are recorded. Runtime/parser/browser tests are deferred by user instruction. No installer, installation or publication requested. Installed UDM remains 1.6.0; browser code remains 0.63.82. Evidence: [build record](validation/webvtt-text-timing-build-20261031.json).
