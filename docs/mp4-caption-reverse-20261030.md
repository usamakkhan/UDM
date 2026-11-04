# Reverse MP4 WebVTT playback edits — 1.6.0

The caption edit parser now decodes the signed 16.16 rate explicitly, without relying on an implementation-defined unsigned-to-signed cast. Finite reverse edits accept magnitudes from 1/65536 through 16. Forward edits, finite dwell edits and normal-rate empty edits retain their existing handling. Open-ended reverse edits, magnitudes above 16 and invalid media start times remain rejected.

For reverse playback from media time M, a cue spanning A to B maps to elapsed movie time (M-B)/abs(rate) through (M-A)/abs(rate), intersected with the edit. Cues wholly above M are skipped; cues crossing M are clipped at movie elapsed time zero. For example, a cue spanning media seconds 6–8, played backward from second 10 at -2x, maps to elapsed movie seconds 1–2. This is a mapping example, not an executed test result.

Signed distance subtraction checks overflow before changing clocks. The magnitude then uses the existing quotient/remainder scaler, finite input bounds, movie-duration clipping, presentation offset and zero-time clipping. Text, identifiers and settings are preserved. A stable presentation sort orders output from reversed or repeated sections; cue/text limits count expanded output. Decode continuity remains in source order and is not reversed.

This extends the signed fixed-point edit model documented in the [ISO base media format draft amendment](https://cdn.standards.iteh.ai/samples/75506/23dfb25cd5994763b503c63f82ebf602/ISO-IEC-FDIS-14496-12-DAmd-1.pdf). It is an implementation of the reverse time mapping, not a claim of conformance certification or an observed IDM behavior for these captions. Caption payload text is retained as-is; embedded text timestamps are not rewritten.

Native build: passed for all five production targets at version 1.6.0. Build-start source hashes remained unchanged; binary sizes and SHA-256 hashes are recorded. Runtime/parser/browser tests remain deferred by user instruction. No installer, installation or publication was requested. Installed UDM remains 1.2.8; browser code remains 0.63.82. Evidence: [build record](validation/mp4-caption-reverse-build-20261030.json).
