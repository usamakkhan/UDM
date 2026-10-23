# GUI parity gap list: IDM 6.43 build 10 vs UDM (2026-10-18)

Scope: the installed Internet Download Manager 6.43 build 10 (trial, dark mode on, Windows 11, 125 % DPI) was launched and every window
reachable from its main menu was captured with our own screenshot tooling (window rectangle copy plus a UI Automation control dump with
positions and sizes). No IDM resources, bitmaps or artwork were extracted. UDM was compared from the current repository sources
(`native/App.cpp`, `native/ToolbarUi.hpp`, `native/ToolbarModel.hpp`, `native/MainFeatures.hpp`, `native/DesktopUi.hpp`, `native/Ui.hpp`,
`native/SelectionUi.hpp`, `native/ProgressUi.hpp`, `native/OptionsUi.hpp`, `native/OptionsPages.hpp`, `native/SchedulerUi.hpp`,
`native/GrabberUi.hpp`) with screenshots of the installed UDM 0.87.0 as supporting evidence where the dialog has not changed since.

Evidence folder: [gui-parity-2026-10-18/](gui-parity-2026-10-18/). Every `idm-*.png` / `udm-*.png` has a matching `*.uia.txt` listing each
control with `x,y WxH` relative to the window in physical pixels (125 % DPI, so divide by 1.25 for dialog pixels at 96 DPI). The full IDM menu
tree with command identifiers is in [idm-main-menu.txt](gui-parity-2026-10-18/idm-main-menu.txt) and UDM's in
[udm-main-menu.txt](gui-parity-2026-10-18/udm-main-menu.txt).

Status legend: **Done** = same control, layout and behaviour; **Partial** = present but text, layout or behaviour differs (difference named);
**Missing** = no equivalent in UDM; **UDM extra** = UDM shows something IDM does not. "Size" is the IDM control size from the UIA dump
(physical pixels at 125 %), with the UDM size when it differs. The trial build shows "This feature is available in registered version only"
for Delete All Completed, so that confirmation flow could not be observed.

Items marked **Done (this branch)** were changed in this branch for the main window; see the end of the document.

## 1. Main window ([idm-main.png](gui-parity-2026-10-18/idm-main.png), [idm-main-wide.png](gui-parity-2026-10-18/idm-main-wide.png), [udm-main.png](gui-parity-2026-10-18/udm-main.png))

After-state of this branch: [udm-main-after.png](gui-parity-2026-10-18/udm-main-after.png) (built from this worktree; the Grabber button
is still present there only because the copied test profile carried an explicit saved toolbar layout).

IDM window: 949x561 default on this profile, no status bar, no find bar. Client layout top to bottom: menu bar, toolbar (71 px tall), then
the "Categories" pane (161 px wide, header 86x16 "Categories" plus a 16x16 "x" close button) beside the download list.

