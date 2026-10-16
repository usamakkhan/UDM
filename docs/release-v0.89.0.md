# UDM 0.89.0

This cumulative release advances from UDM 0.88.0 with wider support for clear HLS playlist variables and more resilient live recording.

## Clear HLS playlist variables

- Resolve imported and locally defined HLS variables in browser and native playlist handling.
- Preserve nested audio-variable values while reloading a live playlist.
- Validate resolved addresses before handing a playlist to the native recorder.

## Live-recording resilience

- Keep declared HLS gap segments explicit rather than attempting to download them as media.
- Continue a live recording after a declared gap while retaining the captured playable prefix.
- Accept a completed segment that accompanies low-latency HLS parts without recording the media twice.

## Verification

Focused browser, Edge handoff, native-recorder, and backend-matrix evidence is recorded under `docs/validation/`. This package is built from this tagged source; full IDM parity remains outside this release.
