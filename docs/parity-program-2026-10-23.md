# Parity program: making UDM match IDM inside and out (2026-10-23)

Goal: behaviour parity with IDM 6.43 in every user-visible place (dialogs, menus,
capture, transfer, media) and in the backend paths that produce it, within the
standing constraints: no IDM code or data is shipped, no certificate injection,
no test-signed drivers, and every claim is backed by a check in the repo.

This document is the working list. Rows move to "Done" only with evidence.

## Current qualification note (2026-10-28)

The [2026-10-28 source audit](remaining-parity-source-audit-20261028.md) corrects stale backlog claims: updater/signing code exists, explicit proxy Negotiate was excluded by the adapter at the time of that audit, and recent import work is built but not runtime-qualified.

A complete isolated candidate now builds all shipping native companions and the network companion alongside the desktop, and packages the current browser files; see [candidate delivery scope](production-bundle-20261027.md). Installation, signing and runtime qualification remain pending.

Native preview now resolves response-URL and MIME-based filenames through the bundled recognition data; see [filename behavior](native-mime-filenames-20261027.md). This also supplies batch extension correction without Content-Disposition. Live comparison and runtime qualification remain pending.

Native import now implements HTML/repeated-link visibility filters from dialog 228, with hidden selection accounting and stable row identity; see [filter behavior](native-import-filters-20261027.md). Runtime qualification and installation remain pending.

Batch shared/per-row extension adjustment is implemented for server-supplied filenames; see [scope and limits](batch-server-extension-20261027.md). MIME and final-response URL filename derivation are now implemented; runtime qualification remains pending.

Native import file-information preview is implemented in source with bounded asynchronous metadata lookup, server filename suggestions, and Size/Type columns; see [behavior and delivery status](native-import-metadata-20261027.md). Runtime qualification and installation remain pending.

Batch generation now supports descending numeric and letter ranges from retained IDM evidence; see [sequence behavior](batch-sequence-reference-20261027.md). Runtime qualification remains deferred. The same investigation establishes that output rename wildcards must use the generated token, not the displayed row number; explicit token substitution is now implemented in [batch output patterns](batch-output-pattern-20261027.md). Metadata-driven shared extension adjustment is implemented; runtime qualification remains pending.

Native import column sorting is implemented in source, with row identity retained across editing and admission; see [implementation and limits](native-import-sorting-20261027.md). Runtime qualification and installation are pending. Explicit batch-token rename substitution is implemented; metadata-driven extension adjustment is implemented but not runtime-qualified.

The historical "Done" rows below describe individual implementation milestones,
not proof of complete installed IDM equivalence. The October 26 comparison
remains the broader acceptance backlog. In particular, the Scheduler row covers
the quota page only; it does not close the whole Scheduler workstream.

Recent Scheduler evidence is separated by scope:

- [Seconds and native time controls](scheduler-seconds-20261027.md): backend
  boundary/DST checks and UI behavior; installed build records are in that report.
- [Run-once stop control](scheduler-runonce-stop-20261027.md): checkbox and
  persistence behavior, with its separate installed executable hash.
- [File-tab reference correction](scheduler-files-reference-20261027.md): actual
  resource geometry; the earlier reference screenshot was the wrong tab.
- [Long date and shortcuts](scheduler-date-shortcuts-20261027.md): native
  formatting, keyboard dispatch and save behavior; installation remains pending
  in its authoritative installation record until verified.
- [Scale/theme matrix](scheduler-dpi-validation-20261027.md): owned-dialog
  simulated DPI handling, not physical mixed-monitor qualification.
- [Quota alignment](scheduler-limits-alignment-20261027.md): a further resource
  mismatch under validation, not yet installed.

Remaining Scheduler acceptance includes current installed side-by-side rendering
and interactions, keyboard navigation beyond the tested mnemonics, physical
mixed-monitor behavior, and scheduled wake/overnight behavior in the actual
Windows environment. Resource styles and isolated tests do not establish those
live outcomes or complete parity elsewhere in UDM.