| Control | IDM behaviour | UDM state | Size (IDM / UDM) |
|---|---|---|---|
| Title bar | "Internet Download Manager 6.43" | Partial: "UDM Download Manager" (branding, intentional) | - |
| Menu bar | Tasks, File, Downloads, View, Help, Registration | Done except Registration (intentionally excluded); all submenu entries and mnemonics match except Language (English only, agreed) and the Start/Stop queue submenus (IDM lists each queue; UDM lists "Start" and "Choose queue...") | - |
| Toolbar buttons | Add URL, Resume, Stop, Stop All, Delete, Delete Completed, Options, Scheduler, Start Queue (dropdown), Stop Queue (dropdown), Tell a Friend; Grabber only in Customize | Done (this branch): default layout dropped the extra Grabber button, labels "Delete Completed" and "Tell a Friend" match IDM casing | 931x71 / 950x79 |
| Toolbar look | Large neon-style outline icons (about 32 px) with caption below, truncated with "..." at fixed button width, toolbar flat on dark background | Partial: UDM redraws its own icon set (allowed), size, captions and truncation match; IDM also offers "3D Style" skin and "Look for new toolbars" entries which UDM replaces with Load/Open/Reload toolbar skin commands | - |
| Toolbar menu (View > Toolbar) | Customize..., Large Buttons, Small Buttons, Look for new toolbars, Neon, 3D Style | Partial: UDM has Customize, Large/Small/Medium, Icons and text/Icons only/Text only, UDM icons, skin list, Load/Open/Reload, Hide toolbar | - |
| Categories pane header | "Categories" static with "x" close button at the right edge | Done | 86x16 + 16x16 / 146x23 + 22x21 |
| Categories tree | All Downloads (expanded: Compressed, Documents, Music, Programs, Video), Unfinished, Finished, Grabber projects, Queues (collapsed) | Done (this branch) for the node set apart from Images: Queues start collapsed, the fallback "Other" category no longer gets a tree node (its records show under All Downloads, as in IDM); Partial: UDM keeps an Images built-in category that IDM does not have (hiding it by default would re-route existing profiles, so it stays a user choice via Options > Save to) | 161x378 / 170x377 |
| Tree icons | Folder-style icons per node, queue nodes show queue state | Done (UDM-drawn equivalents) | 16x16 |
| List columns | File Name, Q, Size, Status, Time left, Transfer rate, Last Try Date, Description (+ hidden Date Added, Save To, Referer, Parent web page; see [idm-customize-url-list-bottom.png](gui-parity-2026-10-18/idm-customize-url-list-bottom.png)) | Done (this branch): hidden column names now use IDM casing ("Date Added", "Save To") | header 24 px tall |
| List row icon | Shell file-type icon of the file (exe, zip, pdf...) | Done (this branch): rows use the shell icon for the file extension, category icon as fallback | 16x16 |
| Q column | Queue icon when the record is in a queue | Done | 22 px wide |
| Size text | "443.72 KB", "3.95 GB"; blank when unknown; stream length ("2 hour(s) 3 min") for media | Done (this branch) for blank unknown size; Partial: media length instead of size not shown | - |
| Status text | "Complete", "Error", or progress "0.24%" / "100.00%" for unfinished records | Done (this branch): two-decimal percentage for Downloading/Paused/Queued, "Error" for failed | - |
| Time left | "19 min 26 sec", "0 sec"; blank when idle | Done (this branch) | - |
| Transfer rate | blank when idle | Done (this branch): blank instead of "--" | - |
| Last Try Date | "Apr 20 11:30" style: month abbreviation, day, 24-hour time ([idm-main-columns-wide.png](gui-parity-2026-10-18/idm-main-columns-wide.png)) | Done (this branch) | - |
| Grid | Full row select, vertical and horizontal grid lines, dark header | Done | row 21 px |
| Status bar | none | UDM extra: UDM showed "N downloads / N active / rate / quota". Done (this branch): hidden unless a storage error or shutdown message must be shown | 0 / 24 px |
| Find bar | none (Find is a dialog) | UDM extra: Ctrl+F opens the Find dialog like IDM; an inline quick-filter field above the list is only shown after Ctrl+Shift+F or View > Quick filter | - |
| Download context menu ([idm-context-menu.png](gui-parity-2026-10-18/idm-context-menu.png)) | Open, Open with..., Open folder, sep, Move/Rename (Ctrl-M), sep, Redownload, sep, Resume Download, Stop Download, sep, Refresh download address, Remove, sep, Move to queue (submenu listing every queue), Delete from queue, sep, On Double click (submenu), sep, Properties; 288x432 | Done (this branch): same order, labels and separators; "Move to queue" is a submenu of queue names; UDM extras sit beside their IDM group (ZIP contents... after Open folder, Show progress after Stop Download, Recycle downloaded file... after Remove, Move up/down in queue after Delete from queue) | 288x432 |
| Drop target | Floating 59x70 window "IDM drop target" toggled from Tasks | Done (UDM "drop basket", own artwork) | 59x70 |

## 2. Add URL ([idm-add-url.png](gui-parity-2026-10-18/idm-add-url.png), [udm-add-url.png](gui-parity-2026-10-18/udm-add-url.png))

