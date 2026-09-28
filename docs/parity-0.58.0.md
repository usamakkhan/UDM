# UDM 0.58.0 / browser 0.39.0

This update adds functional Grabber destination choices, website-relative folders, destination previews and atomic batch addition. Full IDM parity remains open.

## Grabber workflow

- The wizard has four steps: Name and starting page, Save to, File filters and exploration limits, and Explore/review/download.
- Save each collected file in its category folder, force one selected category, or choose an explicit folder. Automatic choices use the same host rules and category paths as ordinary downloads.
- In explicit-folder mode, Use original website subfolders maps the URL path beneath the chosen root. It decodes Unicode and spaces, handles Windows reserved names, preserves identical base names in different folders, and assigns numbered copies for collisions in the same folder.
- Review shows the destination folder, address and whether a link is already added. After addition it shows the actual collision-resolved filename. Reopening old projects preserves their previous automatic-category or explicit-folder behavior.
- Add selected commits the entire selection and its saved project in one history update. Validation or storage failure restores the prior catalog without leaving a partial batch. Repeated additions deduplicate project links.
- Category renames update saved project choices; deleting a custom category falls back to Other. Existing downloads keep their chosen path. Changing a paused download's folder in Properties explicitly releases its original project-root constraint.
- Offline ZIP jobs honor the chosen category or folder. Original subfolders are disabled for that template because hierarchical offline archives are not yet implemented.

Reference: [IDM Grabber save settings](https://www.internetdownloadmanager.com/support/idm-grabber/saveto.html). This independently implements the documented destination workflow; it does not establish identical proprietary internals or full Grabber parity.

## Verification

**1,987 checks passed:** 1,325 native checks, 614 browser checks, 8 native protocol checks, 8 isolated desktop/host checks, and 32 extension/native scenarios across Chrome and Edge.

The new Grabber cases cover automatic and forced categories, custom site rules, destination-preview agreement, legacy projects, selected-only addition, single-write catalog commits, retry after locked history, mid-batch rollback, duplicate queries, Unicode/reserved names, traversal, planned file-folder conflicts, existing and late-created junctions, actual parallel HTTP payloads, pause/process-restart/resume and manual Properties destinations. Category-lifecycle tests exercise successful changes and failed-persistence rollback. Existing media, network, file recovery, queues, options and browser handoff regressions also run.

Chrome/Edge use temporary isolated host registrations and private profiles. The generated-media cases include HLS, direct MP4, live HLS capture, duplicate overwrite, header context and keyboard capture rules. These are not a public YouTube/Dailymotion compatibility or internet-speed claim.

The enlarged native test translation unit requires MSVC /bigobj. The full production build and suite use that setting. The first Edge run exposed a test-reader race: a transient history-file lock could supply an empty baseline and make the original file look like a new replacement. The fixture now waits for a readable snapshot containing the completed original and asserts a distinct replacement ID; the final Edge and Chrome runs pass. Production overwrite behavior was retained. [Evidence](evidence-0.58.0/summary.json), [native log](evidence-0.58.0/build-release-candidate.log), [103-workflow inventory](idm-parity-0.58.0.md).

## Remaining limits

Native visual, keyboard/click, accessibility and mixed-monitor acceptance remain unverified because computer-control initialization failed with a kernel-assets path error. Path checks detect unsafe URL components and reparse subfolders at queueing, transfer and publication, but do not claim protection against adversarial concurrent filesystem changes.

Offline archives still use the existing flat ZIP structure; authenticated/rendered mirroring, advanced exclusion/size filters, broader public video support, internet-speed equivalence, driver equivalence, publisher signing and the updater remain incomplete. Browser 0.39.0 and the signed network runtime are retained unchanged. No fresh Firefox-profile acceptance is claimed.

## Current-PC deployment

[UDM 0.58.0 installer](../installer-out/UDM-0.58.0-Browser-0.39.0-Setup-x64.exe) is built. 45 files were deployed with SHA-256 verification and original-file backups. Installer SHA-256: `162A5C76B55829A3E6DA628627B5C2569746C2E82D452EE41DDC6AB2FD42559E`.

The running app/native bridge reports 0.58.0 and the correct D:/UDM/user-data catalog. All 23 records, queues and saved preferences are unchanged. Browser 0.39.0 sources are installed; the signed network runtime is unchanged. This desktop update does not change extension sources; no new extension reload is required. Personal-session activation remains unverified.
