# Release history

Milestone headings use the reconciled two-digit sequence. Parenthetical recorded numbers identify the original build or changelog values. The [archived tag ledger](#archived-tag-version-ledger) covers tagged milestones that lack a paired heading.

## Unreleased

## 4.74 / browser 0.68.38

### Reuse the FTP control session for the final file check

- Reuse the authenticated session that received the successful end-of-file reply for the final size and timestamp check. This avoids another connection and login when the session is still responsive.
- Give the reused session a two-second budget for both metadata commands. Fall back to a fresh login if the reused session cannot answer the checks or when all ranges were already saved. A changed file still fails validation before publication.

## 4.73 / browser 0.68.37

### Assemble the same numbered parts verified for a saved prefetch

- Keep verified FTP and legacy HTTP prefetch parts open through staging so a path cannot be replaced or opened for writes between receipt verification and assembly.
- Keep receipt-based repair for damaged parts. Reuse one read buffer during verification and another during assembly. Continue hashing the staged output as it is written, including when UDM removes an inserted content prefix.

## 4.72 / browser 0.68.36

### Match the browser panel frame and controls

- Add the continuous beveled blue frame from the IDM reference to the full and compact video and selected-link strips. Match the inset controls, compact button height and green arrow with UDM artwork.
- Keep the compact quality menu aligned with the right edge of the strip and add the inner highlight visible in the reference menu.

## 4.71 / browser 0.68.35

### Match the Other qualities prompt and one-row review

- Show Other qualities in a Windows message box owned separately from the main UDM window. Keep the browser prompt available when the desktop does not support the request.
- Show Download all above a single format when the picker also offers Other qualities. The command opens the desktop review, while selecting the format opens Download File Info.

## 4.70 / browser 0.68.34

### Review YouTube video qualities together

- Download all prepares the selected YouTube video qualities for one desktop review. Direct playback URLs and captured SABR tracks use the same validation and protected storage as individual video downloads.
- Recheck the video visit, selected qualities and stream identity before sending the batch. If a quality cannot be prepared, identify it before sending anything. Batch preparation does not switch the web player's playback quality.

## 4.69 / browser 0.68.33

### Review direct files and adaptive video together

- Send final non-YouTube video catalogs to one desktop review, including mixed direct files and HLS or DASH formats. Adaptive rows keep their captured track plans and origin-scoped request context. Their source address and credentials are fixed in the review.
- Add checked rows in the existing catalog transaction. Adaptive rows bypass ordinary file-size probes against the source page, and different stream formats can share that page address in the list.

## 4.68 / browser 0.68.32

### Review direct video formats together

- Download all opens one desktop review when every format in the catalog is a direct media file. Each row shows its quality and receives a distinct suggested filename. The desktop adds only the rows checked in the review.
- Recheck the originating video, document, capture settings, and browser request context before handing off the batch. A review with an uncertain acknowledgement is never retried as individual downloads.

## 4.67 / browser 0.68.31

### Guide users to additional qualities in the web player

- Add Other qualities to the video menu when UDM has captured a playable format from a single blob-backed player without reading a variant catalog. The prompt explains how to select and play another quality, then reopen the panel.
- Keep the menu open for explicit selection when that command is offered. The guidance closes when the player changes or the panel is disabled.

## 4.66 / browser 0.68.30

### Open File Info directly for a sole confirmed video format

- One activation of the video strip sends a sole confirmed format through the usual desktop confirmation path. The picker remains available for provisional results, additional tracks, warned or live streams, and explicit keyboard browsing.
- A canceled or replaced catalog cannot select a format. After a handoff error, reopening the panel offers the choice and shows the warning before another handoff.

## 4.65 / browser 0.68.29

### Match the compact video menu spacing

- Give titled format rows the trailing space visible in IDM's compact menu. Reduce that space when the viewport or desktop menu width setting is narrow, and keep the full and options menus sized to their contents.
- Show the video strip after its first placement so it starts next to the player.

## 4.64 / browser 0.68.28

### Align browser panel and right-click controls with IDM

- Keep the numbered video format menu separate from panel controls. Open panel controls with the question mark on the full strip, or by right-clicking or pressing Shift+F10 on the compact strip. Opening options does not look up video formats.
- Give the compact format list a gray menu gutter and retain the format's own quality label.
- Show the tab toggle and UDM Browser Tools on the toolbar menu. Match IDM's enabled and disabled toolbar tooltip layout, and offer the link command on HTTP and HTTPS images as well as links and direct media.

## 4.63 / browser 0.68.27

### Prefetch resumable FTP downloads from File Info

- Start receiving into numbered parts before confirmation when FTP resume can be verified. Check file details and wait for Start Download before opening a data transfer on non-resumable servers.
- Keep publication behind confirmation. A completed FTP prefetch can be verified from its numbered parts on a later Start, even if the server is unavailable.
- Exclude the time waiting for confirmation from transfer and elapsed measurements.

## 4.62 / browser 0.68.26

### Keep Download File Info updates lightweight

- Copy only the fields the dialog displays and the two track sizes and addresses. Leave saved ranges, capture plans and private request context on the live job during polling.
- Check eligibility against the live job, including its current tracks and any draft login. Explicit preview commands still capture the request context they need.

## 4.61 / browser 0.68.25

### Open Download File Info before setting up background work

- Begin prefetch and metadata work from a posted dialog update after initialization. Skip automatic prefetch if a draft login or an explicit metadata check is already in use.
- Report a background startup error in File Info and keep the confirmation controls available.
- Update the displayed size, filename, status, and preview controls only when their values change.

## 4.60 / browser 0.68.24

### Reduce catalog work before browser video confirmation

- Use the live jobs as the in-memory history after startup, so a save does not also copy the original history array.
- Prepare the media request fingerprint before taking the catalog lock. Check an existing receipt without copying the journal.
- For video and adaptive captures, save only the armed refresh record for rollback. Validate the adaptive plan at admission and commit the download and receipt together.

## 4.59 / browser 0.68.23

### Begin HTTP resume with the missing range

- For a saved download with a consolidated partial file, a complete range plan and a strong ETag, request an unfinished range directly with `If-Range`.
- Reuse a matching response after checking the local offsets, then verify the overlap bytes before writing. Keep the existing probe for refreshed links, numbered-part-only layouts and nonmatching partial responses.
- If the server returns a complete response, retain it for the existing restart path and apply the saved-identity protection policy.

## 4.58 / browser 0.68.22

### Recover downloads after a stalled HTTP response through a proxy

- Report a stalled response as a header or body interruption so ordinary downloads use the existing retry and resume rules.
- Keep an accepted original response running if an optional range helper stalls.
- Keep response stalls out of proxy-route failover. An interrupted form submission still requires an explicit restart.

## 4.57 / browser 0.68.21

### Set how long UDM waits for a server response

- Set a server response wait of 1 to 300 seconds from Options → Connection → More. Existing profiles default to 30 seconds.
- Use the setting for HTTP and HTTPS receives on WinHTTP and explicit proxy routes, and for FTP replies and data reads.
- Apply changes when the next transfer or preview takes its settings snapshot.

## 4.56 / browser 0.68.20

### Show actual connection starts and coverage in download progress

- Place HTTP and FTP markers where each active connection starts saving new bytes, excluding the overlap it rereads to verify the file.
- Show the byte coverage of separate video and audio tracks when their sizes are known. Audio-only transfers now show their connection rows.
- Fill the map when the file is complete, including a saved output that has no remaining range records.

## 4.55 / browser 0.68.19

### Hand adjacent HTTP ranges to a worker that has finished

- Take the next untouched range when its assigned worker is still opening the HTTP request.
- Cancel the displaced request and discard any late reply before it can change the range or write file bytes. The displaced worker can claim other work.
- Keep existing response, validator and overlap checks for the new owner.

## 4.54 / browser 0.68.18

### Keep accepted HTTP ranges running when an extra connection is forbidden

- Split an owned range after its response and any required resume overlap pass validation.
- When retries are enabled, return a range refused with HTTP 403 to the remaining workers if one still has an accepted response. Retire the refused worker for this transfer.
- Preserve the terminal error path when there is no accepted peer or the refusal includes a usable retry instruction.

## 4.53 / browser 0.68.17

### Keep the original HTTP download when a helper fails

- Skip the initial range probe when the server advertises `Accept-Ranges: none`.
- Keep the original response when an optional range is refused, fails to connect, or returns a partial response that cannot be verified against the original.
- Apply transient server backoff to the next request while the accepted response continues.

## 4.52 / browser 0.68.16

### Refine the IDM-style browser controls

- Match the reference strip bevels and compact menu colors, spacing and selection highlight. Keep the format number visible and use more of each row for the title and format.
- Open the quality list below the strip when the list fits. Open upward when the lower space is insufficient and more room is available above.
- Use UDM Integration Module as the visible browser extension name.

## 4.51 / browser 0.68.15

### Bound HTTP reads at reassigned range boundaries

- Limit each ranged body read to the worker's current span, including the pipelined read started before writing the previous buffer.
- If the span shrinks during an outstanding read, apply the live boundary to the write. Fully consumed responses still get an end-of-response check.

## 4.50 / browser 0.68.14

### Start and reassign HTTP ranges promptly

- Use the retained full response immediately when the file has one planned connection and the server advertises byte ranges with a validator. Check any later resume response before appending.
- Wake waiting workers when the midpoint probe finishes, a connection releases its range, or the transfer finishes. Keep timed checks for cancellation and eligibility based on elapsed time.

## 4.49 / browser 0.68.13

### Collect selected links from the frame that opened the menu

- Scope the selected-link menu to the document where you right-clicked. Reject a frame URL that changed before collection.
- Use the same source tab and frame checks for desktop review and the browser fallback.

## 4.48 / browser 0.68.12

### Keep one Chromium proxy configuration per browser batch

- Resolve unobserved links against one settings snapshot while keeping each route bound to its own URL.
- Stop native batch review if Chromium reports a proxy change during preparation or before dispatch. The user can collect the links again with the new configuration.

## 4.47 / browser 0.68.11

### Share cookie-store checks across browser batch review

- Read the tab and its cookie-store mapping once for signed-in batch preparation. Select cookies per file URL and verify the source and cookie permission before native dispatch.
- Route form requests to browser review before staging a body. Stop review if the captured session's source changes during preparation.

## 4.46 / browser 0.68.10

### Remove request encryption work from batch review startup

- Pass temporary request context to the preview workers in memory. Check headers, browser proxy settings and the website session in those workers before reading file details.
- Avoid encrypting and decrypting every preview on the dialog startup path. Saved downloads still use the existing protected storage path.

## 4.45 / browser 0.68.9

### Keep batch file details when one link changes

- Refresh only the edited browser row when its address or request credentials change. Keep the sizes, types, and active lookups for the other links.
- Cancel the superseded lookup and discard its pending result. Give the replacement priority within the existing four-worker limit.

## 4.44 / browser 0.68.8

### Use the File Properties layout for browser batch edits

- Show the file details, one Save To path, the original page link, and request fields in the compact File Properties layout.
- Preserve category and server defaults when their parts of the destination are left alone. Save an explicitly cleared description as blank.
- Keep browser credentials scoped to their original address. An address change to another origin drops untouched per-file overrides.

## 4.43 / browser 0.68.7

### Resize and maximize browser batch review

- Add the resizable border and minimize and maximize controls from the IDM browser batch dialog.
- Expand the file list with the window. Keep Save To at the bottom left and the filters and confirmation buttons at the bottom right.
- Keep the reference dialog dimensions as the minimum tracking size. Scale the resized window and its controls when moving between monitors.

## 4.42 / browser 0.68.6

### Match the browser batch review layout and queue workflow

- Use IDM's dialog-unit geometry for the browser batch list, Save To group, filters and buttons.
- On OK, add the checked links to the primary download queue without opening a second dialog. The Tasks batch workflow retains the configurable queue picker.

## 4.41 / browser 0.68.5

### Apply browser batch filters to the files queued

- Count and queue only checked rows visible in browser batch review. Showing a filtered row again restores its previous check state.
- Start with repeated links hidden. Disable the repeat filter when all addresses are unique. Recognize HTML content types regardless of case or MIME parameters.

## 4.40 / browser 0.68.4

### Position the selected-link strip at the end of the selection

- Show the strip after the mouse is released and anchor it near the selection endpoint for both drag directions. Hide it when that endpoint scrolls out of view.
- Match the full and compact strip heights from the IDM reference. Keep the one-click handoff to batch review for the selected HTTP links.

## 4.39 / browser 0.68.3

### Bring the browser video menu closer to IDM

- Keep the compact player strip to the download arrow and close button. Open its panel controls with a right-click or Shift+F10.
- Use the reference menu gutters and highlight colors for the expanded and compact format lists. Keep the menu within the viewport when space is limited.

## 4.38 / browser 0.68.2

### Keep hidden desktop history out of the browser handoff

- Defer history, category tree, sorting and row painting while the main window is hidden or minimized. Refresh as soon as the window is restored.
- Keep browser confirmations, clipboard monitoring, transfer events and download alerts active in the background.
- During modal confirmation, update file sizes only in the visible rows and release the catalog lock before painting. Handle durable browser offers without a full history scan for legacy confirmations.

## 4.37 / browser 0.68.1

### Cancel playlist discovery when the video menu closes

- Stop pending HLS and DASH checks when the user closes the menu or changes players. A superseded lookup releases its pending requests.
- Start a fresh lookup immediately on reopen. Cancel messages match the tab, frame, document and specific lookup.
- Preserve verified selections so choosing an early format still opens the desktop download flow.

## 4.36 / browser 0.68.0

### Show verified streaming qualities while other playlists load

- Publish confirmed HLS variants and DASH qualities during discovery. Keep the existing request and byte limits for the remaining playlists.
- Keep the same download keys when early choices remain available, and reject publication from a superseded lookup.
- Keep keyboard focus, pressed pointer selections and open audio options during menu updates.

## 4.35 / browser 0.67.99

### Keep confirmed video qualities when a playlist stalls

- Check independent HLS and DASH sources in parallel, with four shared read slots and one nine-second discovery deadline.
- Keep completed qualities available when another playlist or variant misses the deadline. Tell the user that additional formats may be available on refresh.
- Enforce the shared HLS request and byte limits while reading. Validate the player again before showing any offers.

## 4.34 / browser 0.67.98

### Show direct video files while stream formats load

- Make verified direct files selectable while the same player's HLS or DASH formats are being discovered.
- Preserve direct selections across the catalog update. Keep focus on the selected format and wait for a pressed pointer to release before replacing menu rows.
- Keep the direct choices usable after playlist lookup fails. Recheck the player, source list and settings before handing off a selection.

## 4.33 / browser 0.67.97

### Reduce the wait for formats on non-YouTube players

- Read tab state and capture settings concurrently, then ask the browser to inspect the clicked player immediately.
- Skip unrelated elements for direct players. Keep the visible-player ambiguity check for blob playback and Dailymotion.
- Reuse one capture snapshot when associating a playlist and checking its format details. Recheck the player and its document before publishing formats.

## 4.32 / browser 0.67.96

### Match the IDM browser panel and toolbar appearance

- Align the video strip to the player corner with space above the video when available.
- Give the compact strip a right-aligned, gray format menu with the player title, numbered entries and visible format details when titles are truncated.
- Use an original globe-and-arrow toolbar icon with a crossed-out disabled state. The toolbar still toggles integration for the active tab.

## 4.31 / browser 0.67.95

### Reduce browser work before the Windows batch opens

- Read tab, cookie policy, and desktop capability once when preparing a batch. Each link still retains its own request context, proxy route, and session verification.
- Recheck the source documents, cookie store, and permission before sending the batch to Windows.

## 4.30 / browser 0.67.94

### Reuse browser response details in batch review

- Fill in the file size, type, and filename from a matching, unambiguous browser response when all required details are available.
- Use the desktop metadata lookup for missing or changed request details. The transfer still verifies its own response.

## 4.29 / browser 0.67.93

### Match browser batch file details and apply the previewed filename

- Show Windows file icons and type names from the background Shell worker. Format known sizes in bytes, KB, MB, or larger units while keeping numeric sorting.
- Use the displayed filename to preview the category folder and to add the file. Server-suggested names carry through confirmation, and per-file name edits take precedence.

## 4.28 / browser 0.67.92

### Match the browser batch destination controls

- Use a Save To group with three radio buttons. Choose each file's category folder, one category for all files, or one directory for all files.
- Show more rows in the batch list and arrange the selection buttons and filters beside the destination group. The displayed save paths follow the active destination choice.

## 4.27 / browser 0.67.91

### Review right-click link batches in Windows

- Both right-click batch commands open the Windows review when the desktop supports this workflow. UDM checks the contributing browser documents before opening it.
- The browser batch list follows IDM's column order, shows the planned save path for each file, and hides images found on the page by default.

## 4.26 / browser 0.67.90

### Open selected browser links in the Windows batch dialog

- The **Download with UDM** strip opens the native batch review over the browser. Links start unchecked and no download is added before confirmation.
- The dialog shows link text and checks file details using the browser request context. Older desktop versions and batches that require the browser review still use the extension tab.

## 4.25 / browser 0.67.89

### Bring the browser download strips closer to IDM

- Selected links show a classic **Download with UDM** strip. One click opens the existing batch review with the selected links checked.
- Compact video and selected-link strips keep the close button visible. Compact video menus align with the right edge of the strip and use denser format rows.

## 4.24 / browser 0.67.88

### Start the desktop handshake as the video menu opens

- Prepare the native connection after the browser finds available formats, so the handshake can run while you choose a quality. Selecting a format remains the step that submits the download.
- Browser Tools uses the original video tab and can open desktop settings, restore panels, and recover interrupted downloads from its own tab.

## 4.23 / browser 0.67.87

### Close the quality menu when handing a video to UDM

- Selecting a single video or audio format dismisses the menu immediately. Download File Info handles ordinary successful handoffs.
- A failed or uncertain handoff reports the outcome without retrying it or overriding a later dismissal. Bulk selections retain their progress and partial-failure counts.

## 4.22 / browser 0.67.86

### Match the IDM browser panel and toolbar interaction

- Use a slim player strip with a green play glyph and a compact, numbered format menu. Open secondary panel controls with the question-mark button.
- Click the toolbar icon to toggle UDM on the current tab. Right-click and choose UDM browser tools for settings, Site Grabber, link discovery and recovery.

## 4.21 / browser 0.67.85

### Refresh a failed video session from Download File Info

- Recover a matching browser capture from the open File Info dialog after background prefetch fails. Save the session, then let UDM retry prefetch on the same download while retaining the selected destination.
- The refresh dialog opened from File Info only saves the session. Start Download, Download Later and Cancel remain available in File Info.

## 4.20 / browser 0.67.84

### Show progress while saving a prepared video

- After Start, show the verified file size and publication status through the usual progress window while saving a prepared file.
- Honor the keep-open setting even when publication finishes before the window appears. The retained window shows the measured prefetch peak.

## 4.19 / browser 0.67.83

### Capture prefetch speed between progress updates

- Measure the prefetch peak with a clock and byte counter for each download. Take a final sample at Start or when the transfer phase ends, so a short prefetch can register even before the next desktop tick.
- Keep the prefetch peak separate from the current transfer rate and ETA.

## 4.18 / browser 0.67.82

### Start background downloads when a slot frees up

- Retry eligible direct-file prefetch while Download File Info is open if all transfer slots were busy. A metadata lookup can show the size while the file waits.
- Suspend retries while login edits or a nested dialog are active. Start and Download Later remain available while waiting.

## 4.17 / browser 0.67.81

### Explain saved bytes in the progress details

- Show a separate saved-data row when a completed or verified prefetched file has no measured connection traffic.
- Keep the original number and byte count for connections that transferred data. Hide unused rows after completion.

## 4.16 / browser 0.67.80

### Show the measured prefetch peak after Start

- Record the highest sampled transfer rate while File Info prefetch is active and save it with the bytes already received at Start.
- Display that peak separately from the current transfer rate. Keep the limiter and estimated time based on the live rate.

## 4.15 / browser 0.67.79

### Keep a verified video when closing Download File Info

- Closing File Info saves an already verified media assembly with the paused download. Resume can verify and reuse it.
- Save the assembly record before stopping the worker. A failed save leaves the dialog available to retry. Explicit Cancel on a fresh offer still discards its temporary output.

## 4.14 / browser 0.67.78

### Show the full byte count in compact Download File Info

- Keep a readable size beside the file icon and show the full byte count in a visible row above the dialog buttons.
- Distinguish verified final output, calculated output, retained output, selected tracks and browser-reported sizes. Keep preparation and capacity messages visible when the byte count is pending.

## 4.13 / browser 0.67.77

### Configure new categories from Download File Info

- Open the shared category editor from the plus button. Set a folder, file types, site restrictions and the remember-path preference while creating the category.
- Select the new category after saving and update the proposed destination through the existing category selection workflow.

## 4.12 / browser 0.67.76

### Recover the saved YouTube streaming format from the browser

- Opening the video from Refresh media session identifies the paused download's streaming format to the extension. The recovery page offers that format even when the normal panel prefers a direct link at the same resolution.
- Route the replacement to the open recovery dialog and check the original track identities before saving. An expired recovery page cannot create a separate download.

## 4.11 / browser 0.67.75

### Show progress while checking saved YouTube streams

- Use the existing saved-data verification display while hashing retained YouTube segments or completed tracks.
- Check for Pause between disk reads. An interrupted segment check preserves its prior cache index for the next Resume.

## 4.10 / browser 0.67.74

### Reclaim YouTube streaming files after Cancel

- After cancellation is saved, remove known streaming segments, cache receipts and internal track files from the canceled offer's temporary folder.
- Preserve shared folders, redirected paths and unrelated files.

## 4.9 / browser 0.67.73

### Keep a prepared YouTube file after refreshing its session

- Preserve the assembled file's receipt when the approved replacement session matches the saved stream selection and the receipt belongs to the current download.
- Save the refreshed session and updated receipt together. Resume verifies the existing output before publication.

## 4.8 / browser 0.67.72

### Rebuild fully saved YouTube streams after the session expires

- Resume can rebuild video and audio tracks from a complete verified segment cache after an interruption during assembly.
- Check playback session freshness and establish the network connection only when a track still needs media bytes. A missing or damaged segment still requires a fresh session.

## 4.7 / browser 0.67.71

### Keep the YouTube playback session valid through handoff

- Apply the same document, visit and expiry checks to captured sessions for video and audio.
- Recheck the prepared playback session before native dispatch, including reconnects. Ask for refreshed playback if the session expires while UDM connects.

## 4.6 / browser 0.67.70

### Keep the selected YouTube transport current through connector startup

- Recheck direct video links and streaming audio/video revisions and duration immediately before native dispatch, including reconnects.
- Revalidate native-retrieved pairs for freshness and the chosen quality; apply the current panel size setting to the prepared transfer.
- Reject an outdated selection before creating a desktop download.

## 4.5 / browser 0.67.69

### Account for earlier GUI improvements that reused version numbers

- Restore 39 separately documented functional deliveries to the cumulative count, including category editing, queue membership, menus, columns, DPI scaling and recovery controls.
- Update historical accounting labels and preserve recorded artifact versions. Exclude verification-only continuations and the previously counted Properties build.

## 3.65 / browser 0.67.68 (recorded 0.67.29)

### Check the selected video after connecting to UDM

- Run the final video, page, and permission checks after the native connection is ready. Recheck if the connection drops before the request can be sent.
- Keep the preparation deadline active through validation. A changed selection or expired request sends nothing to the desktop.

## 3.64 / browser 0.67.67 (recorded 0.67.28)

### Prevent late video submission during native connection waits

- Check the selection deadline at the actual native send, including after a reconnect. An expired selection leaves the browser without submitting a download.
- Start the acknowledgement timer at dispatch. Discard a draft recovery receipt if dispatch never occurs. Connection startup still overlaps video preparation.

## 3.63 / browser 0.67.66 (recorded 0.67.27)

### Share the desktop capability read for direct YouTube audio

- Reuse a fresh desktop reply for both request preparation and handoff receipts. Concurrent consumers share a refresh when the earlier read expires.
- Keep browser proxy and session validation and the final selected-audio check before submission.

## 3.62 / browser 0.67.65 (recorded 0.67.26)

### Read desktop capabilities during selected media preparation

- After validating the selected YouTube format or adaptive site offer, read desktop capabilities while preparing the streams. A cold desktop can start in the background during that wait.
- Scope the result to the selection and refresh it if it is stale. A download still requires its receipt and final page and permission checks.

## 3.61 / browser 0.67.64 (recorded 0.67.25)

### Reuse host capabilities on the negotiated connection

- Reuse the native hello reply for capability checks on the same connection, including YouTube audio, HLS and DASH handoffs.
- Clear cached capabilities when the port closes. Legacy hosts and desktop settings retain their existing request paths.

## 3.60 / browser 0.67.63 (recorded 0.67.24)

### Connect to the native host while preparing a selected video

- Overlap native protocol negotiation with YouTube quality preparation and supported site playlist or direct media preparation.
- Reuse the negotiated connection for the eventual handoff. The handshake creates no download, and the final selection and receipt checks still gate submission.

## 3.59 / browser 0.67.62 (recorded 0.67.23)

### Open video quality choices without waiting for Site Grabber

- Load YouTube qualities as soon as the popup identifies the active video. Site Grabber and integration settings load independently.
- Use a dedicated status line for popup video actions. The popup and the page panel only show a preparation message if the handoff is still pending after a short delay. Keep quality selection locked during handoff.

## 3.58 / browser 0.67.61 (recorded 0.67.22)

### Reclaim canceled HLS and DASH segment caches

- After a fresh recorded HLS or DASH offer is canceled, remove the known segment files and recovery receipts from its verified temporary folder.
- Leave shared folders, redirected paths, live recording data and other assembly files in place.

## 3.57 / browser 0.67.60 (recorded 0.67.21)

### Reclaim canceled browser prefetch ranges

- After saving the cancellation of a fresh browser offer, remove its known HTTP range files, including ranges for separate audio and video tracks.
- Preserve shared or redirected folders and files that cannot be safely opened. Closing File Info still preserves the paused download.

## 3.56 / browser 0.67.59 (recorded 0.67.20)

### Reject a fresh browser offer from File Info

- Cancel removes a fresh unconfirmed browser offer from the download list after its prefetch stops. Closing the window keeps the paused record available.
- Existing downloads and original browser responses use the existing pause behavior. A failed removal keeps the dialog available for retry.

## 3.55 / browser 0.67.58 (recorded 0.67.19)

### Add downloads to a running queue without restarting it

- When Start queue processing is selected in Download Later or batch import, add the selected eligible downloads to the current run. Other queue members keep their retry counts, and the schedule and completion cycle keep running.
- If the queue is idle, start it normally. A deferred prefetch still releases its worker slot before the queue resumes the file.

## 3.54 / browser 0.67.57 (recorded 0.67.18)

### Prevent repeated desktop Start queue commands

- Dim Start for a running queue in the Downloads, tray and toolbar menus. Recheck the queue state when the command runs, including from the large-catalog chooser.
- Use Scheduler's existing run-state check for the primary toolbar Start command. Repeated clicks leave the current queue cycle and retry counts intact.

## 3.53 / browser 0.67.56 (recorded 0.67.17)

### Save the selected batch and its queue together

- Commit the selected downloads, any new queue and the batch prompt preference together. A failed admission leaves the selection available for retry.
- Keep the existing duplicate policy and retain old partial files until the batch commits. Report optional queue-start failures separately from the saved batch.

## 3.52 / browser 0.67.55 (recorded 0.67.16)

### Create the selected queue with Download Later

- Save a queue created in the Download Later chooser together with its first download and the chooser preference. Invalid file details or a failed catalog save leave the queue list intact.
- Optional queue processing starts after the download and its queue are saved.

## 3.51 / browser 0.67.54 (recorded 0.67.15)

### Save the Download Later queue prompt preference with the download

- Match the reference queue chooser's “Don't ask me again” caption. Save the single-download preference with Download Later whether prefetch is running or stopped.
- If the optional queue start fails after the download is saved, report that it was added and close File Info after the message.

## 3.50 / browser 0.67.53 (recorded 0.67.14)

### Match File Info category names and destination choices

- Show General and Compressed in Download File Info and name the selected category in the remember-folder checkbox. Keep saved category identifiers and custom names distinct.
- Use the same category folder rules as Options and new download admission when the user changes the category. The destination now follows the configured or default folder for that category.

## 3.49 / browser 0.67.52 (recorded 0.67.13)

### Queue interrupted browser responses with the edited request

- Verify the saved response and prepare its retained bytes for range continuation before committing Start. File Info saves the new destination, login and folder preferences with the queued continuation.
- Resume uses the same admission. On failure the captured body and receipt stay recoverable, and a verified range seed can be reused on retry.

## 3.48 / browser 0.67.51 (recorded 0.67.12)

### Save the Start decision for complete browser responses

- When File Info starts saving a complete response already received from the browser, save the edited destination and preferences with the worker admission. A failed catalog save leaves the previous record and pause state available for retry.
- Publication still checks the saved response and its receipt before writing the final file. Interrupted responses continue through range recovery.

## 3.47 / browser 0.67.50 (recorded 0.67.11)

### Save Start Download with the edited request

- After prefetch stops, save the edited request, remembered preferences and individual start decision together. A failed queue admission or catalog write leaves the dialog available for retry.
- Original browser responses retain their dedicated publication or continuation workflow.

## 3.46 / browser 0.67.49 (recorded 0.67.10)

### Save File Info edits together when prefetch has stopped

- Save the destination, description, folder preferences and per-file or remembered login together. If validation or catalog storage fails, the dialog keeps the edits available to retry.
- Download Later saves the selected queue and pause decision with those edits. Start schedules the transfer after the edited request is saved.

## 3.45 / browser 0.67.48 (recorded 0.67.9)

### Count older builds that reused version numbers

- Recover 42 additional increments from delivered 0.79–0.82 installers and independently built 0.84 desktop and browser candidates. Carry those increments into all later historical browser numbers and into the current desktop and browser versions.
- Give the recovered 0.84 builds their own desktop numbers in the historical ledger. Keep the original numbers with the source tags, receipts and artifacts.

## 3.2 / browser 0.67.47 (recorded 0.66.66)

### Remember an unchanged login without restarting the prefetch

- Save the site login with the Start or Download Later decision when it matches the current request. Start continues the same worker, and Download Later keeps its saved data.
- If the login differs, apply it through the existing request restart path. A failed confirmation save restores the previous site settings.

## 3.1 / browser 0.67.46 (recorded 0.66.65)

### Refresh File Info for edited login details

- Hide the old size and server filename when the login fields change. Cancel any details lookup started with the previous values.
- Refresh details can check the edited login without changing the saved request or writing its session cookies into the download record. Saving a different login clears the old browser size and filename.

## 3.0 / browser 0.67.45 (recorded 0.66.64)

### Show the browser-observed server filename in Download File Info

- Offer the filename from a matching browser response while the desktop waits for its own response or metadata check. Use server name applies it only when selected.
- Prefer the desktop's own response once available. Clear the browser filename when the address or request headers change.

## 2.99 / browser 0.67.44 (recorded 0.66.63)

### Show the reported size for ordinary browser downloads

- Use a matching browser GET response to show the reported byte count when File Info opens. For a valid partial response, use the total from Content-Range.
- Skip conflicting lengths, encoded responses and responses that no longer match the request being handed off. The native response still determines the transfer size.

## 2.98 / browser 0.67.43 (recorded 0.66.62)

### Show the browser-reported size of direct YouTube M4A downloads

- Show the reported byte count as soon as File Info opens when the selected direct audio track provides one. If the signed link and track metadata disagree, wait for the server response.
- Replace the reported size with the response size when available. An address or request-header edit clears the hint.

## 2.97 / browser 0.67.42 (recorded 0.66.61)

### Recover interrupted direct YouTube M4A handoffs

- Give direct YouTube M4A downloads a durable receipt while preserving their ordinary file-transfer and duplicate handling. Check the current audio URL and cookie permissions immediately before sending.
- If the desktop reply is lost, check the original selection's receipt before sending another request. Older desktop versions still accept direct audio through the ordinary handoff.

## 2.96 / browser 0.67.41 (recorded 0.66.60)

### Recover an earlier YouTube handoff before preparing playback

- Check the saved receipt for the selected quality or audio track before reading playback URLs. An accepted handoff can be recovered even if its old playback URL has expired.
- Show the recovered result in the extension popup and acknowledge it after displaying the result.

## 2.95 / browser 0.67.40 (recorded 0.66.59)

### Recover interrupted YouTube handoffs

- Record the selected YouTube video or captured streaming audio with a durable media receipt. If the reply is lost, check whether UDM already accepted the selection before trying again.
- Confirm the current video visit and selection after preparing the receipt. Direct M4A links retain the ordinary file handoff.

## 2.94 / browser 0.67.39 (recorded 0.66.58)

### Recheck a YouTube quality as soon as its stream becomes ready

- While a selection is waiting for playback capture, the panel can wake the next browser check as soon as a verified stream becomes available. The 400 ms polling fallback still covers missed notifications.
- Confirm the selected video, visit and track before submitting. A readiness notification only requests another check.

## 2.93 / browser 0.67.38 (recorded 0.66.57)

### Read ready YouTube streaming sessions with the selected formats

- Read a ready streaming session in the same browser call as the format metadata when a video or audio track is selected. Keep the separate lookup for sessions that arrive later.
- Check that the selection still belongs to the same visit before handoff. Native player results cached for an earlier visit are excluded even if the video ID matches.

## 2.92 / browser 0.67.37 (recorded 0.66.56)

### Keep YouTube stream verification tied to the current visit

- Clear verified streaming evidence when YouTube starts a new navigation or the page video changes. A response that arrives after its visit ended cannot make the panel ready, even after returning to the same video.
- Keep the early handoff path for a stream whose response verifies the current video's identity while an ad is still marked as playing.

## 2.91 / browser 0.67.36 (recorded 0.66.55)

### Show progress while checking a saved HTTP prefetch

- Show the saved-data check separately from download completion when resuming a fully prefetched file. Report progress for both consolidated files and older numbered parts.
- Use the same progress display for FTP replacement checks. Keep Pause available and suppress the transfer rate and ETA during the check.

## 2.90 / browser 0.67.35 (recorded 0.66.54)

### Show progress while checking a replacement FTP address

- Display the percentage of retained bytes verified in the progress dialog. Keep the saved download percentage separate and preserve the Pause action.
- Count verification traffic for quota and diagnostics. Hide the download rate and ETA during the check and start the rate meter from the updated byte count when transfer resumes.

## 2.89 / browser 0.67.34 (recorded 0.66.53)

### Apply browser routing and cookies to the correct replacement address

- Replace a captured proxy route when the browser supplies one for the new address. If the previous route was tied to a different URL, prompt for a new browser capture before using a browser route again.
- Discard the saved browser session on cross-origin address changes. Keep manually entered FTP replacement links separate from browser handoff and review.

## 2.88 / browser 0.67.33 (recorded 0.66.52)

### Refresh a paused FTP download address

- Accept a replacement FTP link in Refresh download address. For a different source, check the server's size and modification time, then compare every retained byte before binding the partial download to the new link.
- Preserve saved parts on mismatch or interruption. Keep strict FTP validation through the final metadata and range boundary checks.

## 2.87 / browser 0.67.32 (recorded 0.66.51)

### Preserve partial bytes while validating a replacement HTTP link

- Keep refresh validation pending through the range requests and their overlap checks. Pause, cancellation and failed requests keep that protection for the next attempt.
- For a refreshed link with saved bytes, check the worker responses against the same validator used by the probe. If a worker sees a changed file or the server stops honoring ranges, keep the partial download available for another link.

## 2.86 / browser 0.67.31 (recorded 0.66.50)

### Repair every damaged range in a saved prefetch

- Check the entire saved plan on Resume. Keep verified ranges and reset every missing, shortened or changed range before fetching again.
- Save the repaired offsets before retrying so an interruption cannot restore the old completed offsets. Remove damaged numbered parts and prevent leftover numbered parts from replacing bytes already verified in the consolidated file.

## 2.85 / browser 0.67.30 (recorded 0.66.49)

### Reuse the verified checksum of a fully prefetched file

- On Resume, use the checksum calculated while verifying a saved single-range receipt for final publication when the file identity and size still match.
- Keep the existing hashing path for files with multiple ranges and for output that is changed during assembly.

## 2.84 / browser 0.67.29 (recorded 0.66.48)

### Separate direct-download work from time spent waiting for Start

- End network timing before creating the saved prefetch receipt. Record receipt work and confirmation wait separately in the finish diagnostics.
- Exclude confirmation wait from active elapsed time, including when the download is deferred or canceled, and from the total time spent finishing the file.

## 2.83 / browser 0.67.28 (recorded 0.66.47)

### Publish sooner when Start arrives during direct-download prefetch verification

- Stop calculating the optional saved prefetch receipt after Start is committed. Continue with normal publication and reuse any running hash progress.
- Keep independent verification for resumed receipts and preserve cancellation and required checksum checks.

## 2.82 / browser 0.67.27 (recorded 0.66.46)

- Account for four additional build increments. Restore the native 0.28.0 offline-site/SOCKS5 build and the 0.75.0 automation candidate to the paired ledger. Split the durable Pause build and the later context-menu rebuild out of the old 2.05 heading.
- Carry native 1.100.0 through 1.105.0 into 2.00 through 2.05. The first build recorded as 2.05 becomes 2.06, and the later 2.05 context-menu build takes the gap after the dark-menu build in chronological order. The previous development build, recorded as 2.80 / 0.66.41, accounts as 2.81 / 0.66.45.
- List previously implicit browser-only milestones within their existing numeric jumps. Archived tags, packages and build receipts retain their recorded numbers. [Counting policy](docs/versioning.md)

## 2.81 / browser 0.67.26 (recorded 0.66.41) (recorded desktop 2.80)

### Reuse the running hash for a single-range prefetch
- Finish the hash already trailing a single-range HTTP transfer when creating its completed prefetch receipt. Publication reuses the finished digest.
- Resume continues to verify saved prefetch bytes against their receipt before reusing them.

## 2.80 / browser 0.67.25 (recorded 0.66.40) (recorded desktop 2.79)

### Reuse empty HTTP prefetches after Download Later
- Record an empty response with the same range plan whether its length was declared or measured at EOF.
- Require the saved zero-byte file before reusing an empty prefetch receipt.

## 2.79 / browser 0.67.24 (recorded 0.66.39) (recorded desktop 2.78)

### Show the measured size of a prefetched response with no length header
- Record the received byte count as the response size after a successful end of stream. File Info can display that count before Start, and a completed nonempty prefetch can use the existing verified receipt.

## 2.78 / browser 0.67.23 (recorded 0.66.38) (recorded desktop 2.77)

### Reuse the source inspection for a preserved MP4
- Reuse the container and track metadata read during MP4 selection. Independently inspect the staged output and verify its hash before reporting the exact size.

## 2.77 / browser 0.67.22 (recorded 0.66.37) (recorded desktop 2.76)

### Stage verified combined MP4s with less temporary storage
- Use a hard link for eligible captures when the filesystem allows it, with a verified copy as the fallback.
- Unlink a leftover staging name before preparing a replacement so a retry cannot overwrite the source through that name.

## 2.76 / browser 0.67.21 (recorded 0.66.36) (recorded desktop 2.75)

### Preserve eligible combined YouTube MP4 files
- Stage a supported MP4 with one video track and one audio track byte for byte. Verify its saved hash and selected tracks before showing its exact size or publishing it.
- Keep the media assembly path for separate tracks and other containers.

## 2.75 / browser 0.67.20 (recorded 0.66.35) (recorded desktop 2.74)

### Preserve browser request context for direct YouTube audio
- Prepare direct M4A downloads through the existing browser handoff path, including the proxy route and permitted request credentials.
- Recheck the selected track and its direct URL after preparing that context and before submitting it to UDM.

## 2.74 / browser 0.67.19 (recorded 0.66.34) (recorded desktop 2.73)

### Bound YouTube video and audio preparation
- Apply a 20-second preparation deadline before submitting a YouTube download. Late player or stream data cannot submit a download after that deadline.
- Keep a separate 40-second acknowledgement window after submission.

## 2.73 / browser 0.67.18 (recorded 0.66.33) (recorded desktop 2.72)

### Recheck direct video selections after preparing browser credentials
- Apply the same final selection check to direct video and adaptive downloads. Abort before submission if the video, panel settings or required cookie permissions changed.

## 2.72 / browser 0.67.17 (recorded 0.66.32) (recorded desktop 2.71)

### Recheck the video immediately before adaptive handoff
- Validate the selected video, panel settings and cookie permission after credential collection and receipt preparation. A changed selection stops before the download request is sent.

## 2.71 / browser 0.67.16 (recorded 0.66.31) (recorded desktop 2.70)

### Reuse the desktop capability check during live DASH handoff
- Skip the redundant desktop capability request when the same selection already checked live DASH Period support. Still revalidate the active Period before submitting the download.

## 2.70 / browser 0.67.15 (recorded 0.66.30) (recorded desktop 2.69)

- Restore 89 previously uncounted increments using paired changelog entries, archived tag manifests, and development records. That audit mapped the 2.68 build to 0.66.29 and used 0.66.30 for this development build.
- Keep recorded version jumps and assign one increment to each documented update that reused a browser number. Count earlier reconciliation builds once each. Unversioned changes do not add increments without evidence of a separate build.

## 2.69 / browser 0.67.14 (recorded 0.65.40) (recorded desktop 2.68)

### Fetch HLS subtitles alongside the selected streams
- Fetch a selected subtitle playlist concurrently with the video and audio playlists. Validate it against the resulting live or recorded selection before handing the download to UDM.

## 2.68 / browser 0.67.13 (recorded 0.65.39) (recorded desktop 2.67)

### Prepare HLS video and audio together
- Fetch the selected video and separate audio playlists in parallel before handing the download to the desktop.
- Preserve track order, the capture deadline, and the audio-only path that skips the video playlist.

## 2.67 / browser 0.67.12 (recorded 0.65.38) (recorded desktop 2.66)

- Preserve all 236 increments already represented by 0.63.236. Under the two-digit rule that version maps to 0.65.36. Count the 2.65 rollover build and this correction as two further increments.

## 2.66 / browser 0.67.11 (recorded 0.64.0) (recorded desktop 2.65)

- Roll both browser manifests forward to 0.64.0 and use at most two digits per version component for future desktop and browser increments.

## 2.65 / browser 0.67.10 (recorded 0.63.236) (recorded desktop 2.64)

### Recover a browser connection before sending a download
- Reconnect once if the native connection closes before the download request is sent.
- Send the request through the port that completed the handshake. A disconnect after submission preserves the existing duplicate-prevention behavior.

## 2.64 / browser 0.67.9 (recorded 0.63.235) (recorded desktop 2.63)

### Keep automatic capture prompts accessible
- Give browser duplicate and capture-exception prompts the same foreground request and taskbar entry as File Info.

## 2.63 / browser 0.67.8 (recorded 0.63.234) (recorded desktop 2.62)

### Keep browser-triggered address and media refresh dialogs separate
- Open replacement-link confirmations with the same hidden owner and taskbar behavior as browser File Info.
- Keep the desktop-initiated refresh workflow available from the download list.

## 2.62 / browser 0.67.7 (recorded 0.63.233) (recorded desktop 2.61)

### Prioritize pending browser confirmations
- Present saved browser captures before refreshing or sorting the download list.
- Schedule another capture pass when a browser notification arrives while an earlier dialog is open.

## 2.61 / browser 0.67.6 (recorded 0.63.232) (recorded desktop 2.60)

### Start media prefetch when a transfer slot frees up
- If all transfer slots are occupied, keep checking for capacity while File Info is open and prefetch as soon as a slot is available.
- Show Waiting or Preparing when the media size is still unknown. Keep Start Download available throughout.

## 2.60 / browser 0.67.5 (recorded 0.63.231) (recorded desktop 2.59)

### Label media track sizes while the final file is being prepared
- Show known input track totals as Selected track size on the first paint and during prefetch.
- Use the browser-reported track size until the individual lengths are known. A verified or calculated output size takes priority when available.

## 2.59 / browser 0.67.4 (recorded 0.63.230) (recorded desktop 2.58)

### Show exact byte counts for direct downloads in File Info
- Show a known direct response size in bytes on the first paint or when metadata arrives.
- Preserve a known size when a later metadata lookup returns no length. Sizes awaiting refresh validation keep the existing display.

## 2.58 / browser 0.67.3 (recorded 0.63.229) (recorded desktop 2.57)

### Show the current media progress when Start is clicked
- Update the received and transferred totals from the audio and video tracks when confirming prefetch, before opening progress.
- Keep the prefetched byte count consistent with the first progress update and continue showing the sampled transfer rate.

## 2.57 / browser 0.67.2 (recorded 0.63.228) (recorded desktop 2.56)

### Close Download File Info promptly after Download Later
- Save the destination and queue choice, then close File Info while the prefetch worker stops.
- Keep a queue start requested during shutdown pending until the prefetch worker releases the transfer slot.

## 2.56 / browser 0.67.1 (recorded 0.63.227) (recorded desktop 2.55)

### Settle cancelled prefetch after the worker exits
- Wait for the background worker's exit signal before clearing confirmation and the provisional final size. Paused media now records a matching paused phase.
- Keep the pending confirmation state available for retry if saving the cancellation fails. Verified partial data remains available to resume.

## 2.55 / browser 0.67.0 (recorded 0.63.226) (recorded desktop 2.54)

### Recover file details after background download failure
- Download File Info reports a failed background transfer while leaving Start Download available.
- If an eligible direct download still has no known size, the dialog tries a metadata lookup once. The lookup does not replace the size of a saved partial transfer.

## 2.54 / browser 0.66.99 (recorded 0.63.225) (recorded desktop 2.53)

### Repair an isolated FTP resume mismatch
- When the remote file metadata still matches and other ranges have saved bytes, retry the affected range from its beginning once.
- Keep the saved parts for manual redownload if repair is ineligible or the mismatch repeats.

## 2.53 / browser 0.66.98 (recorded 0.63.224) (recorded desktop 2.52)

### Check the boundaries between FTP ranges
- Compare up to 64 overlapping bytes before publishing a segmented FTP download. Keep the parts available if the overlap differs.

## 2.52 / browser 0.66.97 (recorded 0.63.223) (recorded desktop 2.51)

### Advance the desktop and browser versions together
- Bring the Chromium and Firefox manifests forward by the 138 development increments recorded since browser version 0.63.85. This is a paired version update.

## 2.51 / browser 0.66.96 (recorded 0.63.85) (recorded desktop 2.50)

### Repair a single mismatched HTTP resume range
- If the server still identifies the same file and other ranges have saved progress, retry the affected range from its beginning.
- Keep the single-connection fallback for mismatches that cannot be isolated or persist after repair.

## 2.50 / browser 0.66.95 (recorded 0.63.85) (recorded desktop 2.49)

### Check the boundaries between HTTP ranges
- New nonzero ranges request up to 64 bytes of overlap. Save the overlap hash with the range and compare it against the adjoining bytes before publishing.
- Apply the existing single-connection recovery when an ordinary download has inconsistent boundaries.

## 2.49 / browser 0.66.94 (recorded 0.63.85) (recorded desktop 2.48)

### Verify HTTP resume bytes before appending
- Compare up to 64 bytes of overlap before writing more data to a resumed HTTP range.
- Restart ordinary transfers on one connection if the overlap differs. Preserve partials when a refreshed address must match the saved file.

## 2.48 / browser 0.66.93 (recorded 0.63.85) (recorded desktop 2.47)

### Verify FTP resume bytes before appending
- Request up to 64 bytes before a saved FTP offset and compare them with the retained part.
- Preserve the partial files and direct the user to Redownload if the overlap differs.

## 2.47 / browser 0.66.92 (recorded 0.63.85) (recorded desktop 2.46)

### Recover from an unreachable FTP passive port
- Try PASV if the EPSV data connection fails and the route can use PASV.
- Keep the fallback on the existing FTP route and perform it before requesting file data.

## 2.46 / browser 0.66.91 (recorded 0.63.85) (recorded desktop 2.45)

### Assemble FTP downloads from their own partial files
- Use the numbered FTP parts for content checks and publication even if a previous HTTP partial file exists in the same folder.

## 2.45 / browser 0.66.90 (recorded 0.63.85) (recorded desktop 2.44)

### Save HTTP range resets before removing obsolete data
- Preserve old partials if the zero-progress reset cannot be saved.
- Recover interrupted cleanup before any worker can adopt stale partials.

## 2.44 / browser 0.66.89 (recorded 0.63.85) (recorded desktop 2.43)

### Preserve old partial files until migration is saved
- Flush the consolidated partial file and save its resume offsets before deleting legacy parts.
- Complete legacy cleanup before workers start writing new data.

## 2.43 / browser 0.66.88 (recorded 0.63.85) (recorded desktop 2.42)

### Recover missing data in sparse HTTP partial files
- On resume, roll a saved range back to the first unallocated region within its recorded progress.
- Keep length-based recovery when the file system cannot report allocation.

## 2.42 / browser 0.66.87 (recorded 0.63.85) (recorded desktop 2.41)

### Checkpoint downloads independently of the desktop timer
- Run catalog checkpoints on an owned background timer so a stalled desktop thread does not stop resume-offset saves.
- Keep unchanged catalogs write-free and join the checkpoint worker before the final shutdown save.

## 2.41 / browser 0.66.86 (recorded 0.63.85) (recorded desktop 2.40)

### Finish HTTP responses cleanly for connection reuse
- Skip speculative reads beyond a worker's assigned range.
- Observe EOF for fully consumed responses before releasing them to the connection pool.

## 2.40 / browser 0.66.85 (recorded 0.63.85) (recorded desktop 2.39)

### Start downloading before an unadvertised range is verified
- On fresh HTTP transfers with a usable validator, write the full response while the midpoint helper checks range support, even if the server omits the range header.
- Require a verified range response before enabling resume or splitting in that case.

## 2.39 / browser 0.66.84 (recorded 0.63.85) (recorded desktop 2.38)

### Keep parallel HTTP available when the first helper arrives late
- If a validated midpoint response overlaps the primary worker's progress, discard its body and split the live remainder when it is still large enough.

## 2.38 / browser 0.66.83 (recorded 0.63.85) (recorded desktop 2.37)

### Keep the first HTTP response moving during helper startup
- For servers advertising byte ranges with a usable validator, read the accepted full response while the midpoint helper connects.
- Give the validated helper its range only if no committed or in-flight write overlaps it. If the full response finishes first, cancel the pending helper.

## 2.37 / browser 0.66.82 (recorded 0.63.85) (recorded desktop 2.36)

### Split fresh HTTP downloads as workers join
- Start with the accepted full response and a validated midpoint range. Additional workers obtain ranges by splitting the current largest eligible span.
- Reuse the validated helper response if another worker shortens its assigned range before it starts reading.

## 2.36 / browser 0.66.81 (recorded 0.63.85) (recorded desktop 2.35)

### Split the largest eligible HTTP range
- Rank candidates by bytes remaining and split the largest eligible span in half after excluding the current disk write.

## 2.35 / browser 0.66.80 (recorded 0.63.85) (recorded desktop 2.34)

### Prefer the next unstarted HTTP range
- When a worker finishes its range, give it the immediately following range if that range is unassigned and has received no bytes. This takes priority over splitting another worker's range.

## 2.34 / browser 0.66.79 (recorded 0.63.85) (recorded desktop 2.33)

### Honor the progress window start view
- Apply the selected start view to automatic browser-video progress windows and leave existing progress windows in their current state when a transfer resumes.
- Start queued progress in the system tray by default. Use a minimized taskbar window if Windows cannot register a tray icon.

## 2.33 / browser 0.66.78 (recorded 0.63.85) (recorded desktop 2.32)

### Match the Options on completion controls
- Disable modem, exit and power choices while the completion dialog is enabled.
- Clear those choices when showing the dialog again and lock completion controls once the download finishes.

## 2.32 / browser 0.66.77 (recorded 0.63.85) (recorded desktop 2.31)

### Match the progress Speed Limiter controls
- Use a right-aligned numeric rate field and disable its labels while the limiter is off.
- Skip catalog saves when the remembered rate is already set to the same value.

## 2.31 / browser 0.66.76 (recorded 0.63.85) (recorded desktop 2.30)

### Match the download connection table
- Show N., Downloaded, and Info columns with zero-based connection numbers.
- Use grouped whole-KB values for transfers of at least 1 KB and match the captured connecting and receiving captions.

## 2.30 / browser 0.66.75 (recorded 0.63.85) (recorded desktop 2.29)

### Match the download segment chart
- Show downloaded coverage in blue and connection starts in magenta over a white track, with Windows colors in high-contrast mode.
- Place HTTP and FTP markers at the latest accepted request offset, including offsets used on resume.

## 2.29 / browser 0.66.74 (recorded 0.63.85) (recorded desktop 2.28)

### Distinguish resuming from starting in the progress window
- Show Resume when a paused or failed download has saved progress and reports resume support.
- Show a disabled Pausing… action until the stop finishes.

## 2.28 / browser 0.66.73 (recorded 0.63.85) (recorded desktop 2.27)

### Reduce unnecessary progress-window redraws
- Update the overall progress bar and segment chart only when their values change.
- Update changed connection rows and skip the details panel while it is collapsed. Refresh the panel as soon as it expands.

## 2.27 / browser 0.66.72 (recorded 0.63.85) (recorded desktop 2.26)

### Restart parallel HTTP downloads safely
- Reuse one eligible full response when a parallel range request requires a restart. Stop the previous workers before writing from byte zero.
- Save the reset range map before recreating the partial file so recovery cannot use old byte offsets for the new file.

## 2.26 / browser 0.66.71 (recorded 0.63.85) (recorded desktop 2.25)

### Reuse a full response when a single-connection resume must restart
- If the server returns a full response to a resume request and restart is allowed, write that response from byte zero.
- Wait for the prefix hasher to release the old partial file before replacing it.

## 2.25 / browser 0.66.70 (recorded 0.63.85) (recorded desktop 2.24)

### Preserve active downloads during long server retry waits
- Let an accepted full response finish if its optional helper receives a retry wait longer than five minutes.
- Keep partial bytes and require review if completing the download would need a new request.

## 2.24 / browser 0.66.69 (recorded 0.63.85) (recorded desktop 2.23)

### Preserve resume after a temporary helper failure
- Keep the accepted response as one resumable range when the server advertises byte ranges and provides a usable validator.
- On reconnect, request the remaining bytes and verify the returned range and resource identity before appending.

## 2.23 / browser 0.66.68 (recorded 0.63.85) (recorded desktop 2.22)

### Keep the original download moving when a helper fails to open
- Continue reading the accepted full response when the optional range request encounters a recoverable connection error, retryable HTTP response, or HTTP 416.
- Honor a retryable helper response's delay if the full transfer later needs to open another request.

## 2.22 / browser 0.66.67 (recorded 0.63.85) (recorded desktop 2.21)

### Reuse the HTTP range verification response
- When a fresh download can use multiple initial ranges, verify range support with the first helper's actual byte range.
- Reuse the open response after checking its offsets, total size and validator. The worker reads it through the normal speed limit and resume path.

## 2.21 / browser 0.66.66 (recorded 0.63.85) (recorded desktop 2.20)

### Reduce the number of initial HTTP ranges
- Start fresh, resumable downloads with balanced ranges bounded by the configured connection count.
- Split slow ranges on demand. Resume existing downloads from their saved range maps.

## 2.20 / browser 0.66.65 (recorded 0.63.85) (recorded desktop 2.19)

### Show the permission warning for direct downloads
- Show an IDM-style warning titled with the file name when a confirmed individual download ends with HTTP 403.
- Keep the failed download in the list so it can be retried or given a refreshed address.

## 2.19 / browser 0.66.64 (recorded 0.63.85) (recorded desktop 2.18)

### Match the Download Complete dialog
- Put Open, Open with, Open folder, Close and the suppression checkbox before the read-only fields in keyboard order.
- Display the abbreviated size to two decimal places without rounding up, with the exact byte count alongside it.

## 2.18 / browser 0.66.63 (recorded 0.63.85) (recorded desktop 2.17)

### Match IDM's Batch Download dialog on open
- Prefill and select a supported clipboard URL in the address field.
- Leave the file previews empty for a plain URL and populate them when the address contains a sequence.
- Disable the authorization labels and fields together until Use authorization is selected.

## 2.17 / browser 0.66.62 (recorded 0.63.85) (recorded desktop 2.16)

### Match IDM's Add URL dialog
- Prefill and select a supported clipboard URL when opening Add URL, including when automatic clipboard monitoring is disabled.
- Disable the Login and Password labels and fields until Use authorization is selected.

## 2.16 / browser 0.66.61 (recorded 0.63.85) (recorded desktop 2.15)

### Match the floating drop target
- Size the window from the 59 × 70 pixel capture at 125% scaling and use a native caption and close button with UDM artwork.
- Accept the browser's native URL drag format and retain support for dropped text and URL lists.

## 2.15 / browser 0.66.60 (recorded 0.63.85) (recorded desktop 2.05, later build)

### Match the download context menu
- Follow the captured IDM command order, group UDM commands in More actions, and allow right-click selection across the download row. [Build receipt](docs/validation/native-2.05-current-build.json)

## 2.14 / browser 0.66.59 (recorded 0.63.85) (recorded desktop 2.13)

### Match the captured dark menus
- Use distinct reference shades for the menu strip, popup menus and their highlighted items.
- Scale popup rows, separators and label insets from the capture while retaining native menu navigation.

## 2.13 / browser 0.66.58 (recorded 0.63.85) (recorded desktop 2.12)

### Match the main toolbar's dark surface and default spacing
- Use the captured gray behind toolbar buttons in dark mode.
- Target the captured 71-pixel toolbar height at 125% scaling for the built-in large icons with captions. Allow the toolbar to grow when the font or native button geometry needs more space.

## 2.12 / browser 0.66.57 (recorded 0.63.85) (recorded desktop 2.11)

### Match the Categories pane's dark colors
- Use the captured colors for the Categories tree, heading, close button and frame. Match the gray surface around the download list.
- Keep Windows light and high-contrast colors available through the existing theme controls.

## 2.11 / browser 0.66.56 (recorded 0.63.85) (recorded desktop 2.10)

### Match the main download list's dark palette
- Apply the captured IDM shades to the dark list body, grid, header and column dividers. Light and Windows high-contrast themes use the system palette.

## 2.10 / browser 0.66.55 (recorded 0.63.85) (recorded desktop 2.09)

### Match the captured download list columns
- Scale the eight visible column defaults from the captured IDM header. Use the same defaults when resetting the layout.
- Use the captured Date Added and Save To captions in the Columns dialog and download list.

## 2.09 / browser 0.66.54 (recorded 0.63.85) (recorded desktop 2.08)

### Match the reference Categories pane layout
- Frame the Categories pane and align its top with the download list beneath the toolbar.
- Scale the pane insets, tree width and close button from the captured layout. Give larger configured fonts extra room in the heading.

## 2.08 / browser 0.66.53 (recorded 0.63.85) (recorded desktop 2.07)

### Match the reference queue column width
- Use the captured IDM width for the Q column on a fresh layout and when resetting the Columns dialog. Scale it with the display DPI.
- Preserve saved column widths and order for existing profiles.

## 2.07 / browser 0.66.52 (recorded 0.63.85) (recorded desktop 2.06)

### Match the reference desktop frame
- Show the build version in the main window title.
- Use the captured IDM frame as the starting size for the main window, scaled to the display DPI.

## 2.06 / browser 0.66.51 (recorded 0.63.85) (recorded desktop 2.05)

### Show local save progress for captured browser downloads
- Show verification, copying and final save as separate steps. The copy percentage uses the bytes written to the destination staging file.
- Show the received file size, clear the network rate and ETA during the local save, and offer Retry save when the worker stops.

## 2.05 / browser 0.66.50 (recorded 0.63.85) (recorded desktop 1.105.0)

### Keep captured browser downloads paused across restarts
- Save the user's Pause decision before stopping a final-save worker. If the browser finishes receiving just as Pause arrives, the complete response still waits for Resume.
- Save the Resume decision before restarting the local copy. A worker stopped only by app shutdown can recover on startup.

## 2.04 / browser 0.66.49 (recorded 0.63.85) (recorded desktop 1.104.0)

### Resume after an interrupted staging copy
- Record the staging file's Windows identity before copying a captured response into it.
- Resume rebuilds a partial copy from the saved response when the staging file still belongs to that download. Older or unrecognized staging files are preserved and a fresh staging file is used.

## 2.03 / browser 0.66.48 (recorded 0.63.85) (recorded desktop 1.103.0)

### Choose a new destination after a captured-download save conflict
- If the destination becomes occupied before the verified staging file moves, UDM keeps the captured response and lets you choose a new destination in File Properties.
- Resume saves from the captured data. A pending save whose outcome cannot be verified keeps its original destination for recovery.

## 2.02 / browser 0.66.47 (recorded 0.63.85) (recorded desktop 1.102.0)

### Recover captured downloads after the desktop opens
- The desktop opens the download list before preparing saves left incomplete in the previous session.
- A worker can finish a captured response from its saved body and receipt, even when the browser integration is disconnected. Recovered downloads keep automatic completion actions disabled.

## 2.01 / browser 0.66.46 (recorded 0.63.85) (recorded desktop 1.101.0)

### Keep the desktop responsive while saving a captured browser response
- Verify and copy the received file on a worker. The catalog lock covers the final save transaction.
- Pause can stop preparation. Resume retries the saved browser response locally after a pause or destination error.

## 2.00 / browser 0.66.45 (recorded 0.63.85) (recorded desktop 1.100.0)

### Keep the desktop responsive during captured-download virus checks
- Run configured virus checks for original browser responses on a worker after saving the file.
- Post the completion notification after the scanner records its result. Downloads with no configured scanner report completion immediately.

## 1.99.0 / browser 0.66.44 (recorded 0.63.85)

### Carry configured virus checks through save recovery
- Save the pending check with the completed file record. After a restart or recovered save, UDM runs any pending check on a background worker.
- Preserve an existing scan result for the same completed file. A scan interrupted while the antivirus was running remains marked as interrupted.
- Keep automatic completion actions disarmed for recovered files.

## 1.98.0 / browser 0.66.43 (recorded 0.63.85)

### Recover ordinary downloads interrupted during the final save
- Record the staged file's Windows identity when the checksum is deferred. Recovery checks that identity and computes the checksum before completing the catalog entry.
- Resume can finish the pending rename for a matching staged download without contacting the server.
- Synchronization replacements record the new file's checksum before updating the original.

## 1.97.0 / browser 0.66.42 (recorded 0.63.85)

### Retry saving an assembled live DASH recording
- Save the verified output size, checksum and caption result with the sealed recording before publishing the file.
- Retry verifies the saved assembly before rereading individual segments. If the assembly is unavailable or fails verification, UDM uses its existing recording recovery.
- Show the exact assembled byte count during saving and retain the saved size after a destination failure.

## 1.96.0 / browser 0.66.41 (recorded 0.63.85)

### Protect the measured video while Download File Info is open
- Read the exact size and SHA-256 from the same open assembly for direct and recorded HLS/DASH downloads.
- Block writes and deletion while waiting for Start. Check that the staging path still identifies the measured file before handing it to the existing save workflow.

## 1.95.0 / browser 0.66.40 (recorded 0.63.85)

### Retain media assembled after Start
- Save a verified output receipt before publication when assembly finishes after Start. Retry can reuse that file if publication fails.
- Show the verified final size during publication for freshly assembled and reused media.

## 1.94.0 / browser 0.66.39 (recorded 0.63.85)

### Keep prepared video available when Start cannot save it
- Save the verified assembly receipt with Start so the video can be retried after a destination or publication failure.
- Show the failed save in the progress window when progress dialogs are enabled.

## 1.93.0 / browser 0.66.38 (recorded 0.63.85)

### Resolve interrupted saves before editing File Properties
- Recover pending file publication at its original destination before applying property changes. If recovery completes the download, reopen File Properties and use Move to choose a different location.

## 1.92.0 / browser 0.66.37 (recorded 0.63.85)

### Offer Retry save for sealed live recordings
- Keep the sealed capture state across restarts. The progress window distinguishes recording, saving and retrying a save.
- Show the saved assembly size for live HLS when a verified assembly receipt is available.

## 1.91.0 / browser 0.66.36 (recorded 0.63.85)

### Reuse a verified live HLS recording after a failed save
- Checkpoint the assembled recording before publication. Resume verifies its size, checksum and sealed recording identity before reusing it.
- Keep the source recording for normal recovery if the saved assembly is missing or fails verification.

## 1.90.0 / browser 0.66.35 (recorded 0.63.85)

### Save live recording details with the completed file
- Include completion state, assembly timing and the DASH caption result in the publication journal. An interrupted catalog save can recover those details with the file.

## 1.89.0 / browser 0.66.34 (recorded 0.63.85)

### Finish saved live HLS and DASH recordings offline
- Let local recording recovery and publication proceed before dialing. Establish the configured connection only when live capture needs another playlist, manifest or media request.

## 1.88.0 / browser 0.66.33 (recorded 0.63.85)

### Resume verified local downloads without dialing
- Check saved HTTP prefetch, completed media tracks and retained assemblies before establishing a dial-up connection.
- Recorded HLS and DASH request a connection only when a segment must be fetched or verified against the server.

## 1.87.0 / browser 0.66.32 (recorded 0.63.85)

### Show the saved assembly size for deferred video
- Download File Info, File Properties and progress show the saved output size while a retained assembly is available.
- The progress window identifies the verified final byte count when publication begins.

## 1.86.0 / browser 0.66.31 (recorded 0.63.85)

### Recover interrupted publication of retained media
- Finish a confirmed download from its verified staging file when the final name is free.
- Reconcile the catalog and clean up the retained source after a completed publication is recovered.

## 1.85.0 / browser 0.66.30 (recorded 0.63.85)

### Verify cross-drive media copies through the destination handle
- Copy prepared media in 1 MiB chunks and hash the flushed destination through the same file handle before publication.
- Clean up only temporary files created by this publication attempt.

## 1.84.0 / browser 0.66.29 (recorded 0.63.85)

### Reuse recorded HLS and DASH assemblies after Download Later
- Keep verified MP4, M4A and TS output when Download Later defers an assembled recording.
- Resume verifies the saved file and its media selection before publishing it. Missing or mismatched output goes through the regular segment workflow.

## 1.83.0 / browser 0.66.28 (recorded 0.63.85)

### Reuse verified YouTube media after Download Later
- Keep a completed MP4 or M4A assembly when Download Later defers a direct or SABR capture. Verify its size, SHA-256 and selected-stream binding before publishing it on Resume.
- Fall back to the saved tracks if the assembly is missing, damaged or belongs to an earlier selection.

## 1.82.0 / browser 0.66.27 (recorded 0.63.85)

### Save Download Later before stopping prefetch
- Commit the destination, queue membership and folder preferences together before stopping the background transfer.
- Keep prefetch available when the destination is invalid or the catalog save fails. A successful defer keeps resumable parts for the queue.

## 1.81.0 / browser 0.66.26 (recorded 0.63.85)

### Save folder history with prefetched download confirmation
- Commit the selected destination and category folder preferences with Start in one catalog write. Preserve the prepared download and previous folder settings when confirmation fails.

## 1.80.0 / browser 0.66.25 (recorded 0.63.85)

### Signal prepared downloads when Start is saved
- Wake the publication worker as soon as confirmation is saved. Keep the verified media size available without repeatedly reading the staging file.
- Include adaptive media completion metadata in the publication transaction.

## 1.79.0 / browser 0.66.24 (recorded 0.63.85)

### Show prefetched bytes when Start opens progress
- Deliver transfer notifications directly to the desktop UI. The periodic refresh can still pick up events deferred by a modal dialog.
- Show how many bytes had already been received at confirmation, including separate audio and video tracks. Keep the transfer rate based on live measurements.

## 1.78.0 / browser 0.66.23 (recorded 0.63.85)

### Prioritize accepted browser download dialogs
- Open accepted browser offers before scanning older confirmation records. Read the saved receipt journal in place and revalidate only the selected download's offer before opening it.

## 1.77.0 / browser 0.66.22 (recorded 0.63.85)

### Show the exact final byte count in compact Download File Info
- Display the byte count when verified prefetch or a supported metadata check supplies it. Label selected-track totals and estimates separately.
- Expose Check size in the compact dialog for supported captures while prefetch is inactive.

## 1.76.0 / browser 0.66.21 (recorded 0.63.85)

### Show Download File Info before loading the file association
- Load the Windows file icon and type name in the background after the dialog is visible. The dialog displays any known content type immediately and fills in the Shell details when available.

## 1.75.0 / browser 0.66.20 (recorded 0.63.85)

### Keep downloads awaiting user action out of Scheduler runs
- Share eligibility checks across queue starts, retries, synchronization recovery and wake planning. Browser recapture, pending confirmation and unresolved response handling take precedence over scheduled work.
- Preserve pending jobs for follow-up without counting them as runnable work or as exhausted retries.

## 1.74.0 / browser 0.66.19 (recorded 0.63.85)

### Apply edits to a one-time Scheduler run
- Reevaluate a rearmed one-time schedule on the next Scheduler tick, even if the previous window was still open when the edit was saved.

## 1.73.0 / browser 0.66.18 (recorded 0.63.85)

### Honor server retry deadlines in scheduled wakes
- When every eligible file must wait, defer the scheduled wake until the earliest server retry deadline. If that deadline falls outside the queue window, use the next eligible opening.

## 1.72.0 / browser 0.66.17 (recorded 0.63.85)

### Match scheduled wake work to queue preparation
- Keep the synchronization original out of wake planning while a replacement is reserved. An eligible replacement can still supply the work for the wake.
- Exclude jobs that require review before retrying. Share synchronization eligibility between queue preparation and wake planning.

## 1.71.0 / browser 0.66.16 (recorded 0.63.85)

### Recover interrupted synchronization on startup
- Recover a checkpointed synchronization check or replacement after an interruption when its queue is enabled and configured for scheduled or startup processing.
- Reconcile completed publication journals first and avoid starting a duplicate check when a replacement is already reserved. An explicit file Stop clears the active check marker.

## 1.70.0 / browser 0.66.15 (recorded 0.63.85)

### Save synchronization admission in one operation
- Commit a replacement or missing-file restoration together with its queue, connection settings, and synchronization metadata.
- Roll back the reservation if the catalog cannot be saved. A restoration rejects a destination that appeared after the missing-file check.

## 1.69.0 / browser 0.66.14 (recorded 0.63.85)

### Resume synchronization after a recurring schedule stop
- Keep an interrupted synchronization check or replacement pending when an enabled queue reaches its daily stop. The next eligible window can resume the work.
- Clear scheduled retry intent on explicit file Stop. Terminal deadlines and stopped queues leave synchronization work paused.

## 1.68.0 / browser 0.66.13 (recorded 0.63.85)

### Retain server retry deadlines after a transfer fails
- Record Retry-After when an HTTP probe or range receives it. A subsequent error without a deadline or with a shorter deadline preserves the later wait for automatic queue retries.
- Measure the saved deadline from receipt of the response so reporting that error later does not restart the wait.

## 1.67.0 / browser 0.66.12 (recorded 0.63.85)

### Honor server retry timing across HTTP range handoffs
- Share the server's Retry-After deadline before releasing a failed range. New requests for the same download wait for the latest deadline, including probes after a resource change.
- Keep reads from responses already open moving while new requests wait.

## 1.66.0 / browser 0.66.11 (recorded 0.63.85)

### Use current response progress for HTTP range handoff
- Return a failed range to peers only when another live response has delivered bytes recently. Finished responses and stale progress no longer qualify.
- Release the worker's pending disk-write reservation when its response ends, including failed write paths.

## 1.65.0 / browser 0.66.10 (recorded 0.63.85)

### Keep reassigned HTTP ranges large enough
- Check the helper's actual byte range after excluding the active worker's reserved disk write. If the preferred split leaves too little to transfer, consider another eligible range.
- Rank ordinary tail splits by the bytes the helper can actually take.

## 1.64.0 / browser 0.66.9 (recorded 0.63.85)

### Keep queue operations tied to membership
- Count and dispatch queue members when filling simultaneous-download slots. Browser-owned responses outside the queue can continue during a queue stop.
- Use registered worker tokens for queue cancellation and block global completion actions while a browser-owned response is active.

## 1.63.0 / browser 0.66.8 (recorded 0.63.85)

### Match new queue concurrency defaults
- Start new built-in queues at four simultaneous files and new custom queues at one. Retain previously saved queue limits.
- Set the fresh-profile global transfer limit to four so the main queue can reach its default concurrency.

## 1.62.0 / browser 0.66.7 (recorded 0.63.85)

### Match the Scheduler file-list presentation
- Use the reference column captions and starting widths, scaled to the display DPI, and show gridlines.
- Use a numeric simultaneous-download field and align its caption to the reference dialog.

## 1.61.0 / browser 0.66.6 (recorded 0.63.85)

### Use IDM's built-in queue labels
- Display Main download queue and Synchronization queue in the tree, Scheduler, menus, selectors, and queue notices. Scheduler places both built-in queues above custom queues.
- Keep saved queue identities and download assignments. Distinguish custom queues with a conflicting display name by appending (custom).

## 1.60.0 / browser 0.66.5 (recorded 0.63.85)

### Match queue Start and Stop availability
- Show Start now for an idle queue and Stop for an active queue cycle in the queue tree context menu and Scheduler. Recheck the queue state when either command runs.
- Show the queue menu on the Queues root, with Create new queue available.

## 1.59.0 / browser 0.66.4 (recorded 0.63.85)

### Match IDM's queue context menu
- Open a selected queue on the Files in the queue tab from Edit queue, or on the Schedule tab from Schedule.
- Use the reference command order and separators. Protect built-in queues from Delete and reject duplicate names when creating a queue from the main window.

## 1.58.0 / browser 0.66.3 (recorded 0.63.85)

### Match IDM's toolbar queue routing
- Start Queue and Stop Queue on the main toolbar target the primary download queue, including when the tree shows a different queue. Use the dropdown arrows to choose a specific queue.
- Start and Stop on a queue's tree context menu act on the queue that opened the menu. Stop uses the same availability check as the named dropdown action.

## 1.57.0 / browser 0.66.2 (recorded 0.63.85)

### Apply Save To changes immediately
- Save category folders, the remember-folder setting, the server-date setting, and temporary folders as each change is made. Closing Options with Cancel leaves these choices saved.
- Display the effective temporary directory in a read-only field. Use Browse to choose a folder or Use default to restore UDM's default location.

## 1.56.0 / browser 0.66.1 (recorded 0.63.85)

### Match category controls in Save To
- Show General first and display Compressed for the archive category. Keep category identities separate from the labels so saved rules and destinations follow the correct category.
- Display category file types and destination paths in read-only fields. Browse changes the selected destination, and General explains fallback routing with Edit disabled.
- Open the same category dialog from Save To and the tree. Its OK button commits the category immediately. Options retains pending choices for other categories and tabs.

## 1.55.0 / browser 0.66.0 (recorded 0.63.85)

### Match IDM's five default categories on new profiles
- Use the Compressed, Documents, Music, Programs, and Video extension lists found in IDM 6.43 build 10. Match archive prefixes such as `r0*` in built-in and editable category rules.
- Save unmatched downloads in the main download folder and use Compressed as the default archive subfolder. Existing profiles retain their category defaults and saved destinations.

## 1.54.0 / browser 0.65.99 (recorded 0.63.85)

### Keep file-details actions responsive during a slow lookup
- Refresh, Start, and Cancel can abandon the current metadata lookup without joining its worker. Each lookup owns its result, so late responses cannot replace a newer preview.
- Limit outstanding file-details workers to four per process. Show a timeout when the lookup deadline expires and offer a retry when the worker limit is reached.
- Recheck cancellation under the catalog lock before persisting a rotated browser session. An already committed session update keeps the existing durable save semantics.

## 1.53.0 / browser 0.65.98 (recorded 0.63.85)

### Close remote ZIP preview without joining blocked network work
- Give remote ZIP tasks independently owned request/result state and nonjoining cancellation, with at most four outstanding workers. Report cancellation immediately and a pending timeout without waiting for an operating-system call to return.
- Replace raw Manager captures in browser-session persistence with short lifetime leases. Destruction rejects new callbacks and drains callbacks already using Manager; ordinary shutdown rejects session writes while stopping.
- Retain current-session/catalog membership checks, synchronous durable cookie persistence and rollback. Preview workers retain no window/task pointer; custom callbacks must own their captured state.
- Keep ZIP byte-range/parser/validator limits and extraction policy. Network calls are not forcibly interrupted, and in-flight persistence can still delay shutdown or catalog access.
- Runtime cancellation, lifetime, session rotation, pool saturation and IDM responsiveness comparisons remain deferred.

## 1.52.0 / browser 0.65.97 (recorded 0.63.85)

### Match Find's bounded Windows input and empty-dialog behavior
- Read at most 2,047 UTF-16 units when submitting either Find UI, following the reference's 2,048-unit buffer. The draft control can still contain more text.
- Accept an empty Find dialog by retaining the prior query and applying the selected fields/case/whole-string options. Keep inline empty submission's separate session-clear behavior.
- Preserve exact Windows units for an unpaired surrogate, including one created by truncation, in bounded optional query metadata. Use those units for matching and native control restoration while keeping JSON Text valid UTF-8.
- Validate the raw-unit metadata and invalidate query/filter caches with it; remove it from an inactive filter. Existing ordinary query shape and synchronous save semantics remain.
- Runtime Unicode, clipboard, limit, option-change, persistence and IDM comparisons remain deferred.

## 1.51.0 / browser 0.65.96 (recorded 0.63.85)

### Add the reference-style submitted search bar
- Add the full-width Find label/input, Find and Settings buttons, and accessible close control using extracted IDM resource geometry and the current UI font's dialog units.
- Keep typed text as a draft until Find or Enter; timer refresh and Find Next continue using the committed query. Settings opens the existing Find dialog.
- Closing disables filtering and restores the category while retaining the query. A submitted empty field clears the session filter without overwriting the previous saved text.
- Preserve shared matching/caching and explicit-save failure handling. Move both categories and list below the bar and update control fonts with appearance/DPI changes.
- Runtime keyboard, focus, accessibility, narrow-window, DPI/theme and IDM comparisons remain deferred.

## 1.50.0 / browser 0.65.95 (recorded 0.63.85)

### Connect Find results to the visible list filter
- Accepting Find reveals and fills the inline search field, filters the current category with the selected filename/description/address and case/whole-string options, then selects the next result.
- Use the same metadata-aware matcher for inline filtering and Find Next, including protected redirected/media addresses and Referer. Snapshot search inputs under the catalog lock and match outside it; cache unchanged source/query results without retaining decrypted data.
- Include query options and exact text in view/cursor invalidation. Inline text edits update the session query; clearing the field restores the current category.
- Restore the filename checkbox after an all-fields-unchecked rejection. Find Next with no query does nothing, following the reference guard.
- Runtime interaction, filter performance, captured-address changes, accessibility and IDM comparison remain deferred. Inline typing timing and full search-bar layout still differ from the reference command-driven path.

## 1.49.0 / browser 0.65.94 (recorded 0.63.85)

### Prevent one stalled file check from blocking every advisory answer
- Replace the single command-file probe worker with up to four shared-state workers and one current deduplicated path request.
- Publish completed path answers on the next observation without waiting for the entire selection. Free workers can service a new selection while stale probes remain blocked.
- Retain matching answers during periodic refresh, reject stale generations, and keep fresh command-time validation authoritative. Closing still never joins blocked filesystem calls.
- Pool saturation, menu timing, storage faults and interaction remain runtime-unverified; tests remain deferred.

## 1.48.0 / browser 0.65.93 (recorded 0.63.85)

### Move periodic history checkpoints off the window thread
- Run periodic dirty scans, detached JSON serialization and durable catalog writes on one owned worker, coalescing pending requests.
- Preserve synchronous explicit saves, ordered snapshot publication and completion durability checks. Stop and join periodic work before the final shutdown save; allow restart after a failed shutdown.
- Keep the worker alive only within Manager lifetime. Existing checkpoint assertions are adapted for eventual persistence but remain unrun.
- Scheduler transitions and explicit actions may still save synchronously; runtime responsiveness, concurrency and shutdown tests remain deferred.

## 1.47.0 / browser 0.65.92 (recorded 0.63.85)

### Recover interrupted DASH prefix checkpoint publication
- Read protected primary and scratch prefix checkpoints independently, selecting the largest hash-verified prefix for the exact request URL and falling back to the other candidate when needed.
- Promote a verified scratch checkpoint before requesting more bytes, preserving its evidence before later checkpoint writes reuse the scratch filename. Changed/unpublishable verified metadata produces an error instead of an unguarded restart.
- Clear both old checkpoint names before resetting a retained prefix for an accepted full replacement. Preserve validator/range, unverified-tail, traffic-accounting and final whole-resource checks.
- Runtime crash, corruption, permissions, range, filesystem and IDM comparisons remain deferred.

## 1.46.0 / browser 0.65.91 (recorded 0.63.85)

### Remove completed history when the saved file is already missing
- Let the combined remove-and-recycle workflow continue approved catalog removal when Windows reports file/path not found after scoped recovery.
- Skip Shell work and successful-recycle metadata for the absent file; standalone Recycle still requires a regular existing target.
- Reject stale catalog references before recycling and recheck membership, activity and completion after recovery. Other attribute errors, directory/reparse rejection, Shell cancellation/failure and no-permanent-delete policy remain.
- Runtime missing-file, inaccessible-storage, recovery, recycle and IDM comparisons remain deferred.

## 1.45.0 / browser 0.65.90 (recorded 0.63.85)

### Separate mixed-selection removal decisions
- Confirm completed records first and unfinished records separately, with the reference-derived single/plural/mixed unfinished wording.
- Let No on the unfinished prompt preserve that group while removing the already-approved completed records; No on the completed prompt cancels the operation.
- Keep independent suppression preferences and apply Recycle Bin selection only to approved completed files. Preserve original selection processing order.
- Recheck the approved completion class before actions and after scoped recovery, retaining changed records for review. Runtime dialog, partial-failure, recycle and IDM comparisons remain deferred.

## 1.44.0 / browser 0.65.89 (recorded 0.63.85)

### Retain Time Left estimates for stopped downloads
- Save the current attempt's sampled integer transfer rate on the download record, including zero samples, and retain it when active sampling stops.
- Use the cached rate for main-list/Scheduler Time Left and Time Left sorting when the live rate is unavailable. Paused, failed and queued records can retain a remaining-transfer estimate without displaying ongoing traffic.
- Include cached rates in immutable row snapshots and normal history checkpoints; existing records without them retain an unknown estimate until a transfer supplies a sample.
- Preserve completed/live/assembly exclusions, coverage and numeric bounds, the list's 10 B/s display threshold, and active-only progress-dialog calculations. Runtime pause/restart/schedule and IDM comparisons remain deferred.

## 1.43.0 / browser 0.65.88 (recorded 0.63.85)

### Responsive completed-ZIP preview
- Create the completed-file ZIP dialog before directory reading and use the existing virtual list instead of inserting up to 100,000 rows on the window thread.
- Preserve type-to-select through an explicit virtual-list search callback with case-insensitive exact/prefix matching and requested wrap behavior, shared by local and remote ZIP lists.
- Open/read/parse on a shared-state local task, with cancellation between bounded reads and immediate dialog closure without joining blocked I/O.
- Hold a read-only file handle, reject non-disk/directory sources, apply the existing bounded parser/read budget, and verify file identity, size and last-write metadata before publishing the listing.
- Check the ZIP's authoritative open in the background task; retain current selection/completion/extension gates without a duplicate synchronous disk probe during dispatch.
- Limit outstanding local preview workers to four. Runtime file-mutation, cancellation, large-list, DPI and IDM comparison acceptance remain deferred.

## 1.42.0 / browser 0.65.87 (recorded 0.63.85)

### Background file checks for menu responsiveness
- Move ZIP/recycle advisory regular-file probes off the window thread into one background worker with a replaceable pending selection batch.
- Prewarm checks after coalesced selection changes and during regular toolbar refresh; ignore results from superseded selections and periodically refresh matching results.
- Keep unknown paths disabled until their first result, retain known advisory results while refreshing, and perform fresh filesystem checks when invoking commands.
- Keep blocked filesystem work independent of the window/manager and avoid waiting for it during shutdown. Runtime latency, menu interaction, large-selection and shutdown acceptance remain deferred.

## 1.41.0 / browser 0.65.86 (recorded 0.63.85)

### Resume DASH resources with strong modification dates
- Retain and reuse verified whole-resource prefixes on servers without ETags when a complete Last-Modified/Date pair supplies sufficient clock separation.
- Persist the date evidence in protected checkpoints and send the saved Last-Modified value in If-Range; prefer strong ETags and exclude weak, duplicated or malformed tags from date fallback.
- Require the same validator, exact remaining range, stable total and existing framing/hash checks before appending. Changed or insufficient validators use the existing full-response fallback.
- Runtime restart, server/proxy clock behavior, corruption and IDM comparisons remain deferred.

## 1.40.0 / browser 0.65.85 (recorded 0.63.85)

### Recover legacy completed DASH receipt scratch
- Recognize the known protected full-receipt scratch format from older builds and recover its already-promoted payload after exact binding, size and SHA-256 verification.
- Keep prefix checkpoints, arbitrary orphan payloads and unknown receipt schemas excluded.
- Persist an independent promoted-payload completion intent before rewriting the legacy receipt scratch, allowing another interruption to retain completion evidence.
- Runtime migration, corruption, restart and IDM comparisons remain deferred.

## 1.39.0 / browser 0.65.84 (recorded 0.63.85)

### Recover completed DASH resource publication
- Persist a protected completion intent after body completion, held-file size/hash verification and flushing, before promoting the payload and publishing its full receipt.
- Recover a matching staged or already-promoted payload during inspection/reconciliation without another HTTP request; retain range/version length requirements.
- Keep completed staging on local publication failure and surface that failure instead of deleting finished bytes or retrying another CDN.
- Remove redundant internal staging after a verified full receipt, preserving committed media. Runtime crash/fault, reconciliation and IDM comparisons remain deferred.

## 1.38.0 / browser 0.65.83 (recorded 0.63.85)

### Resumable DASH resource prefixes
- Keep protected, SHA-256-bound checkpoints for eligible whole resources with a strong ETag, including early streaming responses.
- Flush the first received prefix, then checkpoint at most once per second as data arrives; preserve the latest valid prefix on cancellation or read failure.
- Verify saved bytes before offering Range/If-Range, require the same canonical URL/entity tag and an exact single remaining-range response, and restart fully when the range cannot be used.
- Ignore uncheckpointed crash tails, compare the completed disk file with the running digest, and remove prefix state only after publishing the full-resource receipt.
- Weak/missing validators, URL changes and shared/version-bound resources retain full-request behavior. Runtime faults, pause/resume, crash and IDM comparisons remain deferred.

## 1.37.0 / browser 0.65.82 (recorded 0.63.85)

### Early live-DASH segment streaming
- Admit availabilityTimeComplete=false whole-segment locations at their signaled availability time instead of rejecting the live presentation.
- Carry per-location streaming policy through clock-uncertainty filtering and track workers; compare availability locations using the same URL canonicalization as requests.
- Require unambiguous Content-Length, HTTP/1 chunked framing or HTTP/2/3 stream framing for early responses. Read through completion before publishing a resource receipt.
- Retain existing credential scopes, cancellation, bounded temporary files and whole-segment recovery. Growing shared byte-range/version-bound resources and individual-chunk resumption remain unfinished.
- Browser, live-server, framing-fault, recovery and IDM runtime comparisons remain deferred.

## 1.36.0 / browser 0.65.81 (recorded 0.63.85)

### Find Next session cursor
- Continue Find from its own next-row cursor instead of restarting after each manually selected row, following the retained IDM Find Next routine.
- Advance after a match, wrap through row zero after reaching the end, and keep the cursor at zero after a full unsuccessful pass.
- Restart on changed query/scope; synchronize with the first restored selection after same-scope list rebuilds or ordering changes. Keep the cursor session-only.
- Follow the reference partial-visibility request and focus/selection sequence for a found row. Runtime navigation, scrolling, rebuild and IDM comparisons remain deferred.

## 1.35.0 / browser 0.65.80 (recorded 0.63.85)

### Remember Find text and options
- Restore the last accepted Find text, field choices, Match Case and Whole String options when the main window is created; F3 can use the restored query.
- Save accepted Find state through a targeted settings update with rollback on save failure, rather than replacing unrelated preferences from a stale copy.
- Keep Cancel and failed saves from replacing the active or saved query. Normalize absent/malformed saved Find fields to usable defaults during catalog loading.
- Preserve reference-derived matching and current selection-based Find Next navigation. Restart, storage-failure, GUI and IDM comparisons remain deferred.

## 1.34.0 / browser 0.65.79 (recorded 0.63.85)

### Reference-derived Find site matching
- Whole-string address search now accepts either the complete URL or its literal site portion, following the retained IDM Find helper.
- Use Windows uppercase folding for case-insensitive Find and prepare the query once per Find Next operation.
- Copy only the selected search scopes for one row at a time under the manager lock; protected header/media decoding, URL parsing and matching now run after release.
- Preserve field choices, case-sensitive search, whole filename/description matching, visible-list order and wrap behavior. Existing assertion coverage was corrected/extended but remains unrun.
- Runtime search/Unicode, GUI responsiveness and IDM comparisons remain deferred.

## 1.33.0 / browser 0.65.78 (recorded 0.63.85)

### Queue menu availability and access
- Capture ordered queue names and Stop availability under one manager lock, with at most one shared catalog pass instead of a pass per queue.
- Share the tray Start/Stop snapshot, refresh dynamic queue submenus when opened, and use captured flags during MFC's per-item update routing.
- Keep queue submenus accessible when their first queue cannot stop; each child retains its own enabled state.
- Preserve relocation exclusions, active-transfer handling, enabled queued/synchronization work, manual/cycling work, bounded menu IDs and the large-queue chooser. Recheck Stop eligibility on execution.
- Runtime timing, menu interaction and IDM comparisons remain deferred.

## 1.32.0 / browser 0.65.77 (recorded 0.63.85)

### Batched command availability and shorter UI lock scopes
- Read list selection outside the manager lock and batch toolbar/context-menu decisions from one captured selection.
- Share the catalog pass for requested global Stop All, open-progress, completed-cleanup and recovery decisions, with short-circuit completion when possible.
- Remove the context menu's outer manager lock; resolve detached ZIP/recycle file checks and redownload FTP URL parsing before creating/updating controls.
- Reuse exact-path file status only within one batch; filesystem errors disable the relevant command. Action-time eligibility is still freshly queried.
- Skip selection/manager/settings work for commands that do not need it, and return an empty selection while the list control is absent.
- File probes remain synchronous on the UI thread; runtime timing, concurrency and IDM comparisons remain deferred.

## 1.31.0 / browser 0.65.76 (recorded 0.63.85)

### Navigation snapshots and stable tree refresh
- Limit navigation's manager-lock scope to category names, queue names/icon kinds and project IDs/names; compare snapshots, serialize structural keys and update Windows controls afterward.
- Update queue schedule/type icons in place, preserving node handles and avoiding full tree rebuilds for image-only changes.
- Preserve expanded groups, selected identity and vertical item anchor across structural rebuilds. Removed selections fall back to the nearest surviving ancestor; the Queues root has a distinct identity from All Downloads.
- Restore redraw and invalidate the rebuild cache after interrupted construction. Keep queue-image invalidation compatible with image-list/DPI refresh.
- Existing navigation assertions were maintained; runtime interaction, concurrency, performance and IDM comparisons remain deferred.

## 1.30.0 / browser 0.65.75 (recorded 0.63.85)

### Fractional media-tick MP4 caption timestamps
- Keep inline WebVTT millisecond offsets independent of the MP4 track timescale, avoiding premature whole-tick truncation before forward edit-rate scaling.
- Combine rational inline offsets with sample/edit, presentation-offset and movie clocks before final millisecond rounding, with checked integer additions and bounded clock inputs.
- Subtract the edit origin before adding inline offsets, avoiding unnecessary large absolute-clock intermediates.
- Advance caption assembly compatibility to revision 4 so retained live Period/session outputs rebuild under the corrected mapper; audio/video-only reuse is unchanged.
- Reverse/dwell timed text still requires temporal text-state support and remains explicitly unsupported. Runtime tests, installers and installation remain deferred.

## 1.29.0 / browser 0.65.74 (recorded 0.63.85)

### Reference-derived Size and Transfer Rate ordering
- Sort unknown/zero-byte-size media by unrounded duration, with positive byte sizes forming a separate group, following IDM's Size comparator.
- Sort and display main-list transfer rates as whole bytes per second; use the same conversion for list ETA. Invalid/non-finite rates become zero, and finite overflow is guarded before integer conversion.
- Reuse ordering only when the byte/duration or integer-rate keys match, preserving stable equal-key catalog order and snapshot-based work outside the manager lock.
- Last Try Date comparator review is recorded separately; malformed/legacy date behavior and all runtime comparisons remain unverified. Tests and installers remain deferred.

## 1.28.0 / browser 0.65.73 (recorded 0.63.85)

### Uncapped ETA calculation and reference-derived Time Left behavior
- Remove the one-day cap from duration-based media ETA, retaining the current-attempt resume baseline.
- Guard byte/media estimates before integer conversion; invalid coverage stays unknown and estimates beyond the integer range saturate safely. Fully received active downloads show zero seconds remaining.
- Use integer-rate estimates in list cells and suppress Time Left at 10 B/s or less, while progress dialogs retain fractional-rate estimates.
- Sort Time Left with zero for unavailable values and completion-state tie breaking, following the reference callback; low-rate estimates can still participate in sorting while their cells are blank.
- Preserve the reference formatter's blank-space result above 10,000 hours. Native runtime and reference comparisons remain deferred.

## 1.27.0 / browser 0.65.72 (recorded 0.63.85)

### Reference-derived Status percentage display
- Preserve percentage text for unfinished downloads with retained progress, including failures, instead of replacing it with Error/Not Found solely because the state is Failed.
- Use duration, segment or byte coverage in that order; format unrounded source coverage through the reference's float and truncated-hundredths path.
- Honor the Windows decimal separator, with a thread-local cache invalidated on settings changes; repaint unchanged rows when locale settings change.
- Show neutral blank cells before progress is available. Keep unstarted failure labels, live-recording states and explicit assembly phases.
- Percentage text can reach 100 before publication, as in the reference; Complete still requires the stored completion state. Runtime tests and visual/locale comparisons remain deferred.

## 1.26.0 / browser 0.65.71 (recorded 0.63.85)

### IDM Status ordering and HTTP 404 labels
- Sort Status by completion group and unrounded progress across unfinished downloads, including paused and queued rows, instead of alphabetic state names and rounded active-only percentages.
- Follow the reference's byte-ratio/media-percent score distinction and zero-progress fallback-label ordering in both directions; keep stable equal keys.
- Show Not Found for failed HTTP 404 downloads and retain Error for other failures. Include the HTTP result in immutable list snapshots so changes refresh correctly.
- Bind sort reuse to completion, full progress score and fallback label. Runtime reference, media-clock mapping and visual comparisons remain deferred.

## 1.25.0 / browser 0.65.70 (recorded 0.63.85)

### IDM multibyte Description and Referer sorting
- Compare temporary Windows ANSI keys using the multibyte CRT path traced in IDM, instead of lowercasing UTF-8 bytes.
- Retain full Unicode metadata/display text and raw-key cache invalidation; these columns have dynamically sized comparison keys, without the wide getter's 255-unit limit.
- Keep empty-last ascending order and stable ties. If the CRT reports a comparison error, restart the entire sort with a consistent byte comparator instead of mixing error results into ordering.
- Runtime code-page, locale, visual and performance comparisons remain deferred.

## 1.24.0 / browser 0.65.69 (recorded 0.63.85)

### IDM optional-text column ordering
- Sort populated Description, Save Path, Referer and Parent Web Page values before empty values ascending, reversing that order descending.
- Compare Save Path and Parent Web Page with case-insensitive UTF-16 CRT comparison, following the reference callbacks.
- Bound filename, Save Path and Parent Web Page comparison keys to the reference getter's 255 UTF-16 units; retain complete stored and displayed text.
- Preserve stable ties and unchanged-order reuse. Runtime comparisons and locale qualification remain deferred.

## 1.23.0 / browser 0.65.68 (recorded 0.63.85)

### IDM-style logical filename ordering
- Use Windows StrCmpLogicalW for the main list's File Name sort, matching the call traced from the installed IDM reference's By File Name command.
- Compare Unicode filenames with numerical digit runs and case-insensitive Windows ordering in both directions, retaining stable equal-key order.
- Build UTF-16 keys only when a new filename sort is needed; unchanged-order validation reads immutable raw names without extra string copies.
- Invalidate cached ordering on Windows settings broadcasts. Runtime comparative and locale tests remain deferred.

## 1.22.0 / browser 0.65.67 (recorded 0.63.85)

### Reuse unchanged download-list ordering
- Skip repeated stable sorting when the selected column/direction, candidate sequence and relevant keys match the prior refresh.
- Re-sort after additions, removals, job replacement, catalog reordering or relevant key changes, preserving existing stable-tie behavior.
- Continue updating progress, speed, quota state and asynchronous icons independently of sort reuse.
- Runtime performance and interaction tests remain deferred.

## 1.21.0 / browser 0.65.66 (recorded 0.63.85)

### Background file-type icons
- Show category/cached icons immediately and resolve Windows file-type icons on a dedicated COM-initialized background worker.
- Load icons as rows enter the viewport, deduplicate and bound pending work, and discard stale results when appearance/DPI image lists are rebuilt.
- Keep all list/image-list updates on the UI thread; closing UDM does not wait for a blocked Shell lookup.
- Recompute category fallbacks per row when an association lookup fails; runtime responsiveness and visual tests remain deferred.

## 1.20.0 / browser 0.65.65 (recorded 0.63.85)

### Download-list refresh responsiveness
- Snapshot list fields and queue indicators under the manager lock, then release it before sorting, header decoding, shell-icon lookup and row painting.
- Cache display-only row records instead of full download records containing transfer journals and capture state.
- Preserve selection, focus, scroll anchoring, filters, quota indicators, sort keys and displayed progress from one consistent refresh snapshot.
- Runtime performance comparisons and interaction tests remain deferred.

## 1.19.0 / browser 0.65.64 (recorded 0.63.85)

### Live-DASH captions across silent Periods
- Join audio/video Periods independently and mux rebased caption cues once for the full recording.
- Preserve verified caption coverage and configuration checks, bounded cue storage, language/name metadata and retained-fragment recovery.
- Save all-silent intervals without fabricated cues, with a persisted caption result visible in progress details and Last attempt and result.
- Advance caption output receipt revision and retain cue counts across assembled-output reuse; runtime tests remain deferred.

## 1.18.0 / browser 0.65.63 (recorded 0.63.85)

### Direct queue controls from the tray
- List named queues directly under tray Start queue and Stop queue submenus, using the existing main-menu queue commands.
- Disable inactive Stop entries when populating a queue menu.
- Use an editable queue chooser beyond 1,000 entries instead of assigning an unbounded range of popup command IDs.
- Runtime and visual tests remain deferred.

## 1.17.0 / browser 0.65.62 (recorded 0.63.85)

### Media assembly track verification
- Verify selected audio and video tracks before exact-size confirmation and publication.
- Compare source/output codecs, video dimensions, audio channels/sample rate and available per-track durations; retain saved streams on verification failure.
- Keep the selected-height and AAC audio-only checks; runtime tests remain deferred.

## 1.16.1 / browser 0.65.61 (recorded 0.63.85)

### Caption assembly recovery compatibility
- Rebuild retained live-DASH caption outputs created before the current timing and mux rules.
- Bind both Period and joined-session assembly receipts to the caption implementation revision while preserving verified downloaded fragments and audio/video-only reuse.
- Runtime checks remain deferred.

## 1.16.0 / browser 0.65.60 (recorded 0.63.85)

### Forward MP4 WebVTT inline timing
- Map inline cue timestamps through ctim, positive playback edits and presentation offsets.
- Rebase mapped timestamps during recorded Period joining and live capture clipping.
- Reject missing timing metadata and unsupported reverse/dwell timed payloads explicitly; runtime checks remain deferred.

## 1.15.3 / browser 0.65.59 (recorded 0.63.85)

### MP4 WebVTT cue timing metadata
- Preserve and validate optional ctim metadata through sample/fragment decoding.
- Inline timestamp remapping remains unfinished; runtime checks remain deferred.

## 1.15.2 / browser 0.65.58 (recorded 0.63.85)

### Live-DASH caption output metadata
- Apply the selected caption label/language to final output, including single-Period sessions.
- Version caption assembly reuse records so older results cannot omit the metadata update.
- Runtime checks remain deferred.

## 1.15.1 / browser 0.65.57 (recorded 0.63.85)

### Live-DASH track metadata
- Validate selected audio/caption names, language tags and metadata-to-track consistency at native admission.
- Match the existing recorded-selection metadata bounds; runtime checks remain deferred.

## 1.15.0 / browser 0.65.56 (recorded 0.63.85)

### Browser live-DASH captions
- Discover and select supported live MP4 WebVTT caption tracks in Chromium and Firefox.
- Gate handoff on desktop support and include caption addressing/credential checks.
- Browser-to-file runtime verification remains deferred.

## 1.14.0 / browser 0.65.55 (recorded 0.63.84)

### Native live-DASH captions
- Admit a selected MP4 WebVTT track alongside video and optional audio, with explicit initialization and media-kind checks.
- Preserve three-track selection through recording-session recovery and refresh.
- Advertise desktop capability for forthcoming browser integration; runtime acceptance remains deferred.

## 1.13.1 / browser 0.65.54 (recorded 0.63.84)

### DASH recording caption muxing
- Decode retained MP4 WebVTT into an explicitly mapped subtitle stream with language metadata.
- Validate caption counts and preserve compatible subtitles across joined Period outputs.
- Live capture admission remains unfinished; runtime checks remain deferred.

## 1.13.0 / browser 0.65.53 (recorded 0.63.84)

### Retained DASH caption assembly
- Add native verified-file assembly of retained MP4 WebVTT fragments into clipped, rebased subtitle text.
- Preserve byte-range offsets and reject changed or incomplete retained resources.
- End-to-end live caption selection and muxing remain unfinished; runtime tests remain deferred.

## 1.12.3 / browser 0.65.52 (recorded 0.63.84)

### DASH media-kind preflight
- Verify requested live track kinds against inherited manifest metadata before recording.
- Add explicit MP4 WebVTT classification for the forthcoming caption contract; live captions remain unfinished.
- Runtime checks remain deferred.

## 1.12.2 / browser 0.65.51 (recorded 0.63.84)

### Shared MP4 WebVTT decoding
- Route recorded caption assembly through a retained-file decoder with explicit byte offsets and presentation clock bounds.
- Preserve existing fragment continuation and edit-list mapping for future live recording integration.
- Live captions remain unfinished; runtime tests remain deferred.

## 1.12.1 / browser 0.65.50 (recorded 0.63.84)

### Completed-file controls during relocation
- Disable conflicting completed-file actions and skip active records in Delete Completed.
- Keep download-progress closure independent of file-move cancellation; read destination text from a consistent snapshot.
- Runtime and visual checks remain deferred.

## 1.12.0 / browser 0.65.49 (recorded 0.63.84)

### File-move speed and timing
- Show measured processing speed, total elapsed time and current-phase remaining time during file moves.
- Reset samples between phases and decay speed during stalls; retain destination and byte-count displays.
- Runtime and visual checks remain deferred.

## 1.11.3 / browser 0.65.48 (recorded 0.63.84)

### Move recovery before file actions
- Reconcile the selected download's pending move before Remove, Recycle or Redownload.
- Preserve unresolved history and leave unrelated move journals alone.
- Runtime validation remains deferred.

## 1.11.2 / browser 0.65.47 (recorded 0.63.84)

### File moves and transfer scheduling
- Exclude relocation from transfer slot accounting and automatic queue-stop cancellation.
- Preserve file protection, shutdown cancellation and completion-action waiting while a move runs.
- Runtime validation remains deferred.

## 1.11.1 / browser 0.65.46 (recorded 0.63.84)

### Relocation verification progress
- Show byte progress during destination/original verification and cross-volume rollback.
- Keep the destination visible alongside phase byte counts; protect verification reads against concurrent writers.
- Recovery remains non-cancellable after a successful move. Runtime and visual checks remain deferred.

## 1.11.0 / browser 0.65.45 (recorded 0.63.84)

### Background Move/Rename
- Connect the desktop Move/Rename and Properties workflows to a background progress dialog with phase-aware cancellation.
- Release the manager lock during long file operations, reserve the destination, and wait for relocation during shutdown.
- Preserve unrelated metadata during location commit and recovery. Runtime and visual acceptance remain deferred.

## 1.10.0 / browser 0.65.44 (recorded 0.63.84)

### Move/Rename progress and cancellation backend
- Add optional phase/byte progress and cancellation to native relocation; existing callers keep their default behavior.
- Allow cancellation during initial hashing and Windows cross-volume copying, verifying original-file recovery before reporting a clean cancellation.
- Finish verification/history or rollback after a successful move despite late cancellation.
- Desktop progress/cancel UI and asynchronous locking remain unfinished; runtime tests remain deferred.

## 1.9.2 / browser 0.65.43 (recorded 0.63.84)

### Same-volume Move/Rename performance
- Avoid whole-file checksum reads for supported same-volume renames by locking the source against writes and verifying its Windows file identity, size and timestamps.
- Use the recorded identity for case-only renaming, rollback and recovery; retain compatibility with older checksum journals.
- Keep fresh checksum verification for cross-volume moves and unsupported filesystem identities. Runtime latency and background progress/cancellation remain unverified or unfinished.

## 1.9.1 / browser 0.65.42 (recorded 0.63.84)

### Move/Rename current-file recovery
- Journal and verify the actual file being moved, including files edited since download, without replacing historical download checksums.
- Verify original-file contents during recovery and verify rollback before clearing its journal.
- Preserve uncertain move outcomes and report Windows copy-without-source-removal failures instead of reporting a completed move.
- Runtime acceptance and large-file responsiveness remain outstanding.

## 1.9.0 / browser 0.65.41 (recorded 0.63.84)

### Completed-download filename case
- Allow Move/Rename to change only filename letter case, retaining destination collision protection.
- Record actual spelling and fresh content identity for recovery; verify stored spelling after rename and rollback.
- Preserve ambiguous recovery journals and prohibit file moves during deferred catalog saves. Runtime acceptance remains deferred.

## 1.8.0 / browser 0.65.40 (recorded 0.63.84)

### DASH per-segment index templates
- Accept Number/Time index templates alongside independently timed complete media segments, instead of misclassifying them as whole-representation indexes.
- Validate optional index locators without fetching their resources or sharing credentials with their origins; preserve the existing representation-wide SIDX path.
- Gate live handoff on desktop support. Runtime acceptance and index-based subsegment selection remain deferred.

## 1.7.0 / browser 0.65.39 (recorded 0.63.83)

### DASH duration-boundary overlaps
- Accept bounded non-negative pdDelta declarations in native DASH addressing and both browser catalogs instead of rejecting every duration-boundary declaration.
- Route recorded declarations through finite Period assembly, retaining boundary clipping without changing segment URLs or extending the output duration.
- Allow supported live template, list and indexed selections with duration overlaps; require the updated desktop capability before handoff.
- Negative duration gaps, malformed values and values beyond exact integer bounds remain unsupported. Runtime acceptance remains deferred.

## 1.6.8 / browser 0.65.38 (recorded 0.63.82)

### Catalog duplicate identity
- Distinguish imports by usable browser session, proxy route, form request, capture requirements, offline project and expected hash as well as URL and destination.
- Preserve distinct contexts as separate numbered entries rather than silently skipping them.
- Remember import-assigned filenames so importing the same numbered entry again does not create another copy; later filename changes invalidate that name match.
- An unreadable existing credential record no longer blocks importing a separate usable record. Incoming credentials still require validation.

## 1.6.7 / browser 0.65.37 (recorded 0.63.82)

### Imported download request context
- Preserve required form and media recapture flags when attaching verified completed files from a catalog; the saved files remain completed and usable.
- Preserve media recapture requirements through subsequent catalog exports.
- Download Again rejects missing form/media context and validates the saved browser session before creating a new entry, instead of treating an imported contextual download as an ordinary URL.

## 1.6.6 / browser 0.65.36 (recorded 0.63.82)

### Site Grabber start recovery
- Recover selected existing downloads before Site Grabber begins its start-batch transaction, so recovered history is durable before journals are removed.
- Preserve publication and replacement journals if recovery is requested inside a transaction that defers catalog saves.
- Preserve already committed recovery if a later project-start step fails; unselected downloads remain outside the recovery pass.

## 1.6.5 / browser 0.65.35 (recorded 0.63.82)

### Offline website admission
- Save a standalone offline archive entry only after its project settings, captured HTML and browser login are prepared.
- Restore the prior job list when archive preparation or persistence fails, avoiding incomplete entries after a restart.
- Preserve enclosing Site Grabber and Download Again transactions, so archive creation cannot commit their unfinished work.

## 1.6.4 / browser 0.65.34 (recorded 0.63.82)

### Download Again catalog transaction
- Commit new Download Again entries only after their inherited properties, browser session and offline-page data are ready.
- Roll back the new entry if preparation or the final catalog save fails, preventing partially configured entries from surviving a restart.
- Reject stale selections and overlapping catalog transactions before creating the copy.

## 1.6.3 / browser 0.65.33 (recorded 0.63.82)

### Move/Rename recovery
- Resolve pending publication and replacement recovery for the selected download before changing its file location, then recheck completion.
- Keep files at their existing location while a pending replacement still depends on them; reject stale catalog selections.
- Restore the in-memory record if saving recovered move history fails, retaining the recovery journal for retry.

## 1.6.2 / browser 0.65.32 (recorded 0.63.82)

### Removal recovery
- Reconcile only the selected download's publication and replacement records, including private audio/video tracks, before removing its catalog entry.
- Reject active or stale selections before removal, and preserve the record if recovery cannot complete.
- Recover pending saves before recycling a completed file and recheck its completion status before invoking Windows.

## 1.6.1 / browser 0.65.31 (recorded 0.63.82)

### WebVTT text timestamp synchronization
- Shift timestamps embedded in plain WebVTT cue text together with cue boundaries during HLS timestamp mapping and recorded/live discontinuity placement.
- Apply the same mapping to plain WebVTT DASH Period placement and clipping.
- Preserve text and formatting, remove timestamp markers outside the clipped cue, and reject recognized non-increasing or out-of-cue timestamps.
- MP4 WebVTT payload timestamp remapping remains deferred.

## 1.6.0 / browser 0.65.30 (recorded 0.63.82)

### Reverse MP4 caption playback edits
- Support finite reverse MP4 WebVTT edits at fixed-point speeds down to -16x, alongside forward and dwell edits.
- Reverse cue interval endpoints onto the movie clock, preserve text/settings, and retain movie-duration clipping and presentation sorting.
- Reject open-ended reverse edits and protect signed media-distance subtraction from overflow.

## 1.5.0 / browser 0.65.29 (recorded 0.63.82)

### Fractional MP4 caption playback edits
- Preserve the full 16.16 forward playback rate for recorded MP4 WebVTT edits, including slow motion and fractional acceleration through 16x.
- Convert caption clocks using bounded integer quotient/remainder scaling, avoiding direct multiplication of large timestamps.
- Retain finite dwell edits, movie-duration clipping, DASH offsets and output budgets; reverse playback remains unsupported.

## 1.4.0 / browser 0.65.28 (recorded 0.63.82)

### MP4 caption dwell edits
- Support finite zero-rate edits in recorded MP4 WebVTT captions, holding cues active at the selected media timestamp for the specified movie duration.
- Respect inclusive cue starts and exclusive cue ends, movie placement, DASH offsets, empty samples and existing output limits.
- Reject open-ended dwell edits; retain whole-number forward playback support from 1x through 16x.

## 1.3.0 / browser 0.65.27 (recorded 0.63.82)

### MP4 caption playback edits
- Support positive whole-number playback rates from 1x through 16x when assembling recorded MP4 WebVTT captions.
- Apply each edit's speed to media timing before movie placement and DASH presentation offsets; retain movie-duration clipping for finite edits.
- Preserve repeated/reordered edits, empty gaps and existing output limits. Fractional, zero and reverse rates remain unsupported.

## 1.2.15 / browser 0.65.26 (recorded 0.63.82)

### Replacement recovery
- Prepare both replacement-history records before changing the live catalog during recovery.
- Restore both in-memory records if saving the recovered catalog fails; retain the journal for another recovery attempt.
- Read required recovery snapshots before moving the preserved original back into place.

## 1.2.14 / browser 0.65.25 (recorded 0.63.82)

### Resume recovery
- Recover interrupted publication and replacement saves for the selected download and its private audio/video tracks when Resume is clicked, without requiring a full application restart.
- Leave unrelated downloads' recovery journals untouched and refuse recovery while the selected transfer or replacement original is active.
- Validate replacement-journal filenames against their recorded download IDs before applying recovery.

## 1.2.13 / browser 0.65.24 (recorded 0.63.82)

### Media recovery
- Find saved audio/video child tracks when recovering interrupted file publication, instead of requiring every track to be a top-level download.
- Keep path/hash validation and durable parent-catalog saving; reject ambiguous duplicate identifiers without deleting recovery data.
- Keep replacement-history recovery restricted to top-level download records.

## 1.2.12 / browser 0.65.23 (recorded 0.63.82)

### Media publication
- Allow internal audio/video track jobs to publish while preserving their initial catalog-membership state. The 1.2.9 membership guard incorrectly required these private child jobs to appear in the main download list.
- Continue rejecting changes to membership, destination and replacement metadata during verification.

## 1.2.11 / browser 0.65.22 (recorded 0.63.82)

### Interrupted-save recovery
- Preserve direct-download staging files while publication or replacement recovery is pending, including when journal state cannot be read.
- Prevent direct, recorded-media, live DASH and offline-archive assembly from overwriting pending recovery output on retry.
- Use the shared conservative journal check in recorded-media cleanup without replacing the original failure with a filesystem-check exception.

## 1.2.10 / browser 0.65.21 (recorded 0.63.82)

### Live recording recovery
- Preserve assembled live HLS output after a publication failure when its recovery journal exists or cannot be inspected.
- Block another assembly pass from overwriting that output until publication recovery is resolved.
- Share the conservative recovery-journal check with media publication cleanup.

## 1.2.9 / browser 0.65.20 (recorded 0.63.82)

### Responsiveness
- Move final replacement-file verification outside the catalog lock and pass Pause/cancellation through direct, media, live HLS and offline publication paths.
- Hash held file handles and revalidate file identity and replacement metadata before committing. Preserve the existing publication journal, previous-version backup and history rollback.

## 1.2.8 / browser 0.65.19 (recorded 0.63.82)

### Responsiveness
- Prepare synchronization replacement hashes outside the catalog lock, with cancellation between blocks, so large existing files do not block catalog access during that hash pass.
- Reuse the verified digest for replacement admission while holding a read-only sharing handle, and check file identity before consuming it.

## 1.2.7 / browser 0.65.18 (recorded 0.63.82)

### Scheduler
- Stop unscheduled periodic synchronization at local midnight when no explicit stop time is selected, matching IDM's documented end-of-day limit.
- Preserve the deadline across ordinary settings saves; an explicit Start or configured startup start arms the next deadline. Explicit stop times keep priority.

## 1.2.6 / browser 0.65.17 (recorded 0.63.82)

### Synchronization
- Verify the saved SHA-256 before declaring an unchanged remote file up to date, detecting local edits or corruption even when the file length is unchanged.
- Keep verification cancellable and prevent concurrent file writes during hashing. Legacy records without a valid saved hash retain their existing metadata check.

## 1.2.5 / browser 0.65.16 (recorded 0.63.82)

### Installation
- Register Download with UDM for .url files through SystemFileAssociations rather than modifying the InternetShortcut class, fixing the access-denied installation failure.
- Notify Explorer of association changes during installation and removal without changing the default browser.


## 1.2.4 / browser 0.65.15 (recorded 0.63.82)

### Fixes
- Stop automatic queue retries when a fresh browser proxy or browser session capture is required.
- Keep those entries from being treated as exhausted retry work that permits queue completion actions.


## 1.2.3 / browser 0.65.14 (recorded 0.63.82)

### Fixes
- Preserve restored-file synchronization status after resume, retry, and publication recovery.
- Stop automatic synchronization retries for webpage responses and content-prefix errors that require user review.


## 1.2.2 / browser 0.65.13 (recorded 0.63.82)

### Fixes
- Refine synchronization restoration enrollment, queue membership, and recovery state for files that need to be restored locally.

## 1.2.1 / browser 0.65.12 (recorded 0.63.82)

### Fixes
- Let synchronization restore a missing local destination instead of failing before it can download a replacement.
- Keep duplicate replacement state and synchronization status accurate when a file is restored in place.

## 1.2.0 / browser 0.65.11 (recorded 0.63.82)

### DASH and media capture
- Add recorded and live DASH handling for BaseURL, SegmentList, SegmentTemplate, and index-derived segment addressing.
- Improve DASH period windowing, timing offsets, finalized-segment draining, and recovery across changing manifests.
- Add MP4 WebVTT subtitle fragment parsing and sidecar caption assembly for supported adaptive downloads.
- Preserve media request identity and revalidate browser receipts before handing a capture to the desktop app.

### Desktop experience
- Refine adaptive-download metadata, admission limits, queue and scheduler roles, toolbar presentation, and local-size synchronization.

## 1.1.0 / browser 0.64.92 (recorded 0.63.63)

- DASH address refresh now treats declared duration as display metadata, allowing older captures without that field to refresh. Accepted refreshes replace or clear the displayed length transactionally while retaining segment identity checks. Runtime checks deferred.

- Recorded DASH captures now retain a valid declared presentation duration for the download list when byte size is unknown. The value remains media length, not a file-size estimate. Chromium/Firefox 0.63.56; runtime checks deferred.

- Advanced per-protocol proxy settings now expose Automatic, Negotiate, NTLM, Digest and Basic for explicit HTTP/HTTPS logins. Forced choices do not fall back to another method; existing profiles remain Automatic and browser routes inherit only matching proxy settings. Runtime/domain checks deferred.

- HTTPS form downloads can now negotiate explicit proxy login after a confirmed CONNECT rejection before any upload. Plain HTTP POST and ambiguous failures remain excluded from this retry; domain/runtime checks are deferred.

- IDM export now rejects separately stored form bodies instead of silently emitting an empty POST, and rejects request text that cannot round-trip as UTF-8. Export format descriptions now explain URL-only output and large-form catalog limitations. Runtime checks deferred.

- Combined total and per-file speed limits now reserve both allowances before waiting for the later deadline, instead of adding both delays. Short reservations and live-setting refresh remain; runtime throughput checks are deferred.

- Regular download pacing now re-reads live speed settings between short bandwidth reservations, reducing stale waits after raising or disabling a limiter. Shared total/per-file accounting and quota charging are retained; runtime timing checks are deferred.

- Explicit proxy logins now attempt Negotiate for GET/HEAD after a terminal Negotiate-only proxy challenge. Existing Basic/Digest/NTLM preference and POST behavior are preserved. Domain authentication and runtime checks remain deferred.

- Browser import now preserves explicit choices per row during metadata lookup, keeps unrelated rows eligible for automatic file selection, and submits in displayed order. Late lookup failures no longer overwrite submission progress. Extension 0.63.55; runtime tests deferred.

- Browser import properties now support per-file Referer overrides, including omission, while retaining the original source-page record. A desktop capability gate prevents silent fallback; custom request settings suppress stale browser size metadata. Extension 0.63.54. Runtime tests deferred.

- Browser handoff now shares one fresh desktop capability request across proxy, import properties, POST and session checks, removing up to three redundant bridge round trips per link. No capability cache survives into the next handoff. Extension 0.63.53; runtime timing tests deferred.

- Browser batch review now reports acknowledged handoff progress and retains bounded submission receipts across page reloads or worker interruptions. Active batches cannot be evicted; interrupted lists remain single-use. Extension 0.63.52. Runtime tests deferred.

- Browser link review can now submit all 500 collected links instead of disabling submission above 100. Chromium and Firefox 0.63.51 retain sequential handoff and single-use submission guards. Runtime tests deferred.

- Native import File Properties now displays live type, lookup status and exact reported byte size. Unsaved address or request changes suppress stale metadata until saved and refreshed. Runtime tests deferred.

- Native import properties now offer per-file login and Referer overrides. Protected overrides follow each original row into metadata requests and download admission; shared credentials remain the default. Runtime tests deferred.

- Native import Edit now follows the reference single-highlight rule and updates immediately when highlighting changes, avoiding ambiguous edits to the first of several highlighted rows. Runtime tests deferred.

- Native import metadata now clears the default selection for HTML pages. Explicit checkbox and Check all/Uncheck all choices survive late metadata, filtering and authorization refreshes. Runtime tests remain deferred.

- Batch URLs now replace every asterisk with the same original number or letter, matching the retained IDM generator evidence. Previews and generated rows share this behavior; runtime tests are deferred.

- Native import review now shows sortable Folder override and Description columns, preserving per-file values through filtering and editing. Blank folder overrides inherit the shared destination policy. Runtime testing remains deferred.

- Delivery builds now offer a production-only target set, copy all native dependency notices, and package isolated release/browser directories. Installer version checks now include COM automation; the MIME-data notice is shipped with setup. Runtime qualification remains deferred.

- Native metadata previews now resolve final-response URL filenames and apply specific MIME suffixes to generic names or URL-only script endpoints, sharing the extension's licensed MIME data. Specific names and the extension-correction preference retain precedence. Runtime tests deferred.

- Native imports add Hide HTML files and Hide repeated files. Hidden selections are preserved and counted; metadata-driven filtering and per-file edits retain original row identity. Runtime tests deferred.

- Batch output patterns now use server filename extensions from metadata. The first non-HTML extension updates the shared pattern once; mixed files retain per-row extensions and explicit per-file edits remain authoritative. Runtime tests deferred.

- Native imports now preview server-reported size, MIME type and server filename suggestions in the background. Four bounded metadata workers keep the dialog responsive; URL/authorization changes cancel stale lookup generations. Size and Type columns sort with unknown values last. Runtime tests deferred.

- Generated number/letter batches now offer a live output filename pattern. Every asterisk uses the row's original sequence token, even after sorting; per-file names override the pattern. Selected names are validated before admission. Runtime tests deferred.

- Native batch generation now accepts descending numbers and letters, matching the retained IDM sequence-generation evidence. Zero padding, inclusive endpoints and the 1,000-URL numeric limit are preserved. Runtime tests deferred.

- Native batch/text imports now sort by File name and Address with direction arrows, preserving per-link properties, checkboxes and already-added tracking across reordering. Admission follows the displayed order. Runtime tests deferred.

- Explicit import categories now survive synchronization replacements and UDM catalog export/import. Missing categories fall back to the current profile's category rules, and malformed category-choice markers are rejected before catalog publication. Runtime tests deferred.

- Browser integration 0.63.50 sends the selected-links page popup through import review before desktop submission. Explicitly checked links remain checked during metadata inspection, with their source frame and link text retained. Context-menu selected-link imports now preserve their initial selection too. Runtime tests deferred.

- Native import now offers each file's category folder, one category for all files, or one folder for all files. Explicit category choices survive content-based filename correction, redownload and duplicate restart; per-link folder overrides remain available. Build/source checks only, runtime tests deferred.

- Browser integration 0.63.49 adds the import options Use link texts as descriptions and Hide images on this page, backed by the retained IDM dialog-228 controls. Collection includes page image resources and retains original source-frame context. Explicit descriptions take precedence over link text. Source checks only; runtime tests deferred.

- Native batch/text import adds an Edit command for each unimported link's address, output filename, destination folder and description. Per-link folders override the shared import folder; existing duplicates retain their saved metadata. Compilation/source checks only, with interactive and regression testing deferred.

- Browser integration 0.63.48 adds explicit per-link HTTP authorization to import properties. Credentials use the existing protected desktop-header path, are not saved with browser batch records, and are cleared from the page after submission finishes. Runtime tests deferred.

- Browser integration 0.63.47 adds sortable import columns, MIME-aware file selection, selected-byte totals and hidden-selection counts. Metadata updates preserve manual selection; sorted imports retain their displayed order. Unknown sizes remain separate from known-byte totals. Source checks only; runtime tests deferred.

- Browser integration 0.63.46 adds background size/type inspection and Hide HTML files to link import. It reuses observed response metadata or sends bounded HEAD requests in the source frame, honors cookie-sharing settings, and leaves unavailable or encoded sizes unknown. HEAD requests do not become download or media captures. Runtime tests remain deferred.

- Browser integration 0.63.45 adds per-link save folders and descriptions under More properties. The desktop validates custom destinations and advertises support; older installations reject these edits rather than silently ignoring them. Existing duplicate records retain their saved metadata. Build/source checks only, with runtime testing deferred.

- Browser integration 0.63.44 adds editable output filenames to the link-import page and defaults imported links to Download later. Empty names retain server naming, invalid Windows names are rejected before handoff, and submitting locks the selection against duplicate sends. Source checks only; not installed or browser-tested.

- Browser integration 0.63.43 shows an OFF toolbar badge and an explanatory tooltip when capture is disabled on a tab. Re-enabling restores the shared error badge; tab activation, navigation and worker startup refresh the indicator without delaying handoff. Source checks only; not installed or browser-tested.

- Unattended queue downloads no longer open automatic completion or login dialogs. Authentication failures remain available for manual recovery; individual resumes and explicitly opened completed files retain their interactive behavior.


- Scheduler now uses native time controls with seconds, long-date formatting, reference keyboard shortcuts and corrected tab order. Daily, one-time and independent stop boundaries retain second precision.
- Queue lists show Time left and refresh filename, size and progress while open. Membership and order updates preserve surviving selections and the top visible file; drag operations are not interrupted by a refresh.
- Queue menus show named Start/Stop actions. Idle queues disable Stop in the menu and Scheduler, while active or waiting runs remain stoppable.
- Sounds adds independently configurable Queue started and Queue stopped/finished events. Additional files and retries within a run do not repeat the start notification.
- Scheduler Apply reflects edited values. Unchanged close/navigation avoids writes; edited saves merge changed fields with current queue data, and new queue creation rechecks current names to avoid replacing a concurrently created queue.
- Media confirmation timing excludes the user's wait before Start, and interrupted media records clear stale completion phases and final-size markers while retaining reusable cached tracks.

The Scheduler save-state candidate includes the Scheduler, queue-menu, sound and
media timing changes above. Its installation was verified on October 27;
interactive acceptance is deferred. The newer unattended-dialog build is staged
but not installed. Extension 0.63.45 and its matching import-properties desktop build are a newer staged batch, also not installed. Build and source hashes for the installed candidate are recorded in
[the candidate record](docs/validation/scheduler-save-state-build-20261027.json).


- Updater hardening after review: the update prompt's button actions no longer call through a closure that had gone out of scope; Help > Check for Update answers while a startup check is still running; a failed check is not recorded as done; the update job is marked in its data, so a setup finished after a restart or offered again still meets the gate instead of downloading a second copy; the setup is copied to a private folder under `%LOCALAPPDATA%\UDM\updates` and verified there, off the UI thread; release asset paths must be exactly tag/file; the prompt shows the window when UDM started in the tray; the manifest tools resolve relative paths against the PowerShell location.
- Code signing: Azure Trusted Signing is a third credential for `package.ps1` and `installer/sign.ps1` (`-TrustedSigningMetadata`, optional `-TrustedSigningDlib`): signtool signs through Microsoft's dlib, the timestamp defaults to Microsoft's authority, and signatures are verified by a valid chain from the "Microsoft ID Verified" CAs since the certificate changes per signing. `docs/release-signing.md` lists the setup for this and for a CA-issued certificate step by step. `build-setup.ps1` no longer loses `-SignToolPath` and `-NoTimestamp` when it dot-sources `sign.ps1`.
- Updater: UDM checks a signed `latest.json` on the release host at startup (at most once per 20 hours, with an opt-out and a per-version skip) and from Help > Check for Update, verifies it with the ECDSA P-256 key built in, downloads the setup through its own transfer with the manifest's SHA-256 as the expected checksum, and starts it only when it also carries a trusted Authenticode signature. `tools/update-key.ps1` creates the key and writes the public half into the build; `tools/publish-manifest.ps1` signs the manifest for a release (`docs/release-signing.md`). Staged POST bodies no longer expire after 3.6 seconds (the hour was counted in milliseconds).
- Code signing is wired into packaging: `package.ps1 -SignThumbprint <sha1>` or `-SignPfxPath <file>` signs UDM's executables and the network helper with signtool (SHA-256, RFC 3161 timestamp) and has Inno Setup sign the setup and uninstaller; `-SignDryRun` reports the tool, certificate and files without changing anything. The vendor's WinDivert files are never re-signed. `installer/test-signing.ps1` proves the pipeline with a temporary self-signed certificate (`docs/release-signing.md`).
- Publishing a download to another volume copies it with buffered 1 MiB writes instead of CopyFileW, which copies large files unbuffered and waits on the disk: 384 MiB onto a hard disk now publishes in 2.6 s instead of 3.7 to 7 s, the same as IDM's assembly, and the copy is flushed after publication (`docs/speed-benchmark-2026-10-23.md`).
- The Size column shows a media download's stream length while its byte size is unknown, as IDM does ("2 hour(s) 3 min", "3 min 20 sec", "20 sec"); HLS/DASH captures record the plan's total length, SABR records already carried theirs, and live recordings stay blank.
- Speed-blind tail splits need a half of at least a 64th of the file per connection (256 KiB to 2 MiB) instead of always 256 KiB: on a shared 3 MB/s link a 140 MB file saw 36 tail splits, each a new TLS connection plus wasted in-flight bytes; with the change both public benchmark files download level with IDM within a second per run (`docs/speed-benchmark-2026-10-23.md`).
- Downloads planned with more than one connection negotiate HTTP/2 or HTTP/1.1 rather than HTTP/3: one QUIC connection carried every range and got a single connection's share of a shared link (Alpine over a 4 MB/s link: 143 to 156 s against IDM's 63 to 68 s; 64 s once HTTP/3 was left to single-connection transfers). The record now carries the negotiated protocol, and the benchmark harness takes `--udm-settings` to merge settings into its private catalog.
- View > Toolbar follows IDM's menu: Customize..., Large/Small Buttons, Look for new toolbars (opens the toolbar folder), Neon and a new 3D Style look that draws UDM's own icons on raised tiles; Tip of the Day gains IDM's picture panel and "Did you know..." heading; the Batch download dialog is verified control by control against IDM's capture at 125 % (`ToolbarUiTests`, `BatchDialogUiTests`).
- Each connection issues its next 256 KiB read before writing the previous buffer, so a single connection no longer alternates network and disk: the sequential loopback case went from 3.6 s to 1.4 to 2.3 s against IDM's 2.0 to 3.4 s (`docs/speed-benchmark-2026-10-23.md`).
- Downloads complete the moment the last byte is written: the trailing hash thread stops when the workers return, the sparse attribute is cleared after publication (2 to 12 s before on a full or mechanical disk), and when no expected checksum must be verified the file is published first and its SHA-256 recorded a moment later. A kept-alive connection the server closed is retried once at once instead of being treated as an outage, and files are planned as three 4 to 32 MiB chunks per connection instead of 8 MiB chunks. Against IDM 6.43 on the same loopback origin UDM now finishes within 0.2 s of its last byte on every case and is faster on five of seven (`docs/speed-benchmark-2026-10-23.md`).
- Content recognition grows by 20 signatures written from public specifications and checked against the libmagic test corpus (`docs/content-signatures-libmagic-2026-10-23.md`): PDF behind a UTF-8 byte order mark and SVG documents are no longer taken for plain text (which the web-page stop treats as an error page), plus animated PNG, Zstandard legacy frames, Hangul Word Processor (3.x, 5.x, HWPX), DSDIFF, FIT, GEDCOM, Group Policy .pol, UF2, binary G-code, Android vdex, xclbin, Xilinx .bit, Keyman .kmx, Tarantool logs, binary OpenPGP keys and ext2/3/4 images. `Udm.NativeTests.exe --sniff-corpus <folder>` checks the whole corpus (`tools/fetch-libmagic-corpus.py`).
- Explorer integration: the installer registers a "Download with UDM" verb on Internet Shortcut (.url) files; it runs `UDM.exe --shortcut "<file>"`, which reads the shortcut's address (web and FTP only) and adds it like `--add`. The development installer registers the same verb for the current user (`tests/explorer-verb.test.py`, launch checks in the native suite).
- Capture POST bodies above 4 MiB (up to 64 MiB) from the browser: the extension streams the bytes in ordered base64 chunks over the existing native host pipe (`post-body-begin` / `post-body-chunk` / `post-body-finish`), the desktop verifies size and SHA-256, seals the body DPAPI-protected under `requests/` outside the catalog, and the download record keeps a small reference. Bytes are never treated as text, so binary and non-UTF-8 producers round-trip exactly (loopback check with every byte value). Desktops without this support keep such downloads in the browser without pausing them.
- Draw UDM's own 40x40 page icons in the Configuration dialog (browser window, document, folder, tray, nodes, shield, key, modem, speaker) in place of the Windows stock icons; they follow the light and dark theme and scale with DPI (`OptionsTabsUiTests`).
- Rebuild the Site Grabber as IDM's five-step wizard: "UDM Site Grabber. Step N --- Set start page / Save files to / Set explorer filters / Set file filters / Explore and download" on IDM's 717x475 shell with a Project/Options/Actions/Help menu bar, the left picture panel and Back/Next/Close at IDM's positions; step 1 follows IDM's captured layout with the Advanced >> manual-login area, and the exploration result uses IDM's 905x639 result layout with its actions in the Actions menu (`GrabberWizardUiTests`). Dialogs with a menu bar now keep their full client size.
- Match IDM's About dialog (461x503 layout, build date, licensing group, legal notice, Home Site and Support Team lines), its two-line Download complete text with the file type icon, and a Download limits node in the Scheduler tree that edits the shared download quota. tools/idm-research/dump-dialogs.py dumps RT_DIALOG templates from a PE for such comparisons.

