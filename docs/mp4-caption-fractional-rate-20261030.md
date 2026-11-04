# Fractional MP4 WebVTT playback edits — 1.5.0

Recorded MP4 WebVTT edit-list parsing now preserves the complete unsigned representation of the positive 16.16 rate instead of requiring an integer and discarding its fractional bits. Supported forward rates range from 1/65536 through 16. Finite zero-rate dwell edits retain their separate handling. Reverse rates, rates above 16 and non-normal-rate empty edits remain rejected.

Assembly maps relative media ticks using ticks * 65536 / (track timescale * stored rate), then applies movie placement and the DASH presentation offset. It never directly multiplies a potentially large timestamp by 65536. Integer division separates whole seconds; sixteen bounded remainder-doubling steps produce the exact scaled quotient and remainder. Overflow checks protect signed accumulation and offset subtraction. Floating-point conversion occurs only for the final subsecond fraction, using the existing millisecond rounding.

Finite edit input bounds also split multiplication at 65536 to avoid overflow. Input clipping remains conservative; final output is independently clipped to the movie-duration boundary. Repeated and reordered edit sections may mix fractional, integer and dwell rates. Existing edit-duration, cue-count, text-size and presentation-time limits remain unchanged. Extremely large scaled clocks that cannot fit the signed representation are rejected rather than wrapped.

For example, relative media seconds 1–2 map to movie seconds 2–4 at 0.5x, while seconds 3–6 map to seconds 2–4 at 1.5x, before movie placement and presentation offsets. These are mathematical examples, not executed acceptance results.

The fixed-point mapping is described in the [ISO base media format draft amendment](https://cdn.standards.iteh.ai/samples/75506/23dfb25cd5994763b503c63f82ebf602/ISO-IEC-FDIS-14496-12-DAmd-1.pdf), page 5 of the printed text. This reference is a draft, not a claim of current conformance certification or observed IDM behavior.

Native build: passed for all five production targets at version 1.5.0. Build-start source hashes remained unchanged; binary sizes and SHA-256 hashes are recorded. Runtime/parser/browser tests remain deferred by user instruction. No installer, installation or publication was requested. Installed UDM remains 1.2.8; browser code remains 0.63.82. Evidence: [build record](validation/mp4-caption-fractional-rate-build-20261030.json).