### Implementation batches after the live Scheduler comparison

- [Queue workflow](queue-menu-actions-20261027.md): named actions and shared Stop
  availability; [queue sounds](queue-sounds-20261027.md) adds start/stop events.
- [Queue list completion](scheduler-queue-completion-20261027.md): ETA, shared
  progress formatting and live membership/order reconciliation.
- [Apply and save behavior](scheduler-apply-state-20261027.md): dirty controls,
  no-op unchanged saves and field-level merging against current queue records.

The combined desktop build passed and was installed on October 27. The installed
executable hash matches the candidate; UDM was reopened and responded. Its record is in
[the build record](validation/scheduler-save-state-build-20261027.json).
Interactive checks for these latest batches are deferred at the user's request.
Neither compilation nor deferred testing establishes complete IDM parity.

Later uninstalled batches are tracked separately:

- [Native import properties and destination modes](native-import-properties-20261027.md).
- [Browser import review, metadata and selection](browser-link-import-20261027.md),
  with subsequent changes now staged as extension 0.63.61; see the current candidate record.
- [Explicit category preservation](category-policy-preservation-20261027.md).

These source/build milestones do not replace installed or live acceptance evidence.
Interactive and regression testing remain deferred under the current user instruction.

## 1. Consolidation (done)

All were cut from `13eaf3d` (1.0.1) and conflict narrowly with the releases
1.0.2–1.0.4 and the commits since. A trial merge in a scratch worktree showed:

| Branch | Adds | Conflicts | Note |
|---|---|---|---|
| `claude/kind-blackwell-a61678` | HTTP/3 with HTTP/2 fallback (`deeddcc`); 107 more content signatures (`2afc853`, 170 vs 63 here); rule-list note (`c43c5ec`); browser PAC routes (`bf44546`) | CHANGELOG, `chromium-proxy.js` ×2, `BrowserProxy.hpp`, `tests/chromium-proxy.test.cjs`, two docs | `bf44546` is redundant: 1.0.4 shipped browser PAC/auto-detect routes. Merge the other three commits (cherry-pick), drop `bf44546`. |
| `claude/cranky-jackson-0ae64b` | GUI audit with 60 captures; main window (toolbar, list formats, status bar, Queues pane); Batch download, Columns, Options frame/page geometry | `App.cpp`, `ColumnControlsUiTests.cpp`, `MainFeatures.hpp`, `OptionsUi.hpp`, `build.ps1` | Superset of `gui-parity-audit-af1dca`. |
| `claude/sweet-mcclintock-02af7a` | Download context menu with Move-to-queue submenu and category tree nodes (`b6827c0`) | `App.cpp` | Shares the two audit commits with the row above; merge after it. |
| `claude/sleepy-williams-211bc0` | Benchmark vs IDM with transfer fixes (`920917b`); HTTP ranges written in place into one preallocated file, hashed during transfer, published by rename (`7ec7e46`) | CHANGELOG, `Transfer.cpp` | Overlaps the prefetch-receipt and media-metadata work in `Transfer.cpp` from 1.0.4 and `ff58d39`; needs a careful manual merge plus the full suite. |

Done on 2026-10-23 in that order (merges 0993585, d1c48ad, cherry-picks
5674c8d/5922382/477ba6e, merge 2fcb961): full native suite 2968 passed, node
suites 62/56/102/62, GUI harnesses Columns 158, Options 154, Batch 139,
Toolbar 268, Menu-state 419. Two menu-state expectations moved to IDM's list
formats (two-decimal progress, blank unknown remaining time).

## 2. GUI parity backlog (from `docs/gui-parity-2026-10-18.md`, 60 captures)

Open after the branch work (18 partial, 4 missing; branding differences are
intentional and excluded):