- Take the disk write of each received block out of the catalog lock (eight connections no longer queue behind one write, and the window no longer waits on them), probe every 500 ms for 20 s after a connection drops on a download that was receiving so a short outage ends the moment the network is back (94 ms in the new fixture check, against a 1, 2, 4 s backoff), size each read to at most a quarter second of the speed limit when one applies (so limited downloads pace smoothly instead of stalling after one read), and write the periodic catalog save outside the manager lock ([notes](docs/speed-benchmark-2026-10-23.md)).
- Benchmark UDM against IDM 6.43 on identical loopback and public downloads (`benchmarks/speed`, [results](docs/speed-benchmark-2026-10-18.md)). Fix the shortfalls it found: a connection that breaks after delivering data is reopened at once and a megabyte of progress restores the retry budget, so servers that reset connections or short outages no longer fail the download; a worker whose connection is refused while others are receiving hands its range to them instead of failing the download; parallel ranges use one HTTP/1.1 connection each instead of sharing one HTTP/2 connection (2.3 times faster on a CDN); 256 KiB reads; the assembled file is hashed while it is written.
- Write every HTTP range in place into one preallocated output file instead of one part file per range, hash the file while it downloads (ranges are claimed front to back in 4 to 8 MiB chunks) and publish it by rename. The finish phase after the last byte of a 384 MiB file drops from 2.6 to 3.8 s to 0.1 to 0.9 s; existing part files are folded into the single file on the next start. The resume offset after a crash is the last saved catalog (saved every 1.5 s while downloading).
- Negotiate HTTP/3 where Windows and the server support it (Options › Connection, "Use HTTP/3 when supported by Windows and the server", on by default). A transfer that fails during an HTTP/3 exchange retries once without HTTP/3 before the existing HTTP/2 to HTTP/1.1 step. Explicit proxy routes through the bundled curl stay on HTTP/2.
- Recognize many more file formats from their content: disk images, fonts, e-books, raw camera and texture images, lossless and tracker audio, broadcast video, data and capture files, further app packages inside ZIP, and the producer of compound documents (MSI, Word, Excel, PowerPoint, Outlook, Visio). A download without a usable extension gains the right one for these formats, and a web page or text notice served under an image, font, disk-image or other binary-only name now stops the download like one served under an archive name. See [backend gap closing](docs/idm-backend-gap-closing-2026-10-16.md).

