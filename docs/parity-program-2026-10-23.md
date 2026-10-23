# Parity program: making UDM match IDM inside and out (2026-10-23)

Goal: behaviour parity with IDM 6.43 in every user-visible place (dialogs, menus,
capture, transfer, media) and in the backend paths that produce it, within the
standing constraints: no IDM code or data is shipped, no certificate injection,
no test-signed drivers, and every claim is backed by a check in the repo.

This document is the working list. Rows move to "Done" only with evidence.

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
| Main window | Toolbar menu: Neon / 3D Style looks, "Look for new toolbars" (offline equivalent); toolbar icon set look (own icons stay, geometry matches) |
| Batch download | Single pattern address with asterisk; "Replace asterisk to" group (Numbers/Letters, From/To, wildcard size); authorization group; First/Second/Last file live preview; OK/Cancel (done on `cranky-jackson`, verify after merge) |
| Download complete | Done 2026-10-23: IDM's two-line text and the file type's shell icon on the unchanged dialog-286 geometry (`CompletionDialogUiTests` 106) |
| Options | Frame 576×653 with two-row tabs and OK + Help (done on `cranky-jackson`); General page: "Advanced browser integration" line with Restart, IE-module line, Add browser…; File types page: IDM's layout with UDM's extra rows moved below; 40×40 themed page icons |
| Scheduler | Done 2026-10-23: "Download limits" node with IDM's dialog-309 page editing the shared quota (`SchedulerNumberUiTests` 45) |
| Site Grabber | Wizard form: "Step 1 — Set start page" … four steps with its own Project/Options/Help menu and the 149×320 left picture, instead of the single window |
| Columns | 419×389 dialog, IDM's instruction wording, single-column checkbox list, inline "should be [ ] pixels wide" (done on `cranky-jackson`) |
| About | Done 2026-10-23: 461×503 layout with build date, free-software licensing group, legal notice, Home Site / Support Team / copyright (`AboutUiTests` 32, `build.ps1 -AboutUiTestsOnly`) |
| Tip of the Day | Dialog at startup (option), UDM-written tips |

## 3. Backend and integration backlog (from the gap-closing document)

| Item | State | Plan |
|---|---|---|
| HTTP/3 | On `kind-blackwell` only | Merge; then loopback cannot test QUIC — keep the fallback decision logic under test |
| Rare-format recognition | 63 signatures here, 170 on `kind-blackwell` | Merge; then grow the UDM-owned table from the public libmagic test corpus (no `IDMFType.dat`) |
| In-place range writing | On `sleepy-williams` only | Merge; measured by the benchmark harness |
| Negotiate/Kerberos with explicit login to proxies | Not offered | curl SPNEGO with a domain login; needs a domain-joined test machine, otherwise documented limit |
| POST bodies over 4 MiB, non-UTF-8 producers | P | Chunked body transfer over the pipe with a byte-exact loopback check |
| Bundled verified rule list | Empty by policy | Author UDM-owned rules only when a public site is shown to need one |
| Explorer / IE integration | Missing | Shell context-menu verb "Download with UDM" (registry under installer), drop target exists |
| MMS | Missing (legacy) | Out of scope unless requested |
| HTTPS response takeover | Policy | Needs a user-installed root certificate; decide explicitly |
| Live driver, VPN/HVCI, sleep/power, `.ef2` round trip | U | Needs a VM with elevation and a test-signed driver |
| Signing, updater, ARM64 | Missing | Needs a publisher certificate and CI |

IDM's RT_DIALOG templates are the geometry source where they exist:
`python -I tools/idm-research/dump-dialogs.py "IDM FIles/IDMan.exe" out.json` dumps all 105
(About is not a template; its pixel capture remains the reference).

## 4. Working order

1. Merge the four branches (section 1) and re-run the GUI harnesses.
2. GUI: Download complete, About, Scheduler tree, Options General/File types
   layout, Batch dialog verification, then the Site Grabber wizard (largest).
3. Backend: POST > 4 MiB; Explorer verb; signature table growth.
4. Decide the policy items (HTTPS takeover, VM qualification, signing).