| Area | Open items |
|---|---|
| Main window | Done 2026-10-24: toolbar menu in IDM's order with Neon and 3D Style looks (UDM's own icons, the latter on raised tiles) and "Look for new toolbars" opening the toolbar folder (`ToolbarUiTests` 276+) |
| Batch download | Verified 2026-10-24 against IDM's 125 % capture: client 553x476 at 120 DPI and every control at IDM's position (`BatchDialogUiTests`) |
| Download complete | Done 2026-10-23: IDM's two-line text and the file type's shell icon on the unchanged dialog-286 geometry (`CompletionDialogUiTests` 106) |
| Options | Done: frame 576×653 with two-row tabs and OK + Help, General and File types pages at IDM's geometry (`cranky-jackson`), and on 2026-10-23 UDM's own 40×40 page icons (`OptionsIcons.hpp`, `OptionsTabsUiTests`) |
| Scheduler | Done 2026-10-23: "Download limits" node with IDM's dialog-309 page editing the shared quota (`SchedulerNumberUiTests` 45) |
| Site Grabber | Done 2026-10-23: wizard form on IDM's dialog 239 shell ("UDM Site Grabber. Step N --- Set start page / Save files to / Set explorer filters / Set file filters / Explore and download", 717×475 with the Project/Options/Actions/Help menu bar and the 149×320 picture, Back/Next/Close at IDM's positions), step 1 on the captured layout including the Advanced >> manual-login area, steps 2–4 compact in the same region, the exploration result on dialog 241 (905×639, tree + file list) with its secondary actions in the Actions menu (`GrabberWizardUiTests`, `build.ps1 -GrabberWizardUiTestsOnly`) |
| Columns | 419×389 dialog, IDM's instruction wording, single-column checkbox list, inline "should be [ ] pixels wide" (done on `cranky-jackson`) |
| About | Done 2026-10-23: 461×503 layout with build date, free-software licensing group, legal notice, Home Site / Support Team / copyright (`AboutUiTests` 32, `build.ps1 -AboutUiTestsOnly`) |
| Tip of the Day | Done: shown at startup unless turned off, UDM-written tips; 2026-10-24 adds IDM's picture panel and "Did you know..." (`ToolbarUiTests`) |

## 3. Backend and integration backlog (from the gap-closing document)