| Control | IDM behaviour | UDM state | Size |
|---|---|---|---|
| Dialog "Enter new address to download" | 659x149, address combo with history, authorization group | Done (671x161 including frame difference) | 659x149 |
| Address label + combo | "Address" 61x18, combo 462x24 at 82,46 prefilled with clipboard URL | Done; UDM prefills from the download history and clipboard | 462x24 |
| Use authorization group | Group 529x58, checkbox 123x16, Login edit 137x28, Password edit 131x28 disabled until checked | Done | 529x58 |
| OK / Cancel | 88x26 at 556,46 and 88x28 at 556,80 | Done | 88x28 |

## 3. Batch download ([idm-batch.png](gui-parity-2026-10-18/idm-batch.png), [udm-batch.png](gui-parity-2026-10-18/udm-batch.png))

| Control | IDM behaviour | UDM state | Size |
|---|---|---|---|
| Dialog "Batch download" | 559x511 | Done (this branch): "Batch download" 316x238 dialog units, the same frame offset as Add URL (`batchDialog` in `native/SelectionUi.hpp`) | 559x511 |
| Explanation text | 529x74 static describing img*.jpg wildcard | Done (this branch): same two-paragraph text with a neutral example address | 529x74 |
| Address combo | 473x24 with history, single pattern with asterisk | Done (this branch): combo prefilled with the last 25 download addresses; `[001-100]` bracket ranges still work when no asterisk is given | 473x24 |
| "Replace asterisk to" group | Numbers / Letters radios, From / To spin edits, Wildcard size spin | Done (this branch): Numbers 0-100 with wildcard size 2 by default, Letters switches to a-z and disables the size spin; at most 1,000 files | 529x110 |
| Use authorization group | Same as Add URL | Done (this branch): Login / Password enabled by the checkbox and passed to every file of the batch | 529x58 |
| First file / Second file / ... / Last file preview | Three read-only statics updated live | Done (this branch): previews follow the address, range and wildcard size as they are typed; OK still opens the UDM selection list to choose files, queue and folder | 422x22 each |
| OK / Cancel | 88x28 | Done (this branch) | 88x28 |

## 4. Download File Info (start dialog) and Download progress

