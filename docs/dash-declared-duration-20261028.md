# Recorded DASH duration in the download list

The existing browser DASH parser reads the single Period duration, falling back to MPD mediaPresentationDuration minus Period start. Previously it used that value to expand segment templates but omitted it from the native handoff. The native list's duration fallback therefore worked for HLS/SABR but could remain blank for DASH even when the presentation declared a duration.

Both browser families now attach `durationMs` for a finite positive declared duration of at most seven days. Missing or out-of-range durations are omitted; they are not guessed from bitrate or byte size. Native plan validation accepts the field only for recorded DASH, requires an integer within the same bound, and saves it as `MediaLengthMs` at admission.

The existing Size-column renderer shows known bytes first. It uses the stored media length only when byte size is unknown and suppresses length for live recording. This is presentation metadata, not an exact final-size claim. HLS segment-duration handling is unchanged, and selecting another audio track or audio-only output retains the same presentation duration through the existing plan-copy path.

## Scope and remaining gaps

This does not add dynamic/live DASH, multiple Periods, encrypted media or timeline-derived duration when no overall duration is declared. The current browser parser still rejects dynamic MPDs and multiple Periods; native live admission remains HLS-only. Those are implementation gaps, and exact IDM behavior for each case still needs reference evidence.

Both extension manifests are 0.63.56. JavaScript syntax checks, native production compilation and setup packaging are recorded in `validation/dash-duration-build-20261028.json`. Runtime and visual tests remain deferred at the user's request. No browser reload or installation is performed.

The follow-up [refresh compatibility correction](dash-refresh-duration-20261028.md) permits older captures without duration metadata and updates display metadata transactionally on accepted refresh.
