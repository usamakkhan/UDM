# UDM 0.12.0: recovery from rejected download addresses

## Observed failure and live recovery

The existing `Win11_25H2_English_x64_v2.iso` download failed with HTTP 403 while retaining 329,558,978 bytes. Its Microsoft signed URL carried a P1 timestamp corresponding to 2026-09-20 04:58:09 UTC; the failed attempt was on 2026-09-21 at 06:18:38 UTC. Microsoft's download page states that its generated links last 24 hours. The signed link had expired before that attempt.

On 2026-09-21, the official Microsoft page generated a fresh link for the same filename. An independent one-byte HTTP comparison at 06:44 UTC returned:

| Address | HTTP | Content-Range |
|---|---:|---|
| Original signed link | 403 | None |
| Fresh signed link | 206 | bytes 0-0/8471603200 |

The fresh response's strong ETag matched the original download's saved ETag. UDM accepted the replacement through its real native messaging host, associated it with the original record, and displayed it in the new recovery dialog. Selecting Save and resume continued from 314.3 MiB instead of restarting. All eight existing partial-file prefixes were independently hashed after resuming; all matched the pre-deployment hashes, preserving exactly 329,558,978 bytes. The desktop visibly advanced past 1.5 GiB. Observed rates varied approximately from 2.8 to 5.1 MiB/s during the checks. A brief UI pause/property-save/resume also succeeded, and Microsoft's published SHA-256 was added to the job for automatic verification before publication. This is a live recovery result, not an IDM speed comparison or a completed-ISO integrity result.

The Microsoft page was controlled through a browser session. Its observed link was sent through the actual UDM native-host protocol, because a connected Chrome session was unavailable. This verifies the desktop/native-host recovery path; it does not establish a new end-to-end Chrome interception test.

## IDM evidence

[IDM's published recovery procedure](https://www.internetdownloadmanager.com/register/new_faq/sites2_3.html) describes opening the original page, obtaining the same download again, and associating its new address with the existing download before resuming.

Additional static analysis of the installed IDMan.exe, SHA-256 `03cc62e9adb77a380f9dc12f67ccaaee5106f12844aa73ce32c914ddd16d607c`, found the following:

- Function 0x004ec6b0 initializes localized recovery strings. This alone is string evidence, not a recovered algorithm.
- Function 0x00468300 adds the refresh-address label to a menu using command 0x2b40.
- Function 0x00441570 handles command 0x1465 by showing the new-address confirmation and, on acceptance, sending command 0x1467 to its owning window.
- Function 0x005c5860 uses the expired-session text and appends the refresh-address suggestion in its error-display path.

The two additional traces exported 12 and 14 function entries, with overlap. Reference disassembly and decompiler pseudocode remain under the excluded `benchmarks/reverse-2026-09-20` directory. These observations support the recovery workflow; they do not establish IDM's complete implementation or any need for a driver to solve HTTP 403. The implementation shipped here is original UDM C++.

## Implemented behavior

- File menu, download context menu, and progress dialog expose Refresh download address for paused or failed HTTP/HTTPS file downloads.
- The dialog offers the original download page and a fresh-link field. Older Microsoft Windows 11 ISO jobs can locate the official page even without a saved referrer.
- One existing job can wait for a matching browser filename for ten minutes. A matching handoff stages an encrypted replacement on that record; unrelated filenames continue through the normal add flow. The user reviews the replacement before applying it. Pending offers expire after ten minutes.
- Applying a replacement preserves the existing ID, target, partial files, range plan and validators. Before reusing bytes, the engine checks total size, a matching server validator, range support and a complete saved plan. A mismatch fails with the partial data retained; it does not silently discard the saved download. File Properties URL edits receive the same validation guard.
- Manual moves to another origin discard old Cookie, Authorization and Referer headers. A captured replacement can supply fresh headers explicitly. Pending replacement addresses and headers use current-user DPAPI encryption; the existing saved main URL format is unchanged.
- HTTP 401, 403 and 410 errors explain how to recover the address. Rejection by itself is not assumed to prove expiration; permissions and session requirements can also cause these statuses.

Video/adaptive captures retain their separate stream-capture workflow. This change does not implement automatic renewal of every website's links, DRM support, a kernel driver, or complete IDM parity. If a server no longer supplies a matching validator, existing partial data stays intact and a separate fresh download may be necessary.

## Validation

Native C++/MFC build: **125 passed, 0 failed**, including 28 new recovery checks. New coverage includes a real expired HTTP endpoint, resumed byte offsets and final SHA-256 across a process-state reload, incompatible validators/sizes/range support, an invalid range plan, stale byte counters, browser matching and cancellation, encrypted pending offers, expiration, sensitive-header handling, invalid protocols/headers, and persistence failure rollback. Existing HTTP, dynamic-range, pause, streaming, adaptive-media and native-pipe regressions also passed. The deployed native host passed its independent framed-stdio/desktop-pipe check.

The user history and previous three executables were backed up before deployment. Partial files were retained in place, with their pre-update hashes recorded. The installed desktop executable is version 0.12.0. Browser JavaScript is unchanged from 0.11.0; no extension permission changes are required.

[Machine-readable evidence](reference/recovery-0.12.0.json). Private signed URLs and user history are excluded from this report and the portable package. The ISO subsequently completed. On 2026-09-23 at 05:00:40 UTC, an independent Get-FileHash check of the final 8,471,603,200-byte file matched Microsoft's published SHA-256 exactly. The saved UDM status is Complete and its own computed hash matches as well. Microsoft's published English x64 ISO SHA-256 is `768984706B909479417B2368438909440F2967FF05C6A9195ED2667254E465E3`.

## Using Refresh download address

1. Select a paused or failed HTTP/HTTPS file download. Right-click it and choose **Refresh download address**, or use the same command in the **File** menu. The progress window also has a **Refresh address** button.
2. Click **Open page**, obtain a new link for the same file, and send it to UDM using the browser extension. You can also paste the fresh direct link in **New address**.
3. Review the replacement and click **Save and resume**. UDM validates it before reusing saved parts. The command is disabled for completed downloads and active transfers; pause an active transfer first. Captured videos use their browser video panel to renew streams.
