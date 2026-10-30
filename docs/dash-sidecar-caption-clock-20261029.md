# Period-relative standalone DASH captions

DASH-IF IOP v4.3 section 6.4.5 describes side-loaded TTML and WebVTT as one file per Period with cues aligned to that Period. It says presentationTimeOffset, if present, is expected to be ignored for these external files. Segmented captions instead require ISOBMFF handling. Reference: https://dashif.org/docs/DASH-IF-IOP-v4.3.pdf . No reference code was copied.

The recorded browser catalog previously reused inherited media addressing clocks for standalone WebVTT. Multi-Period assembly subtracts each track's presentationTimeOffset before clipping and joining its Period, so a nonzero inherited offset could shift or discard already Period-relative captions.

Extension 0.63.69 now assigns standalone WebVTT the neutral Period clock (timescale 1, presentationTimeOffset 0) in the multi-Period selection contract. Native assembly can continue applying its existing clip-and-join logic. Audio/video clocks are unchanged; single-Period plans retain their existing representation. Existing saved plans are not rewritten.

This corrects standalone caption timing; it does not enable segmented text or MP4 WebVTT decoding. The current catalog still excludes segmented subtitle addressing until a bounded ISOBMFF caption path is implemented. No plain-text concatenation workaround was added.

Chromium and Firefox are mirrored. Syntax checks and installer compilation passed. The [build record](validation/dash-sidecar-caption-clock-build-20261029.json) records the source and installer hashes. The candidate is unsigned. Native production binaries are unchanged from dash-finalized-drain. Runtime caption alignment, playback and migration tests remain deferred at user request. No installation or publication is performed.
