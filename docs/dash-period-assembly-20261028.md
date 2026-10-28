# Native DASH Period assembly

`DashPeriodAssembly.hpp` adds local assembly for the Period-aware transfer layout. Each Period's selected media track is assembled separately from its own ordered parts, preserving its initialization data. FFmpeg receives each track's negative presentation-time offset with retained input timestamps, then produces a bounded Period output using stream copy. The intended boundary behavior still requires runtime qualification.

Each Period output is probed for selected stream counts and height, codec configuration hashes, stream parameters and time bases. The join rejects incompatible probe signatures. A local ffconcat manifest uses generated relative filenames and explicit Period durations. The final output is probed again for selected tracks and timeline duration. The half-second duration tolerance is a coarse completeness guard, not sample-accuracy, decoded-frame continuity or complete-download proof.

WebVTT cues are parsed with existing bounded code, shifted by their track clock, clipped to their Period and placed on the combined output timeline. Captions are encoded once into the final output. Audio and subtitle labels/languages use existing metadata helpers. Local-only protocol restrictions apply to media-helper inputs.

The implementation follows the [FFmpeg concat requirements](https://www.ffmpeg.org/ffmpeg-formats.html#concat) for matching streams and uses [documented timestamp options](https://www.ffmpeg.org/ffmpeg.html). Stream copy is packet-granular; changing codec configurations, gaps/overlaps and unsupported boundary cases remain open work rather than silently re-encoded content.

## Integration status

The adaptive workflow has an assembly branch ready for Period layouts. It retains the existing final-file hash, exact-size confirmation wait and publication flow. Successful publication cleans generated assembly files and downloaded parts through the existing cleanup path; failed assembly leaves working files for diagnosis/retry, and does not publish a partial result.

Admission is still explicitly disabled. Nested credential scopes, refresh admission, browser index expansion and capability-gated handoff must be integrated before it can be enabled. The new branch has not been exercised at runtime. Runtime tests are deferred at the user's request.

## Build

See `validation/dash-period-assembly-build-20261028.json` for native and setup compilation. Browser 0.63.59 is unchanged. The candidate is unsigned, uninstalled and unpublished. This is an implementation increment, not completed multi-Period or live DASH parity.
