# Remaining reverse/dwell timed-caption work

The current MP4 WebVTT assembly supports ordinary cue payloads under finite reverse/dwell edits, but rejects embedded timestamped payloads in those edits. Forward rate-scaled mapping is already implemented. The rejection is still present after 1.53.0; this review does not reclassify it as complete.

The [W3C WebVTT specification](https://www.w3.org/TR/webvtt1/) requires internal timestamps to increase within a cue. Its past/future styling also depends on playback time. Reversing mapped timestamp numbers would violate ordering; removing them could change styled rendering. Correct support needs an output representation for the changing or held cue state, with markup/style and output-codec behavior considered together. That conclusion is an inference from the format rules and current decoder, not observed IDM behavior.

No caption implementation or tests changed in this review. A separate source-backed remote ZIP cancellation improvement was completed as 1.53.0. Complete reverse/dwell timed payloads, TTML and current player acceptance remain outstanding.
