# MP4 WebVTT dwell edits — 1.4.0

Recorded MP4 WebVTT assembly now accepts finite zero-rate edit-list entries. A dwell selects the cues active at its media timestamp and holds them for the edit's movie duration. Selection uses start <= media time < end, excluding a cue ending exactly at the held timestamp and including a cue starting there. Future cues are not pulled backward into the hold. An empty sample contributes no cue.

Dwell output uses the movie timescale and movie placement directly, followed by the existing DASH presentation offset and zero-time clipping. It does not divide by zero or convert the held duration using the media timescale. Each active cue keeps its text, identifier and settings. Repeated/reordered edits remain sorted by presentation start, and existing cue/text budgets count expanded output.

The parser accepts rate zero only with a finite nonempty media edit. Empty edits retain rate 1. Zero-duration dwell entries, fractional rates, reverse rates and forward rates above 16 remain rejected. Existing 64-entry and seven-day edit-duration limits still apply. Version 0 and version 1 edit lists share this handling.

The implementation follows dwell semantics in ISO/IEC 14496-12 section 8.6.6.3, reproduced in the [2015 specification](https://b.goeswhere.com/ISO_IEC_14496-12_2015.pdf): hold the media selected by media_time for the edit duration. This is container-format support, not a claim that IDM's behavior for this caption case has been observed. Historical QuickTime documentation excludes rate zero; this support follows the ISO base media file format.

Native build: passed for all five production targets at version 1.4.0. Build-start source hashes remained unchanged; binary sizes and SHA-256 hashes are recorded. Runtime/parser/browser tests remain deferred by user instruction. No installer, installation or publication was requested. Installed UDM remains 1.2.8; browser code remains 0.63.82. Evidence: [build record](validation/mp4-caption-dwell-build-20261030.json).
