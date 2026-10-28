# Captured-media restart recovery

Catalog reload now normalizes the media phase for ordinary captured video and
audio-only downloads as well as HLS/DASH jobs. Interrupted active transfers recover
as Paused, and completed, failed and queued jobs receive their matching phase.
Unfinished jobs lose obsolete prefetched-final-size metadata. Retained tracks and
segments are not deleted or changed by this recovery step.

File Info already requires an active confirmation job before displaying the
prefetched exact-size claim. This change corrects the underlying saved metadata
and phase; it does not recover an unconfirmed assembled file across restart.

## Validation

The native `--adaptive-prefetch-checks` suite passed **256 checks, 0 failures**.
Recovery tests create and reload isolated catalogs for video, audio-only and
adaptive media in Downloading, Merging, Paused, Failed, Complete, Awaiting
confirmation and Queued states. They verify phase/status, cleared confirmation
and obsolete size claims, and unchanged retained track/segment bytes. Existing
media timing, exact-size publication, cancellation and folder-change tests passed.
Test and desktop builds succeeded; `git diff --check` passed.

Candidate SHA-256:
`8038816C9A45787C18FFDEC2E111D7D67E8EB8A53137C4583FF31AA5AEA7B600`.
Installation succeeded; the installed executable hash matches the candidate.
No fresh live-browser comparison was
run for this catalog-recovery update, and the extension is unchanged.

[Validation and installation record](validation/media-restart-recovery-20261027.json).
