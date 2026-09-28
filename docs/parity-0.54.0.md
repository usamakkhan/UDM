# UDM 0.54.0 / browser 0.38.0

This update connects separate browser customization dialogs to the browser extension's actual capture and filtering behavior. It closes specific UI and integration gaps; it does not establish a percentage of IDM parity.

## What changed

- Options > General > Keys opens a dedicated key editor with Alt, Shift, Ctrl and Insert for forcing, and Alt, Shift, Ctrl and Delete for bypassing. Combinations must differ. New default behavior uses Ctrl to force and Alt to bypass; explicitly saved preferences stay intact.
- The require-link-click option works. When it is disabled, held keys can cover a delayed script download. The gesture binds to the request at its start, before a browser download popup can take focus. It remains restricted to that request and document, expires, and cannot enable browser capture or bypass site rules. Typed text is never collected.
- The skip-web-resources option stops forced capture of recognized pages, scripts and images. Ordinary file-type capture retains its existing rules.
- Options > General > Edit opens separate Chromium-family and Firefox-family context-menu switches. Download with UDM controls link/media/selected-link entries; Download all is independent. Existing pages update after desktop-policy synchronization. Stale disabled menu clicks are rejected. A menu-rebuild race found in real Edge testing was fixed.
- Options > General > panel customization opens Web players, Selected text, and Position and display tabs. File-type checkboxes, Add, Minimum size, Check All, Clear All, wildcard site exceptions, full/mini mode, hover, placement, menu width and reset controls save real preferences.
- Video menus apply file-type and known-size rules to direct files, YouTube choices and HLS/DASH output containers. Playback byte ranges use the full resource size, not the size of a downloaded fragment. Unknown sizes remain visible. Cached offers are checked again before handoff, including settings changes during retrieval.
- Optional automatic player capture starts an identified, playing, unprotected whole-file video only when browser capture is also enabled. It checks the actual player, document and captured GET request, respects exclusions/bypass/type/size settings, and prevents repeated handoffs. Blob/adaptive players, fragments, known ad markers and protected media are excluded from this automatic path. Adaptive formats still use their menu.
- The protected-content checkbox controls panel visibility only. It does not enable protected-media downloads.
- Both browser source packages are 0.38.0; extension preparation preserves the shared keyboard module. UDM's desktop/native host are 0.54.0.

IDM's documented Ctrl-force/Alt-bypass convention and separate customization controls informed the implementation. Reference resources were used to compare selected control positions; UDM's code is independently implemented. Sources: [IDM starting downloads](https://www.internetdownloadmanager.com/support/using_idm/starting.html), [IDM video capture keys](https://www.internetdownloadmanager.com/register/new_faq/video9.html), [IDM options](https://www.internetdownloadmanager.com/support/using_idm/options.html).

## Verification

**1,720 behavioral checks passed:** 1,040 native; 584 browser regressions; 10 context-menu scenarios; 8 native protocol; 8 isolated app/host; 32 live panel/keyboard checks across Chrome and Edge; 28 full browser/native scenarios; and 10 optional player-capture scenarios.

Real Chrome and Edge extensions each completed byte-identical MP4 and ordinary file downloads, recorded HLS with audio, trusted force/bypass actions and delayed no-click capture through the rebuilt native host and app. Separate tests verified optional direct-player capture, opt-in enforcement and duplicate prevention. All tests used isolated catalogs/profiles and local synthetic media. They are not a throughput benchmark or public-site compatibility claim.

The settings survived native serialization, validation and persistence checks. Desktop policy used the verified native parent, rather than a supplied browser name. Native and browser tests rejected stale documents, expired or released keys, synthetic keyboard events, disabled types, undersized known files, stale selections and private contexts.

45 selected literal control rectangles match reference resource coordinates. The audit discloses 13 omitted dynamic/preview/legacy controls; it does not count them as matches. Native rendered appearance, text fitting, keyboard traversal and mixed-DPI behavior remain unverified: the Windows control tool still fails before initialization with a kernel-assets path error. Browser screenshots were inspected; the HLS format list and footer controls are visible in the recorded Edge fixture.

Receipts: [summary](evidence-0.54.0/summary.json), [workflow inventory](idm-parity-0.54.0.md), [Edge HLS screenshot](evidence-0.54.0/edge-hls-panel.png).

## Remaining acceptance

Full IDM parity remains unestablished. Public YouTube and other video-platform behavior, all driver behaviors, a fresh controlled ISO-speed comparison, clean-machine installation and publisher signing still need work. Firefox has shared-source/policy/packaging coverage in this update, not a new real-profile test. Existing personal Chrome/Edge profiles still need the updated unpacked extension loaded and their pages refreshed; isolated browser success does not establish their activation.

The signed network runtime is unchanged. No test-mode or driver-signing settings were changed for this release. The new dialogs retain native Cancel/OK draft behavior; visual acceptance is explicitly outstanding.

## Current-PC deployment

[UDM 0.54.0 installer](../installer-out/UDM-0.54.0-Browser-0.38.0-Setup-x64.exe) was built. 97 files were deployed with SHA-256 verification and original-file backups. Installer SHA-256: `E2F99BA0977C297B2DED4A26A9493B60EA0A6590F89A0CBB4C5BC6F182060F79`.

The running desktop/native bridge reports 0.54.0 and the correct D:/UDM/user-data catalog. All 23 records, queues and saved preferences are unchanged. Both browser source packages are 0.38.0. The legacy Chromium folder is a junction to D:/UDM/browser/chromium and therefore receives the update too. Existing personal browser sessions still need a verified extension reload; no live Firefox acceptance is claimed. The signed network runtime is unchanged.
