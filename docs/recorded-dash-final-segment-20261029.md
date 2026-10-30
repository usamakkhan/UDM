# Recorded DASH final-segment bounds

Extension 0.63.68 applies inherited SegmentTemplate endNumber to recorded single-Period and multi-Period plans. Native expandDashTimeline already honors this bound, but the recorded browser parser previously generated every duration/timeline-derived URL without consulting it. The correction prevents requests beyond the declared final number while retaining earlier Period/timeline bounds.

Fixed-duration expansion caps the count at the final number. A finite endNumber can bound a final open timeline repeat or a fixed-duration representation when a Period duration is unavailable. Timeline expansion stops only after emitting the final segment, including when it falls at an entry boundary. Safe-integer checks cover clocks, segment starts/ends and numbers; the inclusive final number does not require incrementing beyond JavaScript's exact-integer range. The existing combined initialization/media segment budget still applies.

Invalid endNumber values, an end before startNumber, overlapping timeline entries and open repeats before an implicit next start are rejected. Timing attributes accept unsigned decimal integer text. Initializations retain their existing position and URL/range behavior. No native transfer protocol or capability change is required: the browser emits the existing explicit segment contract.

The implementation uses UDM's existing native bound as the behavioral reference. General timeline terminology was checked against DASH-IF's timing-model guidance: https://dashif.org/Guidelines-TimingModel/ . This is not runtime evidence of IDM behavior.

Chromium and Firefox media scripts are mirrored. Both syntax checks and installer compilation passed. Four staged extension files and all five unchanged native executables were hash-verified. See the [build record](validation/recorded-dash-final-segment-build-20261029.json). The installer is unsigned. Native production binaries are unchanged from the successful dash-template-initialization build. Runtime browser, transfer, boundary and playback tests remain deferred at user request. No installation or publication is performed.
