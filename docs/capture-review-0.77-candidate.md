**Historical candidate record.** Native 0.77.0 / browser 0.51.0 were subsequently installed and activated in personal Edge. See [the release record](parity-0.77.0.md).

# Captured-link review: native 0.77.0 / browser 0.51.0 candidate

30 September 2026. This candidate is built and tested, **not installed or packaged as an installer**. The installed app remains native 0.76.0 / browser 0.50.0. Full IDM parity is not established.

## Behavior

If a browser response has already been canceled and its intended refresh target was changed, completed or removed before commit, UDM now saves the protected request for a native review. It offers Review replacement, Download new file, Discard captured link, and Later. Reviewing a replacement opens the existing address dialog before changing the URL; existing validator checks still govern reuse of partial files. A separate download always requires File Info, even when automatic start or duplicate reuse is configured. Discard removes only the retained request. Later survives restart and can be reopened from the extension's Recover interrupted downloads button.

The review checks its displayed target again before applying a choice. A stale choice redisplays the current details and requires another click. Native ownership stays durable through lost acknowledgements; browser recovery does not resume or resubmit the canceled response. Failed catalog saves restore the previous jobs and protected request.

## Verification

- Native focused review suite: 72 checks passed.
- Full native regression suite: 2,381 checks passed, including those 72 checks.
- Browser/native-host suite: 1,171 checks passed across 44 scripts, including both Chromium and Firefox transaction recovery.
- Actual native MFC dialogs: 67 checks passed. Covered original presentation behavior plus changed/deleted/completed targets, stale clicks, Later/reopen, ordinary File Info and refresh dialogs, and original partial-file hashes. Rendered replacement and deleted-target dialogs were inspected.
- Isolated Edge restart and actual extension popup: six checks passed. This uses a seeded native review and interrupted browser journal with a real canceled HTTP response. Extension storage survived a full browser restart and reactivation of the temporary unpacked extension. The actual popup Recover button communicated through the real native host. No browser replay, resume or new native job occurred.
- Real Edge multipart transfers: binary upload and redirect scenarios passed ten assertions, including an injected lost commit acknowledgement, exact request/output bytes, one native output, cancellation before commit and one persistent native connection.

The browser restart fixture seeds the review boundary; it does not independently reproduce a user's changing the original target during a live handoff. Native model and MFC tests exercise that change. Personal browser activation, fresh Firefox integration, release packaging and paired deployment remain to be completed.

## Failed attempts retained

The first build invocation was blocked by process script policy before compilation. A process-local Bypass invocation then built the app and host but exposed two private-member accesses in the new tests. Those test errors were corrected; the final test compilation succeeded. No persistent Windows execution-policy setting was changed.

Early Edge fixture attempts failed when initiating an HTTP download directly from the extension worker, when the temporary unpacked extension was not reactivated after restart, and when the popup was treated as a regular browser tab/page. The final fixture initiates the download from its own local page, verifies storage survives reactivation, and targets the actual popup. These are retained test setup failures, not claimed downloader fixes. The first direct-worker failure remains without a root-cause diagnosis.

## Preservation and remaining scope

The installed source baseline and the personal 25-record catalog remain unchanged. The source archive and evidence are saved on D:, with build output also on D:. Existing C-to-D junctions remain in place. No driver, proxy, hosts-file or certificate installation is part of this candidate.

The previously approved HTTPS recognition work is already complete: the installed 0.76 release passed all 13 cases / 23 assertions. Both temporary localhost certificates were removed. Current trust-store presence is recorded in this candidate's verification.json; the old approval does not require installing them again.

Still open: legacy uncertain protocol-0/1 handoffs, the earlier intermittent HLS timeout, broader player/ad/media and proxy modes, automatic driver handoff, COM cold activation, remaining GUI/accessibility and backup compatibility, physical recovery and matched-route speed comparisons. This candidate closes the tested changed-target review gap; it is not a claim of full IDM equivalence.