- Save a captured rendered starting page in Site Grabber offline ZIP archives, preserving runtime-created HTML and rewriting downloaded assets and child-page links. Negotiate HTML capture with the running desktop, protect stored captures, and report oversized pages explicitly. Browser integration 0.63.30.

- Clear stale download-list tooltip text when its row disappears after filtering or a list update.

- Preserve the rendered-page capture identity in Site Grabber checkpoints when browser cookies change during exploration. Persist crawler-managed cookie updates with the capture association, restoring both on save failure; manual session and filter changes still require recapture.

- Preserve a verified receipt for fully received ordinary-file prefetch. Download Later can resume from saved parts without spending a one-use URL again; missing or modified saved bytes are rejected before publication.

- Keep ordinary-file prefetch running when Start is clicked with unchanged login details, including filename, folder and drive changes. Assemble verified parts beside the confirmed destination without another HTTP request; invalid destinations and failed confirmation saves keep publication gated.


- Show server-provided filenames in Download File Info with an explicit Use server name action. Preserve edited names and selected folders, keep changes as a draft until confirmation, and reject replacement or conflicting destinations. Metadata lookup uses HEAD without consuming a single-use download link; with prefetch enabled, the suggestion comes from the existing transfer response without another request.

- Decode legacy ISO-8859-1 server filenames in Site Grabber and original-response capture. Extended filenames take precedence over plain fallback names; malformed escapes, embedded NULs and unsupported encodings retain the fallback.
- Validate Scheduler numeric fields completely before saving, rejecting partial numbers, decimals, signs and out-of-range values while preserving blank connection retries as the Options default.

