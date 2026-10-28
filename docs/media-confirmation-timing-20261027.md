# Regular media confirmation timing

Regular captured-video and audio assembly now excludes time spent waiting at the
verified-file confirmation gate from elapsed download time. Confirmation and
cancellation preserve measured network transfer time. Successful publication sets
the media phase to Complete; assembly/transfer exceptions report Paused or Failed
and clear obsolete exact-size metadata.

Background prefetch continues before Start. The exact preview size still comes
from the verified assembled file, and confirmation publishes those same bytes.
This change does not synthesize a speed burst or alter measured network speed.

## Validation and installation

The native `--adaptive-prefetch-checks` suite passed **238 checks, 0 failures**.
New regular-media cases wait two seconds before confirmation or cancellation,
then verify exclusion of that idle time, unchanged network timing, final phase,
cancellation without publication, and exact-size equality after confirmation.
Existing media/adaptive assembly, folder-change and recovery checks also passed.
Test and desktop builds succeeded, and `git diff --check` passed.

Installed executable SHA-256 matches the tested build:
`11B3D9B467E8C05AE5B18C1D3E5DCF6060B3924BF1A0252825DD27EE4AF862BC`.
This update was tested with native media fixtures; no new live-browser comparison
was performed. The browser extension was unchanged.

[Validation record](validation/media-confirmation-timing-20261027.json).