| Item | State | Plan |
|---|---|---|
| HTTP/3 | Merged; narrowed 2026-10-24 | On the real network one QUIC connection carried all eight ranges at a single connection's share (Alpine 143 to 156 s against IDM's 63 to 68 s); multi-connection downloads now negotiate HTTP/2 or HTTP/1.1 and HTTP/3 stays for single-connection transfers. Loopback cannot test QUIC; the fallback logic remains under test |
| Rare-format recognition | Done 2026-10-23 | Merged (170); then 20 more signatures from the libmagic corpus run, including the PDF-with-BOM and SVG text misclassifications (`docs/content-signatures-libmagic-2026-10-23.md`, `--sniff-corpus` opt-in check, unit checks in the suite). No `IDMFType.dat` |
| In-place range writing | Merged 2026-10-23 | Measured by the benchmark harness (`docs/speed-benchmark-2026-10-23.md`); the finish-phase, chunk-size and hard-disk follow-ups of 2026-10-23/24 build on it |
| Speed and finish time against IDM | Measured 2026-10-23 | Seven loopback cases head to head (`docs/speed-benchmark-2026-10-23.md`): finish now 0.1 to 0.2 s (IDM 0.1 to 3.6 s), UDM faster on single-8, resume-8, drop-8, slow-start-2, level on outage-8; after overlapping each connection's next read with its disk write, UDM is also faster on the single sequential connection (1.4 to 2.3 vs 2.0 to 3.4 s) and level on uncapped loopback. Public origins (2026-10-24, two rounds each): Alpine 64.1 and 61.3 s against IDM's 64.5 and 61.9, kernel 40.8 and 39.4 against 39.7 and 38.2, after leaving HTTP/3 to single-connection downloads and raising the tail-split minimum to 2 MiB. Hard-disk destinations (2026-10-24): with parts on the system SSD as IDM keeps them, level or ahead on D:; the cross-volume publish copy is buffered now (2.6 s for 384 MiB, IDM 2.2 to 2.6). Parts on the same hard disk as the destination stay slow (eight interleaved write streams); the temporary-folder option covers it |
| Negotiate/Kerberos with explicit login to proxies | Automatic and explicit method paths added, unqualified | [Negotiate-only proxy support](proxy-negotiate-downloads-20261028.md) retains existing mixed-method preference; [HTTPS forms](proxy-post-connect-20261028.md) require a rejected tunnel before any upload. [Explicit method selection](proxy-authentication-choice-20261028.md) adds mixed-method and plain HTTP form configuration. Domain authentication, UI and NTLM regression acceptance remain pending. |
| POST bodies over 4 MiB, non-UTF-8 producers | Done 2026-10-23 | Bodies to 64 MiB travel in ordered base64 chunks over the pipe (`PostBodyStore.hpp`), are verified by size and SHA-256, sealed DPAPI-protected outside the catalog and referenced by token; byte-exact loopback check with every byte value (`PostChecks.hpp`), extension protocol check (`tests/post-handoff.test.cjs`). Catalog exports omit staged bodies; live browser acceptance not run |
| Bundled verified rule list | Empty by policy | Author UDM-owned rules only when a public site is shown to need one |
| Explorer / IE integration | Verb done 2026-10-23 | "Download with UDM" on Internet Shortcut (.url) files, registered by the setup (`[Registry]`, removed on uninstall) and by `install.ps1`; `UDM.exe --shortcut` reads the address (`Launch.hpp`, `ReliabilityChecks.hpp`, `tests/explorer-verb.test.py`). IE-module integration stays out of scope; the drop target exists |
| MMS | Missing (legacy) | Out of scope unless requested |
| HTTPS response takeover | Policy | Needs a user-installed root certificate; decide explicitly |
| Live driver, VPN/HVCI, sleep/power, `.ef2` round trip | Unqualified matrix | Needs an isolated elevated environment using the existing vendor-signed driver. No test-signing or boot-policy change is authorized by this backlog. |
| Signing, updater, ARM64 | Mixed | Signing and signed-manifest updater code exist; public publisher signing and installed upgrade acceptance are pending. ARM64 build/runtime support remains absent. |

IDM's RT_DIALOG templates are the geometry source where they exist:
`python -I tools/idm-research/dump-dialogs.py "IDM FIles/IDMan.exe" out.json` dumps all 105
(About is not a template; its pixel capture remains the reference).

## 4. Current next-work boundaries (2026-10-28)

The [current completion audit](parity-completion-audit-20261028.md) distinguishes source gaps from deferred acceptance. The candidate's installer and all 147 payload hashes match their records; the installed executable differs. This does not establish runtime parity.

Recorded DASH duration and refresh compatibility are now implemented. Dynamic/live DASH and multiple Periods remain rejected by the parser/native path; retained evidence does not yet establish exact IDM coverage for these cases. Signing and ARM64 remain separate delivery/platform work. Existing runtime acceptance stays deferred under the current user instruction.

## 5. Historical working order

1. Merge the four branches (section 1) and re-run the GUI harnesses.
2. GUI: Download complete, About, Scheduler tree, Options General/File types
   layout, Batch dialog verification, then the Site Grabber wizard (largest).
3. Backend: ~~POST > 4 MiB~~, ~~Explorer verb~~, ~~signature table growth~~ (all done 2026-10-23).
4. Decide the policy items: laid out with needs and recommendations in `docs/parity-decisions-2026-10-24.md` (HTTPS takeover: do not build; driver qualification: VM before release; signing first, then updater, ARM64 last).