## 1.0.4 / browser 0.64.49 (recorded 0.63.20)

- [Release notes](docs/release-v1.0.4.md).
- Extend browser proxy handling to evaluate inline PAC scripts with compatible helper functions, deterministic route selection, cancellation, and bounded execution. Browser and desktop keep the selected proxy route consistent through native handoff.
- Harden browser/native handshakes and proxy diagnostics, including safe request context handling and improved validation for Chromium and Firefox integration.
- Improve desktop list behavior: clarify columns, preserve stream sorting, and refresh visible metadata predictably.
- Recognize Unicode and compound file-name patterns more reliably during content inspection.

## 1.0.3 / browser 0.64.46 (recorded 0.63.17)

- [Release notes](docs/release-v1.0.3.md).
- Allow a browser-captured media download to be confirmed while its tracks are still being fetched. A folder or file-name change is saved transactionally, keeps the existing requests alive, stages assembly in its original working location, and publishes to the confirmed destination.
- Show the verified final media size in the main download list while the confirmation dialog is open, including sorting by that value, without re-entering dialog or capture presentation handling.
- Preserve the first video or audio transfer error when the companion track is cancelled, so the user sees the meaningful HTTP or transport failure rather than a secondary cancellation message.

## 1.0.2 / browser 0.64.45 (recorded 0.63.16)

