# Immediate browser presentation — October 20

Accepted browser captures previously waited for the main window's 500 ms timer
to discover pending dialogs. PipeServer now has a separate presentation callback
for accepted capture commits and direct add/media/SABR/adaptive admissions.
The desktop posts a presentation message without activating the main window.
The timer remains available for recovery and requests deferred by modal dialogs.

The first two UI runs exposed File Info losing WS_EX_TOPMOST on the second and
third confirmations. Diagnostics showed the independent owner was still marked
and WS_EX_APPWINDOW remained set. Keeping that hidden owner topmost corrected
the repeated-dialog case. The owner is never shown and never owns the main list.

All 128 native presentation checks passed after the correction. The suite uses
the new message handler with the ordinary refresh timer disabled, and checks
that real pipe admissions notify presentation without requesting main activation.
Preparing a capture and read-only ping requests do not trigger presentation.
The first failed report is retained in `capture-notify-first-run-20261020.json`;
the passing report is `capture-immediate-notification-20261020.json`.

This removes a possible polling delay of up to 500 ms, not the entire browser
handoff time. The standalone application built successfully and was installed
with verified SHA-256 831B071C33FBF9547AFF4752A719A9FC372355EEA57F0F017649A04AE79220E5.
Receipt: `capture-immediate-installed-20261020.json`. Extension files did not
change and remain version 0.63.21.

The subsequent live Chrome 1080p selection produced visible File Info without
the main list. The first desktop observation was 7828 ms after starting the
browser click; this includes tool and agent latency and cannot establish actual
click-to-paint time or a measured improvement. Before Start, the app received
56649896 track bytes and then displayed the verified final size of 56731539
bytes. Start published that size with ContinuedMediaPrefetch=true. Its SHA-256
matches the earlier test file that passed a complete audio/video decode.
Evidence: `browser-immediate-capture-20261020.json`. No live connection failure
occurred, so this does not verify automatic retry under live network interruption.