The start dialog was captured by adding a test URL ([idm-file-info.png](gui-parity-2026-10-18/idm-file-info.png), 671x291; a second
capture with a resolved 100 MB size is [idm-file-info-100mb.png](gui-parity-2026-10-18/idm-file-info-100mb.png)) and the
complete dialog by downloading Sysinternals PSTools.zip ([idm-download-complete.png](gui-parity-2026-10-18/idm-download-complete.png);
that record and file now sit in IDM's list and Downloads\Compressed). The progress window could not be captured: the first test server
answered 403 (IDM's error box is in [idm-download-error.png](gui-parity-2026-10-18/idm-download-error.png), 544x258), the 4.9 MB
download then finished before the capture fired, and a benchmark in another session kept launching and terminating the IDM GUI instance
between retries. A later 100 MB test download (100MB.bin from ash-speed.hetzner.com, started through the File Info dialog's Start
Download button) completed in IDM without any progress window appearing, so the installed profile appears to have the progress dialog
suppressed; that record now also sits in IDM's list. The progress rows below therefore come from IDM's documented layout and remain to be
verified against a capture. UDM's dialogs: `presentDownload` in `Ui.hpp`/`DownloadInfoUi.hpp`, `Progress` in
`ProgressUi.hpp`, completion in `App.cpp` (`complete`).

| Control | IDM behaviour | UDM state | Size |
|---|---|---|---|
| File Info dialog | 671x291: URL edit (427x28), Category combo (203x24) + "+" button, Save As combo (390x24) + "...", "Remember this path for <category> category" group with folder edit, Description edit, file-type icon and Preview button on the right, Download Later / Start Download / Cancel (130x28) | Done (DownloadInfoUi: same fields plus More/Less, destination history, size preview, Download Later queue panel; needs a side-by-side capture of UDM) | 671x291 |
| Progress title | "<percent> <file name>" with tray icon mirror | Done ("Download progress" caption updates with percent; tray mirror implemented) | 353x250 dialog units |
| Status block | URL, Status, File size, Downloaded, Transfer rate, Time left, Resume capability | Done | - |
| Progress bar and segment map | Overall bar plus per-connection segment map, "N." / "Downloaded" / "Info" list in details | Done (RangeMap, connection list) | - |
| Tabs | Download status, Speed Limiter, Options on completion | Done, plus UDM "Information" tab and per-tab Hide buttons | - |
| Buttons | Pause / Cancel, "<< Hide details", Start/Resume | Done | - |

## 5. Download complete ([idm-download-complete.png](gui-parity-2026-10-18/idm-download-complete.png), [udm-download-complete.png](gui-parity-2026-10-18/udm-download-complete.png))

| Control | IDM behaviour | UDM state | Size |
|---|---|---|---|
| Dialog | 536x299: 40x40 file-type icon, two-line text "Download complete / Downloaded 4.90 MB (5144350 Bytes)", Address edit 506x28, "The file saved as" edit 506x28, Open / Open with... / Open folder / Close (107x28), "Don't show this dialog again", 32x24 drag handle bottom right | Done: resource dialog 286 geometry (unchanged), the file type's shell icon and IDM's two-line text; `CompletionDialogUiTests` 106 checks | 536x299 |
| Virus check line | Shown when a scanner ran | Done (extra 33 px row) | - |

## 6. Options ([idm-options-1..9.png](gui-parity-2026-10-18/idm-options-1.png), [udm-options-1..9.png](gui-parity-2026-10-18/udm-options-1.png))

IDM: "Internet Download Manager Configuration" 576x653, two rows of tabs (General, File types, Save to, Downloads, Connection / Proxy / Socks,
Sites Logins, Dial Up / VPN, Sounds), each page 546x516 with a 40x40 icon and a right-aligned heading over a rule, OK and Help buttons 100x28.
UDM (this branch): "UDM Configuration" 326x309 dialog units (the same frame as IDM's 576x653), tab control and page area at IDM's positions,
OK + Help; the former Cancel / Apply are gone (closing the window discards changes) and "More..." moved onto the Connection page.

| Tab / control | IDM behaviour | UDM state | Size |
|---|---|---|---|
| Dialog frame and tabs | 576x653, tabs in two rows, OK + Help | Done (this branch): 326x309 dialog units, tab control 315x283 at 5,4, pages 311x258 at 7,27, OK + Help 57x14 at 202/263,291 (`native/OptionsUi.hpp`); Help opens the UDM help contents | 576x653 |
| General | Advanced browser integration line + Restart; Launch on startup; Auto-download clipboard URLs; "Run module for monitoring IE-based browsers"; browser capture list with Add browser...; Keys...; context-menu Edit...; download-panel Edit... | Done (this branch) for geometry: every control sits at IDM's position and size; UDM's "Enable browser download capture" checkbox takes the IE module row (IE not supported) | list 482x150 |
| File types | Auto-download extension list, exclusion sites list, Default buttons, "Edit list ..." for addresses | Done (this branch): IDM positions, sizes and captions; UDM keeps "Site rules..." beside "Edit list ..." and the two extra checkboxes below the rule | edit 508x72 |
| Save to | Category combo + New/Edit, file types line, default directory + Browse, "Change folder ... on last selected", set file date, Temporary directory + Browse + hint | Done (this branch): IDM geometry | group 506x256 |
| Downloads | Customize progress dialog Edit..., Show start dialog (+ only add to queue), Show complete dialog, note, start immediately, two queue-panel checkboxes, ignore modification time, duplicate combo, User-Agent, Virus checking Edit... | Done (this branch): IDM geometry and note wording | - |
| Connection | Default max. conn. combo (8), Exceptions list Server/Number with New/Delete/Edit, Download limits group (MBytes every N hours, warning checkbox) | Done (this branch): IDM geometry; UDM adds "Use TLS 1.3 when supported" and a "More..." button for the additional UDM settings below the limits group | list 374x150 |
| Proxy / Socks | Get System, use browser proxy, No proxy / System / Auto-config script + Address / Manual (address, port, user, password, http/https/ftp, Advanced / Socks...), Use FTP in PASV mode | Done (this branch): IDM geometry including the rules between the radio rows and no heading rule; UDM adds "Do not use proxy for addresses beginning with" and an FTP proxy type combo | - |
| Sites Logins | List Site/path, User, Password; New / Edit / Remove | Done (this branch): IDM geometry; UDM adds "Use the current Windows login for intranet servers" | list 510x344 |
| Dial Up / VPN | Use Windows Dial Up, Connection combo, User name, Password, Save password, Apply; Redial attempts and interval | Done (this branch): IDM geometry (`native/DialCredentialsUi.hpp`); UDM adds Connect..., More... and a status line | - |
| Sounds | Event list (Download complete, Download failed, Queue started, Queue stopped/finished) with Sound file column, Browse..., Play | Done (this branch): IDM geometry; UDM lists its own three events | list 488x308 |
| Page icons | 40x40 themed icons per tab | Partial: UDM uses system stock icons (allowed replacement, different look) | 40x40 |

## 7. Scheduler ([idm-scheduler-schedule.png](gui-parity-2026-10-18/idm-scheduler-schedule.png), [idm-scheduler-files.png](gui-parity-2026-10-18/idm-scheduler-files.png), [udm-scheduler-1.png](gui-parity-2026-10-18/udm-scheduler-1.png), [udm-scheduler-2.png](gui-parity-2026-10-18/udm-scheduler-2.png))

| Control | IDM behaviour | UDM state | Size |
|---|---|---|---|
| Dialog | 839x607, queue tree left (201x466) under a 40x40 icon and "Queues" label, selected queue name heading, tab control (Schedule, Files in the queue) | Done (778x619; UDM adds an "Automation" tab) | 839x607 / 778x619 |
| Queue tree | Main download queue, Synchronization queue, Download limits | Done: a "Download limits" node shows IDM's dialog-309 page (enable, MBytes every hours, warning) editing the shared Options quota; queue actions disable while it is selected. Queue names still differ (Main queue) | 201x466 / 202x466 |
| Schedule tab | One-time downloading / Periodic synchronization radios; Start on IDM startup; Start download at + time; Once at + date / Daily + weekday checks; Stop download at + time; Number of retries + edit; Open the following file + ... ; Hang up modem; Exit IDM; Turn off computer + Shut down combo; Force processes | Done (same controls and order; UDM uses 24-hour "00:00" edits where IDM uses time pickers with spin) | - |
| Files in the queue tab | "Download N files at the same time", file list (File name, Size, Status), Move up / Move down / Remove from queue buttons | Done; UDM move buttons are narrow 27 px arrow buttons instead of 88 px captions | list 462x362 |
| Bottom buttons | New queue, Delete, Start now, Stop, Help, Apply, Close (88x28) | Done | 88x28 |

## 8. Site Grabber wizard ([idm-grabber-1.png](gui-parity-2026-10-18/idm-grabber-1.png), [udm-grabber-1.png](gui-parity-2026-10-18/udm-grabber-1.png))

| Control | IDM behaviour | UDM state | Size |
|---|---|---|---|
| Window | "IDM Site Grabber. Step 1 --- Set start page" 717x475 with its own menu bar (Project, Options, Help) and a 149x320 picture on the left | Partial: UDM "Site Grabber" 938x747 single window with "Saved project" combo + New on top, numbered step heading, no menu bar, no picture | 717x475 / 938x747 |
| Step 1 fields | Grabber Project Name, Start page/address, Project template combo + explanation, Use authorization (Login/Password), Advanced >> (manual login page, Don't open the logout page) | Done: Project name, Starting web page, Template + Templates..., login checkbox with user name/password, Advanced... | - |
| Navigation | < Back, Next >, Close (105x28) | Done; UDM adds Save project and Templates... buttons | 105x28 / 118x31 |
| Steps 2-4 | Step 2 save location, Step 3 explore depth and filters, Step 4 file filters, then exploration window | Done as UDM steps 2-4 (destination, exploration, files) and result list with Explore/Resume/Stop/Check all/Properties/Errors/Statistics (not captured: Alt+N did not advance the IDM wizard from automation) | - |

## 9. Columns dialog (View > Customize URL List...) ([idm-customize-url-list.png](gui-parity-2026-10-18/idm-customize-url-list.png), [udm-columns.png](gui-parity-2026-10-18/udm-columns.png))

| Control | IDM behaviour | UDM state | Size |
|---|---|---|---|
| Dialog "Columns" | 419x389 | Done (this branch): 236x177 dialog units with IDM control positions (`customizeColumns` in `native/MainFeatures.hpp`) | 419x389 |
| Instruction text | "Check the columns that you would like visible in this list. Use the Move Up and Move Down buttons..." | Done (this branch): same wording | 389x50 |
| Column list | Checkbox list 243x174, single column | Done (this branch): single headerless checkbox list; opens without a selection like IDM | 243x174 |
| Buttons | Move Up, Move Down, Show, Hide, Reset (124x28) | Done (this branch): IDM casing, no extra button | 124x28 |
| Width | "The selected column should be [ ] pixels wide" inline | Done (this branch): inline sentence; the typed width applies to the selected column as it is typed and is validated on OK | 35x28 edit |
| OK / Cancel | 88x28 | Done | - |

## 10. Customize Toolbar ([idm-toolbar-customize.png](gui-parity-2026-10-18/idm-toolbar-customize.png), [udm-toolbar-customize.png](gui-parity-2026-10-18/udm-toolbar-customize.png))

| Control | IDM behaviour | UDM state | Size |
|---|---|---|---|
| Dialog | Standard toolbar customization 720x285: Available (240x200) / Current (240x200) lists with icons, Add ->, <- Remove, Close, Reset, Move Up, Move Down | Done (732x297, native TBN customization) | 720x285 |
| Default contents | Available: Separator, Grabber; Current: eleven buttons + trailing Separator | Done (this branch) | - |

## 11. Find file ([idm-find.png](gui-parity-2026-10-18/idm-find.png), [udm-find.png](gui-parity-2026-10-18/udm-find.png))

| Control | IDM behaviour | UDM state | Size |
|---|---|---|---|
| Dialog | 409x331: Find: edit, three scope checkboxes, rule, Match case, Match whole string only, Find / Cancel | Done (421x343, identical control positions) | 409x331 |

## 12. Speed Limiter settings ([idm-speed-limiter.png](gui-parity-2026-10-18/idm-speed-limiter.png), [udm-speed-limiter.png](gui-parity-2026-10-18/udm-speed-limiter.png))

| Control | IDM behaviour | UDM state | Size |
|---|---|---|---|
| Dialog | 535x213: "Maximum download speed for one file" edit + KBytes/sec, warning note, "Always turn on Speed Limiter on IDM startup", OK / Cancel | Done; label reads "Maximum total download speed" (UDM limits the total) and the note is reworded | 535x213 |

## 13. Tip of the Day, About, Delete-completed prompt, drop target

| Window | IDM behaviour | UDM state | Size |
|---|---|---|---|
| Tip of the Day | 449x363 picture panel, "Did you know...", tip text, Show Tips on StartUp, Next Tip, Close | Done (461x375; heading reads "Do you know..." ) | 449x363 |
| About | 461x503: icon, name, "Version: 6.43 Build 10 (date) Trial", Check for Update, "This product is licensed to" group with Purchase/Registration, copyright warning, Home Site, Support Team, Close | Done: 461x503 layout with the build date in the version line, a licensing group stating UDM is free software (License / Source code in place of Purchase / Registration), the legal notice, Home Site, Support Team and copyright lines; `AboutUiTests` (`build.ps1 -AboutUiTestsOnly`) | 461x503 |
| Delete All Completed | Trial shows "This feature is available in registered version only." (364x182) | n/a: UDM removes completed records after a confirmation | - |
| Drop target | 59x70 floating always-on-top window | Done | 59x70 |

## Main-window work done in this branch

1. Toolbar defaults follow IDM: eleven buttons (Add URL ... Stop Queue, Tell a Friend); Grabber remains available in Customize Toolbar.
   Captions use IDM casing ("Delete Completed", "Tell a Friend"). `native/ToolbarModel.hpp`, tests in `native/ToolbarChecks.hpp` and
   `native/ToolbarUiTests.cpp`.
2. Download list cells follow IDM formats: shell file-type icons per row, two-decimal progress in Status for unfinished records, "Error"
   for failed records, "19 min 26 sec" countdowns, blank Size/Time left/Transfer rate when unknown or idle, "Aug 05 09:14" dates.
   `native/App.cpp` (`statusText`, `idmDuration`, `dateText`, `fileImage`).
3. Status bar hidden like IDM; it only appears for a storage error or the "Saving downloads and closing..." message (`showStatus`).
4. Categories tree starts with only All Downloads expanded (Queues collapsed), as in IDM.
5. Hidden list columns are named "Date Added" and "Save To" as in IDM's Columns dialog (`native/MainFeatures.hpp`).
6. Download context menu follows IDM's row menu (captured with a selected completed row): Open, Open with..., Open folder | Move/Rename
   (Ctrl-M) | Redownload | Resume Download, Stop Download | Refresh download address, Remove | Move to queue (submenu listing the queues
   directly, command ids 34000+ in `native/QueueMenus.hpp`), Delete from queue | On Double click | Properties. UDM-only actions keep their
   place next to the related IDM group. `fillDownloadMenu` in `native/App.cpp`.
7. The fallback "Other" category has no tree node under All Downloads, Unfinished or Finished; IDM shows uncategorised records only under
   the parent nodes. The category itself still exists for routing, Options and the category editor (`buildTree` in `native/App.cpp`).

Verification: `native/build.ps1 -AppOnly` builds, the toolbar UI harness (`-ToolbarUiTestsOnly`, `Udm.ToolbarUiTests.exe` with the
fixture from `tests/toolbar-fixtures.py`) passes all 268 checks including the status-bar, tree-expansion, cell-format, file-icon,
category-node and context-menu-order checks, and the menu harness (`-MenuStateTestsOnly`, `Udm.MenuStateTests.exe`) passes all 380 checks.

Remaining main-window items (not done): media length in the Size column (UDM stores no stream duration for a record), the UDM-only
quick-filter field (opt-in, hidden by default), the Images built-in category (IDM has none; hiding it by default would re-route existing
profiles), and IDM's "3D Style" toolbar skin and "Look for new toolbars" entries.

## Dialog work done in this branch

1. Batch download (`batchDialog` in `native/SelectionUi.hpp`): IDM's 559x511 layout with the explanation text, Address combo with
   history, "Replace asterisk to" group (Numbers / Letters, From / To / Wildcard size spin edits), Use authorization group, live
   First / Second / Last file previews and OK / Cancel. `batchPattern` expands the asterisk; OK still opens the UDM selection list
   (`selectBatch`) with the authorization header applied to every file. Harness: `native/BatchDialogUiTests.cpp`
   (`build.ps1 -BatchDialogUiTestsOnly`).
2. Columns (`customizeColumns` in `native/MainFeatures.hpp`): IDM's 419x389 layout with a single headerless checkbox list,
   Move Up / Move Down / Show / Hide / Reset, the inline "The selected column should be [ ] pixels wide" sentence and OK / Cancel;
   opens without a selection like IDM. Harness: `native/ColumnControlsUiTests.cpp` (`build.ps1 -ColumnControlsUiTestsOnly`).
3. Options (`native/OptionsUi.hpp`, `native/OptionsPages.hpp`, `native/DialCredentialsUi.hpp`): IDM's 576x653 frame with OK + Help,
   tab control and nine pages at IDM's control positions; UDM's extra settings stay on their pages and the "More..." dialog moved to
   the Connection page. Harnesses: `native/OptionsTabsUiTests.cpp` (`build.ps1 -OptionsTabsUiTestsOnly`) and
   `native/DialCredentialsUiTests.cpp` (`-DialCredentialsUiTestsOnly`).