- [Release notes](docs/release-v1.0.2.md).
- Measure the actual final media container before the download confirmation dialog opens, including audio-only output. The dialog reports a verified size rather than an estimate.
- Let Start publish the already-verified staged media output without re-downloading or remuxing it. Folder, file name, category, description, and queue can still be changed at confirmation time; transfer-affecting changes remain protected.
- Make media publication recoverable: preserve the verified source and a small journal if saving completion state is interrupted, then reconcile the completed download safely on the next recovery pass. Cross-volume copies verify bytes before publication and honor cancellation.
- Improve browser handoff timing and make capture registration resilient when several downloads are captured at once.

## 1.0.1 / browser 0.64.43 (recorded 0.63.8)

- [Release notes](docs/release-v1.0.1.md).
- Build the bundled curl library with Windows SSPI and NTLM enabled, so configured proxy credentials can complete NTLM authentication and the existing Windows-login path remains available for intranet routes.
- Qualify proxy auto-configuration scripts with a loopback PAC fixture, including ordered alternatives, DIRECT and SOCKS results, fallback, and unreachable-route behavior.
- Show exact media sizes only when the selected stream or container reports a trusted content length. Browser and native handoff now preserve that reported size for video and audio selections without substituting a bitrate estimate.
- Improve media preview responsiveness and receipt error recovery while keeping stale, foreign, expired, or untrusted browser captures out of a download handoff.

