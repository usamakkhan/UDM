# File Info metadata preparation — UDM 8.90

UDM 8.90 / extension manifests 0.72.54 moves File Info metadata preparation onto an owned cancellable reader. Refresh details can return while Manager is held by another operation. Actual HTTP completion wakes the dialog directly rather than waiting for its periodic pulse. This removes two sources of avoidable waiting; it does not establish a measured speed advantage over IDM.

## Ownership and launch authority

`FileInfoPreviewTask` captures the selected job, generation, URL, private request selection and settings snapshot under Manager. It derives the preview record outside that lock, then reacquires Manager to validate membership, selection and current eligibility before launching the existing bounded probe. Changed URL, headers, actual protected browser proxy, saved progress, completion or request-capture requirements prevent launch. Automatic verification, authentication, retry and response-error guards remain in force. Settings are a point-in-time snapshot from the first read.

Edited login credentials are applied only to a private probe record. Its saved progress is reset for the check; its persistence manager is null. The paused download and saved authorization remain unchanged. Request selection now includes the actual `ProtectedBrowserProxy` field alongside the legacy key, so pending binding also rejects changed routing.

The UI applies a result only for its current job and generation. Credential edits, refresh replacement, decisions, cancellation and teardown invalidate obsolete preparation. Cancellation wakes a reader waiting for Manager; teardown joins the preparation thread. The existing HTTP probe remains bounded and uses its existing atomic cancellation and deadline; this does not introduce a new guarantee that every HTTP worker is joined. Completion retains a weak dialog notifier, and its callback runs after releasing the result mutex.

## Qualification

All five native products report **8.90**; both source manifests report **0.72.54**. Freeze 585 repository inputs and generated COM inputs. Recompile Transfer, Grabber, Bridge, DownloadPreview, ZipPreview and CaptureAdmission; reuse the remaining verified core objects. Transfer source is unchanged, but its object changes through the shared preview header. [Build](validation/file-info-preview-8.90/build.json), [dependencies](validation/file-info-preview-8.90/dependency-inventory.json), [generated COM](validation/file-info-preview-8.90/generated-com-inputs.json).

Pass **3,046 backend assertions across 64 C:/D: runs**. The new reader contributes 29 checks per drive, including a real HEAD request, completion notification, private credential drafts, canceled lock waits and rejection of changed selection, routing, progress, completion and capture requirements. Pending-binding tests also reject changed actual protected proxy routing. [Backend checks](validation/file-info-preview-8.90/backend-checks.json).

Pass **628 actual UI assertions**: 283 Main/File Info, 257 pending-admission and 88 progress-menu checks. The new Main scene holds HTTP headers, stops File Info polling and verifies that actual HTTP completion updates the reported exact byte count. It then holds Manager while Refresh details returns, checks timer/paint responsiveness, edits credentials, cancels, and verifies no late HEAD or stale publication after release. Saved credentials and paused history remain unchanged. [Main UI](validation/file-info-preview-8.90/ui-checks.json), [pending UI](validation/file-info-preview-8.90/pending-ui-checks.json), [progress menu](validation/file-info-preview-8.90/progress-menu-ui-checks.json).

All UI runs use separate owned desktops, close those desktops afterward, never switch the user's desktop and inject no personal-session input. These are component and loopback HTTP checks, not live browser or IDM comparisons. The metadata fixture checks an origin's reported byte length, not the final muxed size of arbitrary streaming formats.

## Version accounting and superseded work

8.88 / extension 0.72.52 introduced the preparation stage, without separate qualification. 8.89 / 0.72.53 added direct HTTP-completion notification. Its 25-check preflight passed, but the source audit found missing final eligibility rechecks and a routing-key mismatch. Stop the incomplete prototype builds, add the negative launch/binding cases, advance to 8.90 / 0.72.54 and freeze fresh inputs. Neither earlier stage is a release or a separately qualified candidate. Preserve its preflight and before/after source hashes, excluding that preflight from primary counts. [Audit](validation/file-info-preview-8.90/prequalification-audit.json).

## Cleanup and delivery

Remove **68 owned fixture directories and 147 temporary executable/object/resource files**, including the incomplete prototype. Preserve redacted assertion records, current products and core cache. Verify seven installed targets, both personal download catalogs and the installed media-helper hashes remain unchanged. [Cleanup](validation/file-info-preview-8.90/cleanup.json), [preservation](validation/file-info-preview-8.90/final-preservation.json).

The pending UI fixture reuses the existing 8.83 alias to installed ffmpeg/ffprobe; no helper binaries are copied and no new alias is created. The prior automatic approval review rejected removal of that alias as “blocked by policy”; no removal retry is attempted.

Installed UDM remains 8.68 / extension files 0.72.32. Browser JavaScript is unchanged; source manifest versions advance with the desktop improvement. No installer, installation, commit, push or release is included.

## Next work

Callers may still wait during offer restoration, capture-context preparation, admission or duplicate handling before reaching File Info. ZIP-specific preparation, category/scheduling actions and final prefetch teardown retain synchronous reads. Move those remaining paths onto owned operations while preserving decision and acknowledgement barriers. Then compare cold/warm dialog readiness, pre-Start exact sizes, complete downloads and all finishing/background work with IDM in a live session. Full visual, browser and timing parity remains unproven.
