# Video capture responsiveness — extension 0.63.21 candidate

The user reported later detection than IDM around YouTube ads, slow video
handoff, lack of apparent prefetch, and the entire main window coming forward.

Changes:

- Removed the 350 ms direct-link grace wait when a verified captured streaming
  session is already available. Direct retrieval and capture still run together;
  document/quality revalidation and single admission remain required.
- The page observer can retain a response identifying the requested video while
  an ad is showing, even when the player still reports the ad's video ID. Explicit
  ad URL markers remain rejected, and UMP identity verification must match the
  page's requested video. Playback quality preparation still avoids acting on ads.
- A fresh, matching readiness marker allows the panel to appear before the ad
  DOM flag clears. Wrong-video and expired markers do not reveal the panel.
  Metadata reads require the verified session before accepting this transition.
- Automatic File Info and duplicate dialogs have an independent hidden owner.
  The main window is disabled during modal review and re-enabled without showing
  it. Removed explicit main-window ShowWindow calls in capture presentation.
- Panel status now describes opening Download File Info. The operation is still
  acknowledged only after desktop acceptance; no fake completion is shown.

Validation completed:

- 45 transport handoff checks for Chromium and 45 for the Firefox mirror.
- 192 page-capture checks covering ad state, differing player IDs, identity
  rejection, and advertisement markers; 72 advertisement-format checks.
- 5 isolated real-Chrome readiness tests and 19 panel regression tests.
- Native capture-presentation harness built and passed 108 checks, including
  independent File Info ownership and preserving hidden main-window state.
- Existing native media suite passed 65 checks, including confirmation/prefetch
  behavior. The saved user setting PrefetchFileInfo was true, Parallel was 3,
  and SkipBrowserFileInfo was false. No settings were changed.

Prefetch already starts from File Info and confirmation can retain its in-flight
transfer. This patch removes handoff and presentation obstacles; it does not
fabricate a speed burst. Prefetch can still be unavailable for unsupported media
plans, duplicates, required recapture or occupied transfer slots.

Evidence: `capture-focus-20261018.json`, `panel-ad-ready-20261018.json`,
`panel-capture-regression-20261018.json`, and
`video-workflow-media-20261018.txt`.

These are source/candidate tests. Extension 0.63.21 and the modified desktop UI
have not been installed or verified against the user's current YouTube ad and
1080p video. No measured click-to-dialog comparison with IDM is claimed. The
separate PAC candidate qualification remains unfinished; this document does not
clear its installation gate.

## Installed-browser follow-up

Installed extension 0.63.21 and the desktop candidate (receipt in
`video-capture-install-20261018.json`). The user reloaded the extension.
On YouTube video qFEuYUfauNY, selecting 1080p H.264/60 fps produced the
accepted handoff status within the first screenshot (~1.5 seconds including
browser automation). This measures acknowledgment, not dialog visibility.
The catalog recorded ConfirmationPending=true and Received=311658892 bytes
without Start being clicked. Real prefetch was confirmed.

The user reported File Info was invisible initially, appeared much later,
and still takes too long. Desktop automation did not expose File Info during
the initial check. Therefore the visible-dialog latency/focus acceptance failed.

The follow-up patch explicitly shows and paints the completed File Info form
before beginPrefetch or metadata work, avoiding reliance on MFC's idle-time
modal display. The presentation test now checks visibility before its screenshot
helper (which calls ShowWindow and previously masked hidden dialogs).
Follow-up native presentation checks passed (114; `capture-visible-20261018.json`). The desktop build succeeded. Installation is waiting for Windows elevation; live confirmation is pending.


## Progress size and prefetch display

The actual completed 1080p test had 392664198 final bytes versus 391995075
reported track bytes, with ContinuedMediaPrefetch=true. A subsequent paused
attempt had Size=-1, ReportedMediaSize=391995075 and Received=142946512.
Progress previously formatted Size alone and therefore showed Unknown even
with valid reported track bytes. It now labels that fallback as tracks with
final size pending; final Size still takes precedence. Received explicitly
says includes prefetch when confirmation continued a background transfer.
The rate remains measured transfer speed, not a fabricated burst.

The native progress harness passed all 201 checks, including reported-size
fallback, prefetched bytes and final-size precedence. Evidence:
`progress-prefetch-20261018.json`. Combined application build succeeded. Windows reported that the elevation request was cancelled; the fixes are not installed or live-verified.


## Live focus failure and pipe correction