## 1.0.0 / browser 0.64.37 (recorded 0.63.8)

- [Release notes](docs/release-v1.0.0.md).
- Promote the content-recognition and browser/native integration work from 0.92.0 to the first 1.x release.
- Check the downloaded content before publishing. A web page or text notice served in place of an archive, installer, document, or media file now stops the download with its data kept, and queues do not retry it automatically. Resuming such a download asks whether to save the received content anyway. When web page text is found in front of an otherwise recognized file, the download pauses and resuming asks whether to remove that text or keep the data as received.
- Add a missing or script-style extension (for example `download.php`) from the recognized content before publication. Explicit extensions are never changed. Both checks can be turned off in Options › File Types.
- Check the first bytes of a download while it is still running, so a web page served for a file stops after about 5 KB instead of after the whole body.
- Retry once with HTTP/1.1 when an HTTP/2 transfer fails mid-exchange (stream reset, GOAWAY, transport error); the download keeps HTTP/1.1 afterwards. See [backend gap closing](docs/idm-backend-gap-closing-2026-10-16.md).
- Support Windows login (Negotiate/NTLM) for servers and proxies. Saved site logins are offered for any challenge scheme, intranet servers can receive the current Windows login (Options › Logins, on by default), and a configured proxy login answers Basic, Digest and NTLM challenges (the vendored curl is now built with Windows SSPI), while a proxy asking for Windows login without a configured login receives the current user's login on intranet routes. A loopback authenticating proxy covers Basic, Digest and Negotiate in the native suite. A loopback proxy auto-configuration script qualifies PAC route order, DIRECT and SOCKS directives, failover to the next alternative and the absence of a direct fallback when scripts or proxies are unreachable.
- Reattach a new desktop process to a running network helper. The desktop records its helper; an active helper whose desktop exits waits up to two minutes for a same-user replacement desktop, and a new desktop adopts it instead of elevating another helper, keeping capture objects and response writers.
- Site capture rules with the D flag now also apply to plain addresses routed through an HTTPS proxy, matching the recovered IDM semantics (an https address or a TLS proxy route). Media captured from the browser video panel drops a detected web page prefix automatically, as IDM does for its video downloads; other downloads keep the prompt.
- Qualify FTP against a loopback server (EPSV/PASV, EPRT/PORT, REST resume, SIZE/MDTM, logins, HTTP CONNECT) and add FTP gateway proxy types (USER user@host, SITE host, OPEN host; Options › Proxy). Active FTP works through a gateway; CONNECT and SOCKS routes still require passive mode.
- Add Windows download automation: `UDM.LinkTransmitter` (COM, per-user registration by the installer or `Udm.Automation.exe --register`) with `SendLinkToUDM`, `SendLinkToUDM2` and `SendLinksArray`. Single links follow flags 1 (silent) and 2 (queue only); link arrays are stored encrypted and reviewed in a "Download all links with UDM" dialog. A running UDM hosts the class object itself, so callers reach it without any registry lookup; the installer registers `Udm.Automation.exe` machine-wide for cold starts, and that server starts the desktop when it is not running.
- Site capture rules now also act on responses: V/A/C stream rules place a matching reply on the video panel with the rule's media type, S rules hand matching background requests (XHR, media, objects) to UDM, and the diagnostics page lists the last 50 rule decisions (host, path shape and query names only) with a "Save report as file" button.
- Add site capture rules (Options › File Types › Site rules). One rule per line, `[FLAGS][#EXT]!PATTERN`, decides before the file-type list whether a browser download is ignored, captured outright, or captured through a file name or extension taken from the address. No rules ship by default.

## Unreleased

- Delivery builds now offer a production-only target set, copy all native dependency notices, and package isolated release/browser directories. Installer version checks now include COM automation; the MIME-data notice is shipped with setup. Runtime qualification remains deferred.

- Native metadata previews now resolve final-response URL filenames and apply specific MIME suffixes to generic names or URL-only script endpoints, sharing the extension's licensed MIME data. Specific names and the extension-correction preference retain precedence. Runtime tests deferred.

- Native imports add Hide HTML files and Hide repeated files. Hidden selections are preserved and counted; metadata-driven filtering and per-file edits retain original row identity. Runtime tests deferred.

- Batch output patterns now use server filename extensions from metadata. The first non-HTML extension updates the shared pattern once; mixed files retain per-row extensions and explicit per-file edits remain authoritative. Runtime tests deferred.

- Native imports now preview server-reported size, MIME type and server filename suggestions in the background. Four bounded metadata workers keep the dialog responsive; URL/authorization changes cancel stale lookup generations. Size and Type columns sort with unknown values last. Runtime tests deferred.

- Generated number/letter batches now offer a live output filename pattern. Every asterisk uses the row's original sequence token, even after sorting; per-file names override the pattern. Selected names are validated before admission. Runtime tests deferred.

- Native batch generation now accepts descending numbers and letters, matching the retained IDM sequence-generation evidence. Zero padding, inclusive endpoints and the 1,000-URL numeric limit are preserved. Runtime tests deferred.

- Native batch/text imports now sort by File name and Address with direction arrows, preserving per-link properties, checkboxes and already-added tracking across reordering. Admission follows the displayed order. Runtime tests deferred.

## Native 0.87.0 / browser 0.64.31 (recorded 0.63.8)

- [Detailed changes since 0.85.0 RC3](docs/changelog-rc3-to-087.md) and [release notes and limits](docs/release-v0.87.0.md).
- Capture rendered Site Grabber links through Edge and native messaging with document identity, protected snapshots, durable receipts, and lost-reply reconciliation.
- Preserve original browser responses through durable native admission and recover interrupted captures without creating a second request.
- Improve Chromium proxy bypasses, mapped-address native transport, media/caption recovery, and isolated installer repair/uninstall behavior.
- Advance desktop and installer versions to 0.87.0 and browser integration to 0.63.8, including safer same-document media navigation.

### Native 0.85.0 RC1 / browser 0.64.21 (recorded 0.63.0)

- [Changes since 0.84.0 RC3](docs/changelog-rc3-to-085-rc1.md) and [RC1 package verification](docs/release-v0.85.0-rc.1.md).

- Retain the first full HTTP response for single-use download links while preserving validator checks on recovery. Controlled live cases passed 8/8 and the native suite passed 2,589/2,589.
- Support Digest authentication for explicit HTTP proxies and keep proxy challenges separate from origin authentication.
- Advance native, browser, and installer version declarations for the RC1 package.

## Native 0.84.80 RC3 (recorded desktop 0.84.0) / browser 0.63.34 (recorded 0.62.13)

- [Changes since RC2](docs/changelog-rc2-to-rc3.md) and [RC3 package verification](docs/release-v0.84.0-rc.3.md).
- Keep HLS captions synchronized across distant timestamp anchors and preserve valid empty subtitle tracks.
- Support larger recorded HLS/DASH selections and durable per-segment adaptive checkpoints.
- Preserve pipelined and body-framed HTTP requests with their original client during network takeover decisions.
- Expose the Properties source link to Windows accessibility.

## Native 0.84.79 RC2 (recorded desktop 0.84.0) / browser 0.63.33 (recorded 0.62.13)

- [Changes since RC1](docs/changelog-rc1-to-20261008-push.md) and [RC2 package verification](docs/release-v0.84.0-rc.2.md). This prerelease is a source and installer publication; the installer has not been executed or installed on the personal machine.

- Honor Windows static-proxy bypasses and configured PAC URLs for FTP; unresolved WPAD and unsupported static proxy routes remain fail-closed.
- Expand captured browser POST bodies to 4 MiB with an 8 MiB native request frame; preserve exact-byte and over-limit rejection checks.
- Reject ranged HTTP responses that omit the validator established by the download probe.

- Wait for a saved completion before executing CLI /q exit or /h dial-up hang-up; a failed catalog save retains both pending actions for retry.

- License UDM's original code and documentation under Apache 2.0; retain third-party notices and include the project license/notices in future packages.
- Version progress-dialog reference metadata and sanitized validation/release records, retaining the full-suite failure and its passing focused rerun.

- Make individual Pause transactional: a failed catalog save preserves the job, capture prompt, scheduler state, and running transfer. Cancel the worker only after the pause decision is saved.
- Avoid catalog writes when stopping an idle completed download while retaining pending-synchronization cancellation and scanner-wait handling.

- Keep the completion dialog drag area icon-only, retain its Unicode filename as the control name, and support icon rendering in window previews.
- Add a completion-dialog test runner and protect its optional warning observer flag across threads.

## Native 0.84.71 RC1 (recorded desktop 0.84.0) / browser 0.63.21 (recorded 0.62.8)

- Consolidate queue retry, synchronization, recovery, backup and restore behavior with focused backend checks.
- Add `/a` queue-only downloads and `/s` queue start, including forwarding to an existing application instance.
- Extend browser capture recovery, multipart handling, file recognition and HLS/DASH media selection.
- Improve native menus, properties, progress controls, keyboard behavior and accessibility.
- Add installer migration, ownership and transaction safeguards, plus pinned HTTP/2 transport dependencies.
- See [candidate acceptance and delivery status](docs/current-staged-candidate.md) for the exact tested, packaged and installed revisions. This entry records source changes; it does not announce a qualified release.

## Native 0.78.4 (recorded desktop 0.78.0) / browser 0.53.40 (recorded 0.53.1) — video preparation deadlines

- Stop stalled cross-site video preparation before the panel timeout and prevent late native submission.
- Guard direct-media handoff after asynchronous settings/credential work; cancel HLS/DASH preparation when expired.
- Keep unknown native acceptance distinct from a request never sent, without automatic retry.
- [434 targeted and 35 actual Edge checks, decoded outputs and explicit limits](docs/parity-browser-0.53.1.md).

## Native 0.78.3 (recorded desktop 0.78.0) / browser 0.53.39 (recorded 0.53.0) — Firefox handoff reliability

- Keep Firefox's original response running until UDM has durably prepared the request.
- Preserve refused/unavailable downloads without pause/resume or a second request.
- Recover prepared/committed ownership after lost acknowledgements and actual background reloads.
- Preserve browser completion and user pause/cancel decisions during preparation.
- [386 targeted and 47 live checks, baseline failures and remaining limits](docs/parity-browser-0.53.0.md).
- Native 0.78 and earlier browser changes: [release](docs/parity-0.78.0.md), [manual Firefox recovery](docs/parity-browser-0.52.2.md), [current gap register](docs/idm-research-current-status-2026-09-29.md).

## 0.74.0 / browser 0.47.39 (recorded 0.47.1) — repeated-cancel exclusions

- Offer site or literal-address exclusions after two consecutive cancelled automatic captures.
- Add suppression/re-enable, File Types Add/Delete, replay protection and transactional settings updates.
- Preserve literal asterisks, long exact URLs and IPv6 in both extension bundles.
- Preserve multipart.js and ordered dependencies when regenerating Firefox.
- Correct network-helper signature wording.
- [2,821 checks and qualification limits](docs/parity-0.74.0.md).

## 0.73.0 / browser 0.47.38 (recorded 0.47.0) — queue startup and export scopes

- Persist per-queue Start download on UDM startup independently of the current Stop state.
- Prepare opted-in queues once at launch, with rollback if saving fails.
- Add all, selected and named/all queue export scopes with retained per-file checkboxes.
- Reject stale/duplicate export records and empty export sets before writing.
- Reject overlong instance tags instead of silently connecting to the main app.
- [1,938 checks and qualification limits](docs/parity-0.73.0.md).

## 0.72.0 / browser 0.47.37 (recorded 0.47.0) — native toolbar customization

- Replace individual toolbar buttons with a native toolbar and queue dropdowns.
- Add Available/Current command editing, repeated separators, Move Up/Down and Reset.
- Migrate existing preferences and save customized command layouts.
- Support bounded .tbi skin strips, normal/hot/disabled states and high-DPI image variants.
- Preserve UDM artwork and fall back safely if a saved skin is missing.
- [1,917 passing checks, development corrections and remaining GUI limits](docs/parity-0.72.0.md).

## 0.71.0 / browser 0.47.36 (recorded 0.47.0) — remote ZIP preview

