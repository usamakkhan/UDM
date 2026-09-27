# UDM 0.13.0 — workflows from the live IDM review

This release implements the desktop and selected-link workflows identified in the live review. The native app, host and monitor remain original C++17/MFC code; the browser extension remains JavaScript. The 103-item audit is a historical baseline, not a claim that this release completes every item.

## What changed

| Workflow | New behavior |
|---|---|
| Completed-file properties | Address, description, parent page, Referer and authorization can be edited. Cross-origin address changes discard old authentication, cookies and Referer. File movement has its own action. |
| Move/Rename | Available from Properties, the File/context menus and Ctrl+M. Supports movement across volumes; refuses existing files and destinations reserved by another job. A recovery journal handles interruption, and failed state writes roll back a completed move. |
| Open with | Available in completed Properties, File/context menus and the completion dialog. Opens Windows’ application chooser. Mixed-slash paths are normalized before the shell call. |
| Redownload | Creates a fresh, numbered copy with the original description, source page, credentials and connection preferences. The existing completed file remains intact. Captured media requires fresh browser streams. |
| Completed double-click | Context menu offers Open or Properties. Properties remains the default requested in this project. |
| List columns | Twelve columns, including Date added, Save to, Referer and Parent web page. Visibility, width, order and sorting survive restart. File Name remains visible. |
| Find Next | Ctrl+F opens a dedicated Find row. Search covers filename, address, description and source page. F3 advances through matching rows and wraps. |
| Appearance | View offers a dark content theme, a Windows font chooser, category-pane visibility and UDM/Windows/hidden tray-icon choices. Preferences persist. Windows menus, title bars and some standard controls retain their system styling. |
| File Info prefetch | Regular HTTP/HTTPS downloads start receiving bytes while File Info is open, when the preference is enabled and capacity is available. Publication waits for confirmation. Cancel/Download Later stops the transfer and retains saved parts; Start validates and resumes them. Confirmation waiting is excluded from network-time metrics. |
| Connection settings | Up to 32 HTTP workers; per-host limits for newly added downloads. Existing jobs retain their per-file connection settings. The default remains eight connections. |
| Temporary storage | A separate temporary-files folder can be selected. Existing partial directories stay where they are; new jobs persist their chosen partial-directory path. |
| Quota | A configurable MB limit over 1–168 hours, replacing the fixed one-hour period. |
| Server date | Optional use of HTTP Last-Modified for the completed file’s creation and last-write timestamps. If no valid server date exists, the ordinary filesystem timestamp remains. |
| Queue membership | Pending membership is separate from the stored queue name. Completed history is absent from Scheduler’s pending list. Removing membership leaves the record and file in history; starting the queue skips removed records. |
| Queue progress | Queue-originated progress can start minimized; the preference is enabled by default. |
| Selected-text browser panel | On permitted sites, selecting text containing hyperlinks displays a separate UDM panel. It deduplicates HTTP/HTTPS links, provides checkboxes for review, supports compact/full display and all/off/listed-site settings, and honors excluded hosts. No new host permission is requested by this update. |

The Windows chooser uses the documented [SHOpenWithDialog API](https://learn.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-shopenwithdialog). MFC’s generic server-busy prompts are suppressed during that expected modal shell operation.

## Validation

- **30 browser integration checks passed**, including selection deduplication, private-tab rejection, source-page validation and validating the complete selection before any native handoff.
- **7 selected-link checks passed in isolated real Chrome**, covering selection boundaries, duplicate and unsafe-link exclusion, explicit confirmation, unchecked items, compact mode, site-specific preferences, excluded sites and selection clearing.
- **16 existing real-Chrome video-panel checks passed**.
- Extension preparation retained the stable extension identity, manifests and shared sources.
- Live native UI review verified Properties and Move/Rename layout, the Windows chooser, Columns, saved Date added visibility, an empty pending queue despite completed history, persisted dark content colors, description search and F3 navigation.

The deployed app displays all 10 existing records (7 complete, 3 failed) and 0 active downloads. Every pre-existing field in every download record remains unchanged; the migration adds queue-membership and confirmation fields. The registered native host passed a real length-prefixed ping. See [machine-readable results](reference/workflows-0.13.0.json).

## Deployment and boundaries

The installed Chromium extension’s old path is a directory junction to `D:\UDM\browser\chromium`; it therefore already resolves to the updated 0.13.0 files. Chrome is running in Windows session 2, while UDM and computer control are in session 1. The files are updated, but the running Chrome extension has not been reloaded. Its native-host manifest already points to `D:\UDM\release\Udm.NativeHost.exe`. No reinstallation or permission expansion was performed. Updated content scripts take effect on the next browser launch/page load; an already-open browser would need an extension reload and page refresh.

This release does not establish Internet speed parity with IDM. It does not implement duplicate overwrite/resume policies, periodic synchronization queues, complete toolbar/skin customization, all live video platforms, or a production driver. The existing WFP driver was not changed or installed. File Info prefetch applies to regular HTTP/HTTPS jobs, not FTP or captured adaptive media. Redownload deliberately creates another file rather than replacing an existing completed file.

Backups of the pre-change source and binaries are under `backups/before-0.13.0`; the original user-history snapshot is under `benchmarks/workflows-0.13.0/history-before.json`. Tests use disposable files and isolated state.