The combined dialog/progress build E8E9D95D0730237299E0427084DBB7D5CEB7AEBF2A239786C72CF95997872B7D
was installed and verified (`dialog-progress-installed-20261018.json`). The
user confirmed that the main UDM list still came forward on a fresh 1080p
selection. Prefetch again received 101188579 bytes with ConfirmationPending=true.
The focus requirement therefore remained failed despite the native UI checks.

Inspection found capture-commit explicitly calling the main-window show callback
for accepted captures. The new correction reserves that callback for review
results; accepted captures are presented by the normal UI timer without restoring
the main window. Explicit show commands remain functional. A real isolated named
pipe regression now checks prepare/commit without main activation and explicit
show with activation. All 117 presentation checks passed
(`capture-pipe-focus-20261018.json`). This additional pipe correction is not yet
installed or live-verified.

## Direct media handoff follow-up — October 19

Installed the capture-commit correction, but the live YouTube test still raised
the main window. Its request uses direct media/sabr admission, not capture-commit.
The common direct-admission branch also called show(). That branch now leaves
add/media/sabr/adaptive presentation to the UI timer, preserving explicit CLI,
automation and show requests. The isolated named-pipe test also checks direct
browser admission. A run interrupted before writing a report was rerun; the
completed run passed 118 checks (`capture-direct-focus-20261019.json`).

Installed and SHA-256 verified executable
2771F3B97A9FDBCEAC5F6BBE4D7382ED653441135A09865AFF9A1055ABD9317C
(`capture-direct-installed-20261019.json`). On the subsequent 1080p selection,
the native visible-window inventory no longer included the main UDM window.
The catalog showed 39811075 bytes received with ConfirmationPending=true.
The helper cannot enumerate the independently owned File Info dialog, and the
user changed foreground windows during the check. Precise visible dialog latency
and foreground placement are not yet verified; user confirmation is pending.

The user reported that File Info was still delayed or missing after the direct
handoff fix. Therefore dialog latency/visibility remains failed, even though
main-window activation and prefetch behavior improved. The next candidate gives
automatic File Info WS_EX_APPWINDOW and WS_EX_TOPMOST, explicitly paints it before
prefetch, and retains the independent owner. This makes it reachable when Windows
refuses foreground activation and when a hidden tool-window owner has no taskbar
entry. The isolated presentation suite passed 124 checks, including those styles
and visibility before rendering (`capture-reachable-20261019.json`).
Installation and live verification of this additional change are pending.

## Installed reachable-dialog live result — October 19

Installed and verified SHA-256
24FC598813AE225B3D9738F314B2FC495075029A8EB015B1158C22A02DABE655.
The current YouTube video offered 1080p/60 H.264. File Info was visible and
controllable at the first native inventory, 5476 ms after the browser click;
this includes tool and agent latency and is not a precise UI rendering duration.
The main UDM window was absent. File Info showed 297.87 MB and, under More,
Downloaded: 85.10 MB. Waiting for your confirmation. A prior catalog snapshot
confirmed 74616279 bytes received before Start. Clicking Start retained those
bytes and set ContinuedMediaPrefetch=true.

The first attempt failed with Windows HTTP read error 12030 at 217626732 bytes.
The main window was then explicitly opened for recovery, not by browser capture.
Progress correctly displayed the track-size fallback and includes prefetch.
Resume reused 215689418 saved bytes and completed the same job. The output is
314419781 bytes, H.264 1920x1080 plus AAC, duration 3095.127075 seconds. Three-second
video/audio decode samples at 0, 1545 and 3085 seconds passed. This was a recovered
transfer, not an uninterrupted success. Reported track bytes were 312336826;
they were not the exact final container size. See
`browser-1080p-reachable-20261019.json` and
`capture-reachable-installed-20261019.json`.

## Current installed build — October 20

A fresh Chrome tab on the reference video ZUjiBTaltvQ offered 1080p H.264.
Selecting it opened a visible File Info dialog at the first desktop inventory;
the main UDM window was absent. Precise click-to-paint latency was not measured.
The initial displayed size was 54.03 MB. While confirmation remained pending,
the catalog recorded 56649896 received track bytes. Prefetch then finished and
File Info explicitly showed “Exact final file: 56731539 bytes” before Start.

Start completed the file with ContinuedMediaPrefetch=true and a final size of
56731539 bytes, matching the exact pre-confirmation display. FFprobe identified
1920x1080 H.264 video and AAC audio, duration 365.273107 seconds. A complete
audio/video decode with FFmpeg exited successfully, without error output.
No UDM windows remained in the desktop inventory after completion. This run
did not exercise automatic network recovery and does not establish an exact
final size immediately on selection: the exact value appeared after prefetch.
Evidence: `browser-1080p-prefetch-20261020.json`.