- Add Preview to Download File Info for ordinary ZIP links before download.
- Read bounded HTTP/HTTPS/FTP ZIP and ZIP64 directories using existing login, browser sessions and proxy routes.
- Handle small no-range responses, cancellation, timeouts and changed archive responses.
- Show names, sizes and encryption flags in a virtual native list; retain local preview.
- Correct toolbar/dial-up scope in the comparison inventory.
- [1,887 passing checks, initial failures and limits](docs/parity-0.71.0.md).

## 0.70.0 / browser 0.47.35 (recorded 0.47.0) — offline website folders

- Enable original subfolders for offline ZIP projects with an explicit destination folder.
- Preserve nested pages and stylesheets, rewrite relative links, retain a root entry page, and separate colliding/query/external resource names.
- Reuse redirected resources, validate archive names and content, and retain verified cache reuse on pause/resume.
- Correct the About dialog's stale version label.
- [1,848 passing checks and remaining limitations](docs/parity-0.70.0.md).

## Browser 0.47.34 (recorded 0.47.0) / native 0.69.2 (recorded desktop 0.69.0) — multipart text-form downloads

- Preserve browser-generated multipart text fields, original order, repeated names, encoding and submitter overrides.
- Match the ephemeral submission snapshot to the observed request and retain verified same-origin POST redirects.
- Keep unsupported file/Blob or oversized bodies in the browser.
- [846 checks and actual Edge byte comparisons](docs/parity-browser-0.47.0.md).

## Browser 0.46.34 (recorded 0.46.0) / native 0.69.1 (recorded desktop 0.69.0) — responsive video formats

- Bind panel format reads to the browser-issued document identity, avoiding redundant tab lookups and document-idle waits.
- Preserve strict current-tab checks for download actions and legacy browser paths.
- Complete a public Edge 360p video pause, link refresh, resume and full decode on one history record.
- [807 current checks, measured results and limits](docs/parity-browser-0.46.0.md).

## 0.69.0 / browser 0.45.34 (recorded 0.45.0) — direct video recovery

- Refresh an unfinished direct video/audio pair from the browser panel on its existing record.
- Validate partial-track HTTP identity and completed-track integrity before reuse; retain saved data on rejection or cancellation.
- Open the original page automatically when address refresh begins.
- Check middle and final playback bytes as well as the first byte before accepting a native player pair.
- [1,785 passing checks, public-test failures and remaining limitations](docs/parity-0.69.0.md).

## v0.68.0 — combined 0.58 through 0.68 update (28 September 2026)

This published release includes the previously untagged 0.58.0 through 0.68.0 milestones. The v0.68.0 tag is the complete source snapshot; the entries below preserve the recorded detail for every included version.

- **0.58–0.64:** expanded Site Grabber destinations, filters, link handling, website workspaces, browser sign-in, login persistence, and session updates.
- **0.65–0.67:** signed-in browser downloads, Edge proxy handoff, encrypted proxy/session handling, and ordered proxy alternatives.
- **0.68:** refresh an interrupted recorded HLS or static DASH session from a browser capture while preserving the original download, history, and verified cache.

See [0.58](docs/parity-0.58.0.md), [0.59](docs/parity-0.59.0.md), [0.60](docs/parity-0.60.0.md), [0.61](docs/parity-0.61.0.md), [0.62](docs/parity-0.62.0.md), [0.63](docs/parity-0.63.0.md), [0.64](docs/parity-0.64.0.md), [0.65](docs/parity-0.65.0.md), [0.66](docs/parity-0.66.0.md), [0.67](docs/parity-0.67.0.md), and [0.68](docs/parity-0.68.0.md) for behavior and stated limits.

## 0.68.0 / browser 0.45.33 (recorded 0.45.0) — recorded-video link recovery

- Refresh interrupted HLS/static DASH sessions from the browser panel without creating another download.
- Preserve filename, history and cache; review fresh credentials before applying them.
- Verify retained segment bytes against the fresh source before fetching missing segments; preserve cache on failure, pause or restart.
- Reject changed selections, foreign-origin credentials and incompatible segment layouts.
- [1,739 passing checks and remaining limitations](docs/parity-0.68.0.md).

## 0.67.0 / browser 0.45.32 (recorded 0.45.0) — extended Edge proxy handoff

- Preserve encrypted HTTPS proxies and Edge SOCKS4 local DNS through browser capture and native download.
- Keep proxy credentials separated by encryption protocol; preserve existing native compatibility.
- Preserve ordered PAC alternatives for connection failures without replaying POST or bypassing TLS/authentication errors.
- [2,586 passing checks and remaining limitations](docs/parity-0.67.0.md).

## 0.66.0 / browser 0.44.32 (recorded 0.44.0) — Edge proxy handoff

- Captured fixed/direct Edge routes, per-protocol endpoints, bypass rules, and explicit-link proxy selection.
- Corrected explicit HTTP/SOCKS proxy routing for loopback downloads with a bounded native Schannel transport.
- Added native compatibility checks; retained browser ownership for unresolved PAC routes.
- [2,509 checks and remaining limitations](docs/parity-0.66.0.md).

## 0.65.0 / browser 0.42.32 (recorded 0.42.0) — Signed-in browser downloads

- Managed, exact-origin cookie sessions for ordinary browser file captures with consent, store/partition checks, encrypted updates, duplicate handling and link refresh.
- Removed default-profile cookie guessing and fixed UTF-8 cookie-header budget accounting.
- [Acceptance and remaining limitations](docs/parity-0.65.0.md).

## 0.64.0 / browser integration 0.41.32 (recorded 0.41.0) — Website session updates

- Apply website Set-Cookie responses to signed-in Site Grabber sessions, including redirects, expiry, deletion, domain/path scopes and separate partitioned identities.
- Share updates across a transfer's parallel requests and protect saved sessions with Windows account encryption.
- Retain updated cookies in exploration checkpoints, File Info lookups, queued file transfers, redownload, synchronization and offline ZIP requests. Reject late updates after browser recapture; roll back failed catalog saves.
- Reproduce the previous release's HTTP 401 failure with an actual Chrome sign-in and a server that rotates its session. Verify the replacement-cookie workflow in Chrome, Edge and Firefox.
- Browser source and signed network runtime remain unchanged. Public-site coverage, rendered exploration, cross-job session sharing and full IDM parity remain open.

## 0.63.0 / browser integration 0.41.31 (recorded 0.41.0) — Site Grabber browser sign-in

- Add manual browser sign-in, a login-page field, logout exclusions and an explicit project selector in the extension popup.
- Save scoped browser cookie snapshots with Windows protection; carry them into exploration, metadata, queued files, redownload, synchronization and offline ZIP requests.
- Respect tab stores/containers, matching partitions, cookie paths, Secure and expiration; recalculate scope through redirects.
- Handle stale wizard saves, corrupt sessions, expiring tickets, replay protection and catalog/template credential boundaries.
- 2,381 checks pass, including real Chrome, Edge and Firefox sign-in/popup/native workflows. Rendered crawling, cookie rotation and full IDM parity remain open.

## 0.62.0 / browser integration 0.40.31 (recorded 0.40.0) — Site Grabber login and descriptions

- Add project-specific HTTP Basic/Digest login with Windows-protected persistence, exact-origin request scoping, editable credential recovery and save rollback.
- Carry login details into exploration, parallel metadata checks, queued downloads and offline ZIP archives.
- Extract static link text and image alt text into collected-file properties, a Description column and new download records; retain bounded Unicode descriptions across scan continuation.
- Preserve edited metadata on existing downloads; templates exclude project credentials. Browser integration remains 0.40.0.
- 2,276 checks pass, including 45 new native checks, 40 real Chrome/Edge integration scenarios and 33 offline website checks. Native visual acceptance and full IDM parity remain open.

## Browser integration 0.40.30 (recorded 0.40.0) / native 0.61.0 — Capture lifecycle

- Fix a reproducible late-navigation cleanup race that erased newly detected playlists.
- Retire previous document/frame captures, authenticated request context and SABR sessions while preserving fresh captures; serialize menu-offer storage with cleanup.
- Persist bounded document identity across service-worker restarts and reject delayed work after tab closure.
- Add webNavigation permission; reload the extension and refresh video tabs to activate.
- 687 fresh checks pass, including 25 new lifecycle checks and 20 real extension/native scenarios in each of Chrome and Edge. Native engine and signed network runtime unchanged; full IDM parity remains unestablished.

## 0.61.0 / browser integration 0.39.30 (recorded 0.39.0) — Offline website links

- Add a Complete website preset and conversion of HTML/CSS references to actual downloaded destinations, including original subfolders, numbered filenames, redirects and CSS imports.
- Retain original pages for repeat conversion; preserve external edits and unsupported encodings. Recover interrupted page/catalog publication with verified journals and rollback.
- Support conversion while queues are disabled, Stop/retry, and synchronization using original server lengths. Preserve project association on updated files.
- 2,198 checks pass, including 53 new native checks and 33 real offline-site checks in Chrome/Edge. Native visual acceptance, public-site coverage and full IDM parity remain open.

## 0.60.0 / browser integration 0.39.29 (recorded 0.39.0) — Live Site Grabber workspace

- Download matching files during exploration with bounded concurrency and incremental persistence. Start/stop checked downloads without enabling unrelated queues.
- Add reusable templates, folder/referring-page trees, editable collected filenames, file/folder actions, address copying and live statistics.
- Preserve edits across scan checkpoints, transactionally remap category/template destinations, and stop on persistence failures.
- 2,112 checks pass, including 56 new native checks and 32 Chrome/Edge integration scenarios. Native visual acceptance and full IDM parity remain open.

## 0.59.0 / browser integration 0.39.28 (recorded 0.39.0) — Advanced Site Grabber

- Add separate Explorer and File Filters pages with include/exclude patterns, independent site/path rules, depth, main-domain scope, size bounds and deterministic duplicate hiding.
- Add concurrent metadata lookup, server filenames, result properties, referring-page context, Stop with retained results and saved-scan continuation.
- Use an included MPL-2.0 Public Suffix List snapshot for domain boundaries; preserve legacy saved projects and category/destination behavior.
- 2,056 checks pass, including 69 new native checks and 32 Chrome/Edge integration scenarios. Native visual acceptance, broader video/driver equivalence and full IDM parity remain open.

## 0.58.0 / browser integration 0.39.27 (recorded 0.39.0) — Grabber destinations and reliable batches

- Add category-based, selected-category and explicit-folder destinations, with optional original URL subfolders for collected files.
- Add a dedicated Save to step and per-file destination/added-state previews; retain old project behavior and category choices across category renames/deletion.
- Save each selected batch and project atomically, with rollback on invalid paths or storage failure; reject traversal and redirected subfolders and preserve pause/restart/resume.
- 1,987 checks pass, including real HTTP folder downloads and 32 Chrome/Edge extension/native scenarios. Native visual acceptance and full IDM parity remain open.

## v0.57.0 — combined 0.41 through 0.57 update (28 September 2026)

This published release includes all previously untagged milestones from 0.41.0 through 0.57.0. The v0.57.0 tag is the complete source snapshot; the linked notes retain the recorded details for each included milestone.

- **0.41–0.43:** Firefox proxy-route handoff, indexed DASH resource binding, and selected YouTube audio-only streaming.
- **0.50–0.53:** media-session recovery, download and completion dialogs, configuration/queue controls, plus PAC and transport configuration.
- **0.54–0.56:** browser customization, quota workflows, and safer duplicate/overwrite behavior.
- **0.57:** exposed clear live HLS recording with Stop and save, pause/recovery, and native parallel capture.

See [0.41](docs/parity-0.41.0.md), [0.42](docs/parity-0.42.0.md), [0.43](docs/parity-0.43.0.md), [0.50](docs/parity-0.50.0.md), [0.51](docs/parity-0.51.0.md), [0.52](docs/parity-0.52.0.md), [0.53](docs/parity-0.53.0.md), [0.54](docs/parity-0.54.0.md), [0.55](docs/parity-0.55.0.md), [0.56](docs/parity-0.56.0.md), and [0.57](docs/parity-0.57.0.md) for detailed behavior and limits.

## 0.57.0 / browser integration 0.39.26 (recorded 0.39.0) — live HLS recording

- Capture exposed clear live HLS through native parallel workers and playlist refreshes; Stop and save the captured portion or pause separately.
- Persist encrypted segment receipts, verify resumed media, recover offline after expiration/publication conflicts, and clean only owned cache files after successful publication.
- Respect retry budgets and Retry-After; cancel stalled HTTP promptly; preserve separate-track timing, discontinuities and changed initialization sections.
- 1,461 checks passed, including generated TS/fMP4 media and 32 actual Chrome/Edge extension/native scenarios. Native visual acceptance, broad public-site support and full parity remain open.

## 0.56.0 / browser integration 0.38.26 (recorded 0.38.0) — duplicate overwrite workflows

- Remember numbered-copy, overwrite and Existing choices, with full dialog descriptions and all policies in Options > Downloads.
- Replace completed HTTP/FTP files after verifying new bytes; retain prior versions with accurate metadata and invalidate obsolete scan results for edited copies.
- Restart unfinished duplicates at the original filename with a fresh parts generation, preserved preferences and correct browser/CLI confirmation routing.
- Commit history and preferences together; retain old parts on failure and recover cleanup after a crash without deleting unrelated or changed files.
- 1,185 checks passed: 1,135 native, 16 protocol/app-host, 30 Chrome/Edge and 4 real active/passive FTP scenarios. Eight selected resource bounds match. Native visual acceptance and full parity remain open.

## 0.55.0 / browser integration 0.38.25 (recorded 0.38.0) — quota and download workflows

- Add the download-limit warning preference, native warning dialog, quota countdown and waiting status.
- Handle simultaneous quota reservations, large chunks, cancellation, rollover, restart and malformed/future saved timestamps.
- Open Download complete for existing completed links and resume unfinished duplicates directly; keep automatic completion actions separate from viewing history.
- Preserve new-download confirmation, saved segments, paused records on persistence failure and existing double-click behavior.
- 1,139 checks passed, including 1,095 native checks and real isolated Chrome/Edge downloads. Six selected reference rectangles match. Native visual acceptance and full parity remain open.

## 0.54.0 / browser integration 0.38.24 (recorded 0.38.0) — browser customization update

- Add separate native Keys, Browser Menus and Video Panels dialogs with functional preferences.
- Support Insert/Delete combinations, request-bound held-key capture, Ctrl-force/Alt-bypass defaults and forced web-resource filtering.
- Apply browser-family context-menu switches and video type/minimum-size/site-exception rules; reject stale disabled offers.
- Add optional verified direct-player capture with opt-in, protected/ad/fragment checks and duplicate prevention.
- Fix Edge menu-rebuild and held-key/download-popup races found in real browser testing.
- 1,720 behavioral checks passed, including real isolated Chrome/Edge downloads and 1,040 native checks. 45 static control rectangles match the reference. Native visual acceptance, public-site coverage, full parity and speed equivalence remain unverified.

## 0.53.0 / browser integration 0.37.24 (recorded 0.37.0) — transport configuration update

- Add asynchronous custom PAC scripts, per-protocol configuration, redirect reevaluation and ordered proxy alternatives without POST replay.
- Route passive FTP control/data through PAC-selected HTTP CONNECT and SOCKS; retain precise connection failure messages.
- Add a working TLS 1.3 toggle and an opt-in modification-date override. Preserve certificate, ETag, byte-range, length and refreshed-link identity checks.
- Add reference-position transport controls and clear stale PAC settings from captured browser routes.
- Verified: 1,024 native checks, 24 new transport scenarios, 127 existing live transport regressions, 8 native-host checks, 6 isolated app/host checks and 112 static control bounds. Visual acceptance, full parity and speed equivalence remain unestablished.

## 0.52.0 / browser integration 0.37.23 (recorded 0.37.0) — configuration and queue update

- Rebuild configuration around nine tabs with font-based dialog units, browser lists, category settings, connection exceptions, downloads, proxy routes, logins, dial-up/VPN and sounds.
- Add browser executable enable/disable preferences; the native host derives its actual parent identity. Restart reconnects only UDM native hosts from the active install.
- Add queue selection, optional queue creation/start and persistent prompt preferences for Download Later and batch imports. Queue-only browser capture applies to files and HLS/DASH media.
- Make category extension edits affect routing; preserve folder-memory and type settings across rename/delete and roll back failed persistence.
- Replace the duplicate dropdown with radio choices and compact Add URL with authorization controls. Existing replacement safeguards and HTTPS-only saved logins remain.
- Merge Options drafts without overwriting unrelated live settings and refresh controls after nested editors. Restore global SOCKS selection through Advanced and import Windows proxy routing.
- Verification: 1,008 native checks, 8 host protocol checks, 6 isolated app/host checks and 107 selected resource-bound comparisons. Native visual/click acceptance remains open.

## 0.51.0 / browser integration 0.37.22 (recorded 0.37.0) — download dialog update

- Rebuild File Info, File Properties, Progress, Completion and progress customization with Windows dialog units and Tahoma 8-point measurements. Use native Windows buttons in the light theme.
- Add category creation and recent destination folders to File Info; keep authentication, queue and preview options under More.
- Match completion button order and add the address field. Keep saved-file drag, Open with, folder and suppression actions connected.
- Apply speed-limit edits immediately; preserve remembered limits and reset temporary overrides on resume. Add persistent Hide tab controls.
- Add separate disconnect, exit, power and force controls on the completion tab. Combined actions run once after a cancellable countdown, with force off by default and explicit confirmation when selected.
- See [verification and remaining gaps](docs/parity-0.51.0.md). Native visual acceptance and full parity remain unverified.

## 0.50.0 / browser integration 0.37.21 (recorded 0.37.0) — media recovery update

- Retain completed captured-stream segments across pause, failure and process restart. Verify hashes and exact content/track identity before reuse, including after changing the connection count.
- Add **Refresh media session** for paused captured YouTube streaming jobs: open the original video, recapture the same selection, then save the matching session or save and resume on the existing record.
- Reuse verified complete tracks when retrying assembly, even after the browser session expires. Preserve input tracks on output hash failure or destination conflict.
- Show retained/reused bytes, media phases, **Retry assembly**, and an ETA that excludes previously retained media. Browser panels acknowledge session recovery with a review message.
- See [test evidence and remaining limitations](docs/parity-0.50.0.md). This update does not establish current YouTube compatibility or complete IDM parity.

## v0.40.0 — combined 0.37 through 0.40 update (27 September 2026)

This published release includes the previously untagged 0.37.0, 0.38.0, and 0.39.0 milestones as well as the final 0.40.0 changes. The tag provides one honest source snapshot and file comparison from v0.36.0; the linked notes preserve the recorded detail for each intervening milestone.

- **0.40.0:** separate HTTP/HTTPS/FTP proxy routes, passive FTP through HTTP CONNECT, and Microsoft Defender/ClamAV scanner presets. See [0.40 verification and limits](docs/parity-0.40.0.md).
- **0.39.0:** durable browser-handoff ownership receipts and recovery after an extension worker restart. See [0.39 verification and limits](docs/parity-0.39.0.md).
- **0.38.0:** safer browser form-download capture up to 1 MiB, plus Firefox handoff and saved-history reliability fixes. See [0.38 verification and activation](docs/parity-0.38.0.md).
- **0.37.0:** audio-only M4A output, explicit recorded-media audio language selection, and optional WebVTT subtitles in supported MP4 downloads. See [0.37 verification and limits](docs/parity-0.37.0.md).

## Included milestone: 0.39.0 / browser integration 0.29.20 (recorded 0.29.0)

Adds persistent browser-handoff ownership receipts and recovery after the extension worker stops. Accepted downloads continue in UDM; unsubmitted downloads resume in the browser. A durable release receipt prevents a delayed Add from creating a second download. Ambiguous ownership remains for review, and recovery never replays form bodies. Adds the popup recovery button. Activated locally in UDM, Chrome and Edge; Firefox was verified in an isolated profile. Passed 761 native checks, 366 browser/protocol/preparation checks and 61 live browser checks. The setup package was rebuilt. See [verification and limits](docs/parity-0.39.0.md).

## Included milestone: browser integration 0.28.20 (recorded 0.28.0)

Adds recorded clear DASH SegmentBase/SIDX support: bounded index parsing, exact range validation, parallel native media downloads, selected audio language and audio-only M4A. Compatible with native 0.38.0. Verified with 345 automated checks and 29 isolated Chrome, Edge and Firefox checks. See [implementation and limits](docs/browser-parity-0.28.0.md).

## Included milestone: 0.38.0

Supports browser form downloads up to 1 MiB with desktop negotiation and bounded memory. Fixes incomplete multipart capture, redirected-body fallback and unreloadable history writes. Extension 0.27.1 fixes Firefox request-event ordering and pause states, validates captured upload lengths, and preserves browser downloads when complete request data is unavailable. Native 0.38.0 and Chrome/Edge integration are activated. Removed the unrelated mouse utility and restored UDM documentation. See [verification and activation status](docs/parity-0.38.0.md).

## Included milestone: 0.37.0

Recorded HLS/DASH now supports audio-only M4A output, explicit audio language selection, and optional WebVTT subtitles embedded in MP4. The simple video handoff stays one click. Native, Chrome, Edge and Firefox checks are recorded in [the verification report](docs/parity-0.37.0.md).

Each version is represented by a source snapshot tag and its original portable bundle on the [GitHub Releases page](https://github.com/usamakkhan/UDM/releases). Select **Compare** to see the exact file and code changes introduced by that version.

| Version | Historical release date | Changes from previous version |
| --- | --- | --- |
| [v0.7.0](https://github.com/usamakkhan/UDM/releases/tag/v0.7.0) | 26 Aug 2026 | First archived source snapshot |
| [v0.8.0](https://github.com/usamakkhan/UDM/releases/tag/v0.8.0) | 28 Aug 2026 | [Compare v0.7.0...v0.8.0](https://github.com/usamakkhan/UDM/compare/v0.7.0...v0.8.0) |
| [v0.9.0](https://github.com/usamakkhan/UDM/releases/tag/v0.9.0) | 31 Aug 2026 | [Compare v0.8.0...v0.9.0](https://github.com/usamakkhan/UDM/compare/v0.8.0...v0.9.0) |
| [v0.10.0](https://github.com/usamakkhan/UDM/releases/tag/v0.10.0) | 2 Sep 2026 | [Compare v0.9.0...v0.10.0](https://github.com/usamakkhan/UDM/compare/v0.9.0...v0.10.0) |
| [v0.10.1](https://github.com/usamakkhan/UDM/releases/tag/v0.10.1) | 4 Sep 2026 | [Compare v0.10.0...v0.10.1](https://github.com/usamakkhan/UDM/compare/v0.10.0...v0.10.1) |
| [v0.11.0](https://github.com/usamakkhan/UDM/releases/tag/v0.11.0) | 6 Sep 2026 | [Compare v0.10.1...v0.11.0](https://github.com/usamakkhan/UDM/compare/v0.10.1...v0.11.0) |
| [v0.12.0](https://github.com/usamakkhan/UDM/releases/tag/v0.12.0) | 8 Sep 2026 | [Compare v0.11.0...v0.12.0](https://github.com/usamakkhan/UDM/compare/v0.11.0...v0.12.0) |
| [v0.12.1](https://github.com/usamakkhan/UDM/releases/tag/v0.12.1) | 11 Sep 2026 | [Compare v0.12.0...v0.12.1](https://github.com/usamakkhan/UDM/compare/v0.12.0...v0.12.1) |
| [v0.12.2](https://github.com/usamakkhan/UDM/releases/tag/v0.12.2) | 13 Sep 2026 | [Compare v0.12.1...v0.12.2](https://github.com/usamakkhan/UDM/compare/v0.12.1...v0.12.2) |
| [v0.13.0](https://github.com/usamakkhan/UDM/releases/tag/v0.13.0) | 15 Sep 2026 | [Compare v0.12.2...v0.13.0](https://github.com/usamakkhan/UDM/compare/v0.12.2...v0.13.0) |
| [v0.14.0](https://github.com/usamakkhan/UDM/releases/tag/v0.14.0) | 17 Sep 2026 | [Compare v0.13.0...v0.14.0](https://github.com/usamakkhan/UDM/compare/v0.13.0...v0.14.0) |
| [v0.14.1](https://github.com/usamakkhan/UDM/releases/tag/v0.14.1) | 20 Sep 2026 | [Compare v0.14.0...v0.14.1](https://github.com/usamakkhan/UDM/compare/v0.14.0...v0.14.1) |
| [v0.15.0](https://github.com/usamakkhan/UDM/releases/tag/v0.15.0) | 22 Sep 2026 | [Compare v0.14.1...v0.15.0](https://github.com/usamakkhan/UDM/compare/v0.14.1...v0.15.0) |
| [v0.16.0](https://github.com/usamakkhan/UDM/releases/tag/v0.16.0) | 24 Sep 2026 | [Compare v0.15.0...v0.16.0](https://github.com/usamakkhan/UDM/compare/v0.15.0...v0.16.0) |
| [v0.16.1](https://github.com/usamakkhan/UDM/releases/tag/v0.16.1) | 26 Sep 2026 | [Compare v0.16.0...v0.16.1](https://github.com/usamakkhan/UDM/compare/v0.16.0...v0.16.1) |
| [v0.29.0](https://github.com/usamakkhan/UDM/releases/tag/v0.29.0) | 26 Sep 2026 | [Compare v0.16.1...v0.29.0](https://github.com/usamakkhan/UDM/compare/v0.16.1...v0.29.0) |
| [v0.30.0](https://github.com/usamakkhan/UDM/releases/tag/v0.30.0) | 26 Sep 2026 | [Compare v0.29.0...v0.30.0](https://github.com/usamakkhan/UDM/compare/v0.29.0...v0.30.0) |
| [v0.31.0](https://github.com/usamakkhan/UDM/releases/tag/v0.31.0) | 27 Sep 2026 | [Compare v0.30.0...v0.31.0](https://github.com/usamakkhan/UDM/compare/v0.30.0...v0.31.0) |
| [v0.33.0](https://github.com/usamakkhan/UDM/releases/tag/v0.33.0) | 27 Sep 2026 | Includes [0.32 FTP-through-SOCKS](docs/parity-0.32.0.md) and [0.33 scheduler wake timers](docs/parity-0.33.0.md); [compare v0.31.0...v0.33.0](https://github.com/usamakkhan/UDM/compare/v0.31.0...v0.33.0) |
| [v0.34.0](https://github.com/usamakkhan/UDM/releases/tag/v0.34.0) | 27 Sep 2026 | [Compare v0.33.0...v0.34.0](https://github.com/usamakkhan/UDM/compare/v0.33.0...v0.34.0) |
| [v0.36.0](https://github.com/usamakkhan/UDM/releases/tag/v0.36.0) | 27 Sep 2026 | Includes [0.35 alternate audio-track selection](docs/parity-0.35.0.md) and [0.36 signed network backend](docs/parity-0.36.0.md); [compare v0.34.0...v0.36.0](https://github.com/usamakkhan/UDM/compare/v0.34.0...v0.36.0) |
| [v0.40.0](https://github.com/usamakkhan/UDM/releases/tag/v0.40.0) | 27 Sep 2026 | Includes [0.37 recorded-media outputs](docs/parity-0.37.0.md), [0.38 form-download reliability](docs/parity-0.38.0.md), [0.39 handoff recovery](docs/parity-0.39.0.md), and [0.40 proxy/FTP/scanner updates](docs/parity-0.40.0.md); [compare v0.36.0...v0.40.0](https://github.com/usamakkhan/UDM/compare/v0.36.0...v0.40.0) |
| [v0.57.0](https://github.com/usamakkhan/UDM/releases/tag/v0.57.0) | 28 Sep 2026 | Includes milestones 0.41–0.43 and 0.50–0.57; [compare v0.40.0...v0.57.0](https://github.com/usamakkhan/UDM/compare/v0.40.0...v0.57.0) |
| [v0.68.0](https://github.com/usamakkhan/UDM/releases/tag/v0.68.0) | 28 Sep 2026 | Includes milestones 0.58–0.68; [compare v0.57.0...v0.68.0](https://github.com/usamakkhan/UDM/compare/v0.57.0...v0.68.0) |

The commits were restored from the original archived source trees. This makes GitHub's Files changed view useful for every consecutive release pair.

## Archived tag version ledger

These tags have no paired milestone heading above. The recorded number comes from each tag’s Chromium manifest. The accounted number shows its place in the reconciled sequence.

| Desktop tag | Recorded browser | Accounted browser |
| --- | --- | --- |
| `v0.92.0` | 0.63.8 | 0.64.36 |
| `v0.91.0` | 0.63.8 | 0.64.35 |
| `v0.90.0` | 0.63.8 | 0.64.34 |
| `v0.89.0` | 0.63.8 | 0.64.33 |
| `v0.88.0` | 0.63.8 | 0.64.32 |
| `v0.85.0-rc.3` | 0.63.0 | 0.64.23 |
| `v0.85.0-rc.2` | 0.63.0 | 0.64.22 |
| `v0.40.0` | 0.29.0 | 0.29.21 |
| `v0.36.0` | 0.25.0 | 0.25.20 |
| `v0.34.0` | 0.24.0 | 0.24.19 |
| `v0.33.0` | 0.24.0 | 0.24.18 |
| `v0.31.0` | 0.24.0 | 0.24.16 |
| `v0.30.0` | 0.24.0 | 0.24.15 |
| `v0.29.0` | 0.24.0 | 0.24.14 |
| `v0.16.1` | 0.16.1 | 0.16.8 |
| `v0.16.0` | 0.16.0 | 0.16.7 |
| `v0.15.0` | 0.15.0 | 0.15.7 |
| `v0.14.1` | 0.14.1 | 0.14.8 |
| `v0.14.0` | 0.13.0 | 0.13.7 |
| `v0.13.0` | 0.13.0 | 0.13.6 |
| `v0.12.2` | 0.11.0 | 0.11.6 |
| `v0.12.1` | 0.11.0 | 0.11.5 |
| `v0.12.0` | 0.11.0 | 0.11.4 |
| `v0.11.0` | 0.11.0 | 0.11.3 |
| `v0.10.1` | 0.8.0 | 0.8.3 |
| `v0.10.0` | 0.8.0 | 0.8.2 |
| `v0.9.0` | 0.8.0 | 0.8.1 |
| `v0.8.0` | 0.8.0 | 0.8.0 |
| `v0.7.0` | 0.5.0 | 0.5.0 |

The `v1.0.1` manifest records 0.63.14, while its earlier changelog heading recorded 0.63.8. The sequence uses the tag value for that milestone. The 1.0.4 release commit and its notes record 0.63.20. The separate `v1.0.4` snapshot contains 0.63.61, within the browser advances accounted for by 1.1.0.

Earlier repeated desktop 0.41, 0.42, 0.65, 0.69 and 0.78 deliveries also receive distinct patch labels below. Their browser increments were already counted, so relabeling them adds no new increments.

### Earlier GUI delivery accounting

These 39 functional candidate deliveries were omitted from the previous audits. Each linked note records its own changed behavior and build or delivered candidate. Allocate a block before the already-accounted Properties candidate: desktop 0.84.1–0.84.39 and browser 0.55.45–0.55.83. Rows use filename order as stable accounting identifiers; the notes do not establish a complete delivery chronology. The former Properties label 0.84.1 becomes 0.84.40, and later 0.84 labels and browser accounting positions move forward by 39. Recorded versions in binaries, receipts and tags remain unchanged.

| Historical note | Accounted desktop | Accounted browser |
| --- | --- | --- |
| [Arrange files menu — staged candidate](docs/gui-arrange-menu-candidate-20261001.md) | 0.84.1 | 0.55.45 |
| [Category dialog controls — staged candidate](docs/gui-category-controls-candidate-20261001.md) | 0.84.2 | 0.55.46 |
| [Predefined category lifecycle — staged candidate](docs/gui-category-lifecycle-candidate-20261001.md) | 0.84.3 | 0.55.47 |
| [Category site restriction routing — staged candidate](docs/gui-category-restrictions-candidate-20261001.md) | 0.84.4 | 0.55.48 |
| [Category icons refresh after editing](docs/gui-category-row-refresh-20261001.md) | 0.84.5 | 0.55.49 |
| [Category file types — staged candidate](docs/gui-category-types-candidate-20261001.md) | 0.84.6 | 0.55.50 |
| [Column customization: save failure and recovery](docs/gui-column-save-recovery-20261001.md) | 0.84.7 | 0.55.51 |
| [Completed-file synchronization enrollment — staged candidate](docs/gui-completed-enrollment-candidate-20261001.md) | 0.84.8 | 0.55.52 |
| [Completed synchronization queue members — candidate acceptance](docs/gui-completed-membership-candidate-20261001.md) | 0.84.9 | 0.55.53 |
| [Default synchronization queue — staged candidate, 1 October 2026](docs/gui-default-sync-queue-candidate-20261001.md) | 0.84.10 | 0.55.54 |
| [Display scaling candidate](docs/gui-display-scaling-20261001.md) | 0.84.11 | 0.55.55 |
| [Downloads menu placement — staged candidate](docs/gui-downloads-layout-candidate-20261001.md) | 0.84.12 | 0.55.56 |
| [File menu alignment — staged candidate](docs/gui-file-menu-candidate-20261001.md) | 0.84.13 | 0.55.57 |
| [Find dialog alignment — staged candidate](docs/gui-find-dialog-candidate-20261001.md) | 0.84.14 | 0.55.58 |
| [Font menu and reset — staged candidate](docs/gui-font-reset-candidate-20261001.md) | 0.84.15 | 0.55.59 |
| [Column-header customization menu](docs/gui-header-columns-20261001.md) | 0.84.16 | 0.55.60 |
| [Help product actions — staged candidate](docs/gui-help-product-candidate-20261001.md) | 0.84.17 | 0.55.61 |
| [Offline Help and stable column widths — candidate acceptance](docs/gui-help-topics-candidate-20261001.md) | 0.84.18 | 0.55.62 |
| [Main-window keyboard navigation](docs/gui-keyboard-navigation-20261001.md) | 0.84.19 | 0.55.63 |
| [Speed Limiter menu — staged candidate](docs/gui-limiter-menu-candidate-20261001.md) | 0.84.20 | 0.55.64 |
| [Shared speed-limit retention — staged correction](docs/gui-limiter-shared-save-candidate-20261001.md) | 0.84.21 | 0.55.65 |
| [Atomic multi-file queue changes — staged candidate](docs/gui-membership-batch-candidate-20261001.md) | 0.84.22 | 0.55.66 |
| [Native menu-state correction — staged candidate](docs/gui-menu-state-candidate-20261001.md) | 0.84.23 | 0.55.67 |
| [Mixed-selection Stop/Resume correction — staged candidate](docs/gui-mixed-selection-candidate-20261001.md) | 0.84.24 | 0.55.68 |
| [Progress limiter control recovery](docs/gui-progress-limiter-recovery-20261001.md) | 0.84.25 | 0.55.69 |
| [Queue drag-and-drop — staged candidate](docs/gui-queue-drop-candidate-20261001.md) | 0.84.26 | 0.55.70 |
| [Queue state indicators — staged candidate, 1 October 2026](docs/gui-queue-indicators-candidate-20261001.md) | 0.84.27 | 0.55.71 |
| [Queue membership display — staged candidate, 1 October 2026](docs/gui-queue-membership-view-candidate-20261001.md) | 0.84.28 | 0.55.72 |
| [Named queue submenus — staged candidate](docs/gui-queue-submenus-candidate-20261001.md) | 0.84.29 | 0.55.73 |
| [Expired-link recovery and stale metadata](docs/gui-refresh-link-recovery-20261001.md) | 0.84.30 | 0.55.74 |
| [Scheduler queue membership controls — staged candidate](docs/gui-scheduler-membership-candidate-20261001.md) | 0.84.31 | 0.55.75 |
| [Selected-file Stop dialog lifecycle — staged correction](docs/gui-selected-stop-candidate-20261001.md) | 0.84.32 | 0.55.76 |
| [Separate Pause all and Stop all — staged candidate](docs/gui-stop-all-candidate-20261001.md) | 0.84.33 | 0.55.77 |
| [Tasks menu alignment — staged candidate](docs/gui-tasks-menu-candidate-20261001.md) | 0.84.34 | 0.55.78 |
| [Tip of the Day — staged candidate](docs/gui-tip-day-candidate-20261001.md) | 0.84.35 | 0.55.79 |
| [UDM toolbar action completion — staged candidate, 1 October 2026](docs/gui-toolbar-share-candidate-20261001.md) | 0.84.36 | 0.55.80 |
| [Tree context menu targeting — candidate acceptance](docs/gui-tree-context-candidate-20261001.md) | 0.84.37 | 0.55.81 |
| [Help: Check for updates — staged candidate](docs/gui-update-check-candidate-20261001.md) | 0.84.38 | 0.55.82 |
| [View menu grouping — staged candidate](docs/gui-view-menu-candidate-20261001.md) | 0.84.39 | 0.55.83 |

Excluded: the whole-menu gap audit, active-menu acceptance, limiter rollback qualification, actual-transfer progress qualification and concurrent Stop all qualification introduce no separate functional delivery. Completed-file Properties is already counted by the Properties candidate and receives no additional increment.

### Additional historical builds

The 0.79–0.82 installers reused browser 0.53.1. The remaining rows reused desktop 0.84.0 and the browser version shown. Accounted desktop numbers for the repeated 0.84 builds are assigned in delivery order, including RC1–RC3 above. These labels identify historical milestones. Build receipts and tags retain the recorded versions.

| Historical build | Accounted desktop | Recorded browser | Accounted browser | Delivered change |
| --- | --- | --- | --- | --- |
| [Recorded-caption package](docs/recorded-caption-release-20261008.md) | 0.84.78 | 0.62.13 | 0.63.32 | Recorded WebVTT initialization headers before RC2 |
| [Properties 0.62.11 package](docs/properties-release-20261008.md) | 0.84.77 | 0.62.11 | 0.63.30 | Corrected subtitle link labels after the live WebVTT build |
| [Saved completion gate](docs/cli-saved-completion-20261006.md) | 0.84.76 | 0.62.8 | 0.63.26 | Wait for durable completion before CLI actions |
| [Scanner diagnostics](docs/completion-scanner-20261006.md) | 0.84.75 | 0.62.8 | 0.63.25 | Readable completion scanner diagnostics |
| [CLI hang-up](docs/cli-hangup-20261006.md) | 0.84.74 | 0.62.8 | 0.63.24 | Disconnect after successful completion |
| [CLI completion exit](docs/cli-completion-exit-20261006.md) | 0.84.73 | 0.62.8 | 0.63.23 | Exit after a successful download |
| [Transactional Pause](docs/validation/pause-package-20261006.json) | 0.84.72 | 0.62.8 | 0.63.22 | Durable pause before stopping a worker |
| [R212](docs/current-staged-candidate.md#r212-completion-dialog-controls-and-verification) | 0.84.70 | 0.62.8 | 0.63.20 | Completion dialog icon and drag area |
| [R210](docs/current-staged-candidate.md#r210-progress-options-gui-and-duplicate-text-fix) | 0.84.69 | 0.62.8 | 0.63.19 | Completion-option checkbox captions |
| [R209](docs/current-staged-candidate.md#r209-progress-status-control-alignment) | 0.84.68 | 0.62.8 | 0.63.18 | Progress URL and status controls |
| [R207](docs/current-staged-candidate.md#r207-scheduler-command-line-start) | 0.84.67 | 0.62.8 | 0.63.17 | CLI queue start |
| [R206](docs/current-staged-candidate.md#r206-queue-only-command-line-compatibility) | 0.84.66 | 0.62.8 | 0.63.16 | Queue-only CLI admission |
| [R201](docs/current-staged-candidate.md#r201-properties-source-link-control-difference-removed) | 0.84.65 | 0.62.8 | 0.63.15 | Accessible native Properties source link |
| [R174](docs/current-staged-candidate.md#r174-find-download-address-coverage) | 0.84.64 | 0.62.5 | 0.63.13 | Find matches additional download addresses |
| [R172](docs/current-staged-candidate.md#r172-protected-column-checkbox-consistency) | 0.84.63 | 0.62.5 | 0.63.12 | Protect the File Name checkbox |
| [R171](docs/current-staged-candidate.md#r171-columns-showhide-controls) | 0.84.62 | 0.62.5 | 0.63.11 | Separate Show and Hide column actions |
| [R159](docs/current-staged-candidate.md#r159-columns-dialog-controls-staged-2026-10-05) | 0.84.61 | 0.62.1 | 0.63.6 | Column width and ordering controls |
| [R158](docs/current-staged-candidate.md#r158-category-panel-close-control-staged-2026-10-05) | 0.84.60 | 0.62.1 | 0.63.5 | Category-panel close control |
| [R157](docs/current-staged-candidate.md#r157-main-tray-recovery-staged-2026-10-05) | 0.84.59 | 0.62.1 | 0.63.4 | Main tray recovery |
| [R155](docs/current-staged-candidate.md#r155-tray-progress-caption-staged-2026-10-05) | 0.84.58 | 0.62.1 | 0.63.3 | Live tray progress captions |
| [R154](docs/current-staged-candidate.md#r154-dark-download-list-gridlines-staged-2026-10-05) | 0.84.57 | 0.62.1 | 0.63.2 | Dark download-list gridlines |
| [R153](docs/current-staged-candidate.md#r153-toolbar-layout-tested-not-installed-2026-10-05) | 0.84.56 | 0.62.1 | 0.63.1 | Single-row toolbar |
| [R151](docs/current-staged-candidate.md#r151-staged--saved-http-and-ftp-logins-2026-10-05) | 0.84.55 | 0.62.1 | 0.63.0 | Saved HTTP and FTP logins |
| [R149](docs/current-staged-candidate.md#r149-installed--saved-login-controls-and-settings-lists-2026-10-05) | 0.84.54 | 0.62.1 | 0.62.99 | Saved-login editing controls |
| [R148](docs/current-staged-candidate.md#r148-installed--download-quota-preferences-2026-10-05) | 0.84.53 | 0.62.1 | 0.62.98 | Download quotas |
| [R147](docs/current-staged-candidate.md#r147-server-exception-selection-controls-staged) | 0.84.52 | 0.62.1 | 0.62.97 | Server exception selection controls |
| [R146](docs/current-staged-candidate.md#r146-scoped-wildcard-server-connection-rules-staged) | 0.84.51 | 0.62.1 | 0.62.96 | Scoped server connection rules |
| [R145](docs/current-staged-candidate.md#r145-tray-cleanup-regression-under-test) | 0.84.50 | 0.62.1 | 0.62.95 | Tray icon cleanup |
| [R144](docs/current-staged-candidate.md#r144-per-download-tray-mode-installed) | 0.84.49 | 0.62.1 | 0.62.94 | Per-download tray mode |
| [R143](docs/current-staged-candidate.md#r143-progress-start-view-behavior-staged) | 0.84.48 | 0.62.1 | 0.62.93 | Progress dialog start-view preferences |
| [R139](docs/current-staged-candidate.md) | 0.84.47 | 0.62.1 | 0.62.92 | Dark group-box captions |
| [R138](docs/current-staged-candidate.md) | 0.84.46 | 0.62.1 | 0.62.91 | Server retry and synchronization probe recovery |
| [R137](docs/current-staged-candidate.md) | 0.84.45 | 0.62.1 | 0.62.90 | Download dialog and queue-only preference dependency |
| [R136](docs/current-staged-candidate.md) | 0.84.44 | 0.62.1 | 0.62.89 | Configuration tab order |
| [R132](docs/current-staged-candidate.md#r132-idle-checkpoint-correction-in-r131-clean-build-workspace) | 0.84.43 | 0.61.0 | 0.61.87 | Idle queue checkpoint correction |
| [R126](docs/current-staged-candidate.md) | 0.84.42 | 0.61.0 | 0.61.86 | Resume status across video and audio |
| [R125](docs/current-staged-candidate.md#r125-installed-native-fix-and-browser-061-files) | 0.84.41 | 0.60.0 | 0.60.85 | Live media addresses and destination field |
| [0.84.0 Properties candidate](docs/properties-release-assembly-20261001.md) | 0.84.40 | 0.55.0 | 0.55.84 | Completed-file Properties queue-membership correction |
| [0.82.0](docs/candidate-0.82.0.md) | 0.82.0 | 0.53.1 | 0.53.44 | Dial-up/VPN credentials |
| [0.81.0](docs/candidate-0.81.0.md) | 0.81.0 | 0.53.1 | 0.53.43 | Shared HTTP connections |
| [0.80.0](docs/candidate-0.80.0.md) | 0.80.0 | 0.53.1 | 0.53.42 | HTTP/2 negotiation |
| [0.79.0](docs/candidate-0.79.0.md) | 0.79.0 | 0.53.1 | 0.53.41 | Import/export and explicit Resume |

### Deliveries already covered by a browser increment

| Delivery | Recorded browser | Accounted browser |
| --- | --- | --- |
| [0.83.0 nested HLS audio installer](docs/browser-055-nested-audio-candidate.md) | 0.55.0 | 0.55.44 |
| [0.83.0 media receipt installer](docs/candidate-0.83.0.md) | 0.54.0 | 0.54.44 |
| [0.41.1 (recorded 0.41.0) hierarchical recorded DASH](docs/browser-parity-0.31.0.md) | 0.31.0 | 0.31.21 |

### Earlier paired development records

| Desktop record | Recorded browser | Accounted browser |
| --- | --- | --- |
| [0.78.2 (recorded 0.78.0) / Firefox and resume](docs/parity-browser-0.52.2.md) | 0.52.2 | 0.52.41 |
| [0.78.1 (recorded 0.78.0) / Firefox recovery](docs/parity-browser-0.52.1.md) | 0.52.1 | 0.52.40 |
| [0.78.0](docs/parity-0.78.0.md) | 0.52.0 | 0.52.39 |
| [0.77.0](docs/parity-0.77.0.md) | 0.51.0 | 0.51.39 |
| [0.76.0](docs/parity-0.76.0.md) | 0.50.0 | 0.50.39 |
| [File-form capture candidate](docs/candidate-browser-0.49.0.md) | 0.49.0 | 0.49.39 |
| [File recognition candidate](docs/research-recognition-followup-2026-09-29.md) | 0.48.0 | 0.48.39 |
| [0.75.0 automation candidate](docs/candidate-native-0.75.0.md) | Unspecified | 0.47.40 |
| [0.65.1 (recorded 0.65.0) / Edge video selection](docs/parity-browser-0.43.0.md) | 0.43.0 | 0.43.32 |
| [0.43.0](docs/parity-0.43.0.md) | 0.36.0 | 0.36.21 |
| [0.42.3 (recorded 0.42.0) / captured YouTube audio](docs/parity-browser-0.35.0.md) | 0.35.0 | 0.35.21 |
| [0.42.2 (recorded 0.42.0) / hierarchical DASH indexes](docs/parity-browser-0.34.0.md) | 0.34.0 | 0.34.21 |
| [0.42.1 (recorded 0.42.0) / separate DASH indexes](docs/parity-browser-0.33.0.md) | 0.33.0 | 0.33.21 |
| [0.42.0](docs/parity-0.42.0.md) | 0.32.0 | 0.32.21 |
| [0.41.0](docs/parity-0.41.0.md) | 0.30.0 | 0.30.21 |
| [0.38.0](docs/parity-0.38.0.md) | 0.27.1 | 0.27.21 |
| [0.37.0](docs/parity-0.37.0.md) | 0.26.0 | 0.26.20 |
| [0.35.0](docs/parity-0.35.0.md) | 0.25.0 | 0.25.19 |
| [0.32.0](docs/parity-0.32.0.md) | 0.24.0 | 0.24.17 |
| [0.28.0](docs/parity-0.28.0.md) | Unspecified | 0.24.13 |
| [0.27.0](docs/parity-0.27.0.md) | 0.24.0 | 0.24.12 |
| [0.26.0](docs/parity-0.26.0.md) | 0.23.3 | 0.23.15 |
| [0.25.0 / browser audit fixes](docs/audit-0.23.2.md) | 0.23.2 | 0.23.14 |
| [0.25.0 / reload recovery](docs/browser-reload-0.23.1.md) | 0.23.1 | 0.23.13 |
| [0.25.0](docs/browser-parity-0.25.0.md) | 0.23.0 | 0.23.12 |
| [0.23.0](docs/player-retrieval-0.23.0.md) | 0.21.0 | 0.21.12 |
| [0.22.0](docs/links-login-0.22.0.md) | 0.20.3 | 0.20.15 |
| [0.21.2](docs/parallel-video-0.21.2.md) | 0.20.3 | 0.20.14 |
| [0.21.1](docs/video-comparison-0.21.1.md) | 0.20.3 | 0.20.13 |
| [0.21.0](docs/video-comparison-0.21.0.md) | 0.20.3 | 0.20.12 |
| [0.20.0 / panel recovery](docs/browser-0.20.2.md) | 0.20.2 | 0.20.11 |
| [0.20.0 / capture bypass](docs/browser-0.20.1.md) | 0.20.1 | 0.20.10 |
| [0.20.0](docs/gui-0.20.0.md) | 0.20.0 | 0.20.9 |
| [0.19.0](docs/gui-0.19.0.md) | 0.17.0 | 0.17.9 |
| [0.18.0](docs/gui-0.18.0.md) | 0.17.0 | 0.17.8 |
| [0.17.0](docs/speed-menu-0.17.0.md) | 0.17.0 | 0.17.7 |

## Documented development milestones

The project also retains the following development notes. They are committed with the current source so their work is visible in Git, but they are **not** backfilled as source tags or GitHub Releases: separate source snapshots and portable builds for these points were not available. Consequently, GitHub cannot show an honest file-by-file comparison for them. The notes describe the recorded work; they do not claim to recreate the exact code at that time.

| Documented version | Recorded change note |
| --- | --- |
| 0.17.0 | [Transfer measurements, video formats and download dialogs](docs/speed-menu-0.17.0.md) |
| 0.18.0 | [Desktop GUI expansion](docs/gui-0.18.0.md) |
| 0.19.0 | [Desktop workflow completion pass](docs/gui-0.19.0.md) |
| 0.20.0 | [YouTube panel repair and browser controls](docs/gui-0.20.0.md) |
| 0.20.1 | [Browser integration](docs/browser-0.20.1.md) |
| 0.20.2 | [Panel recovery and verified Edge download](docs/browser-0.20.2.md) |
| 0.21.0 | [Video comparison and implementation notes](docs/video-comparison-0.21.0.md) |
| 0.21.1 | [Live video comparison and storage repair](docs/video-comparison-0.21.1.md) |
| 0.21.2 | [Parallel YouTube downloads](docs/parallel-video-0.21.2.md) |
| 0.22.0 | [Media addresses and server logins](docs/links-login-0.22.0.md) |
| 0.23.0 | [Native player retrieval and browser integration](docs/player-retrieval-0.23.0.md) |
| 0.23.1 | [Extension reload recovery](docs/browser-reload-0.23.1.md) |
| 0.23.2 | [Audit](docs/audit-0.23.2.md) |
| 0.25.0 | [Browser integration update](docs/browser-parity-0.25.0.md) |
| 0.26.0 | [Native and browser comparison](docs/parity-0.26.0.md) |
| 0.27.0 | [Browser integration 0.24.0](docs/parity-0.27.0.md) |
| 0.28.0 | [Offline websites and SOCKS5](docs/parity-0.28.0.md) |
