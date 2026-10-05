# UDM 0.79.0 candidate — import/export and explicit Resume

Verified 30 September 2026. **Source and the installer are delivered; the running desktop remains 0.78.0.** Browser files remain 0.53.1. Activating this candidate requires restarting UDM. Full IDM parity is not established.

## Changes

- Tasks now groups Export and Import as submenus with separate IDM-file, text-file and UDM-catalog routes. Batch download from clipboard and the drop target occupy the observed reference Tasks positions.
- The EF2 reader/writer preserves each URL's Referer, Cookie, form data (`pd`) and User-Agent. It validates the complete input before import, keeps selected files paused, stores request context using Windows-account encryption, and requires an explicit checkbox before using or exporting saved cookies/form data. EF2 output itself is plain text.
- Text-file import extracts HTTP, HTTPS and FTP addresses from surrounding text, quoted strings and HTML. Add Batch retains its existing sequence-expansion workflow.
- Catalog duplicate matching includes the POST body, preventing distinct forms at one URL from collapsing into one record.
- Explicit Resume starts the selected file while its queue is disabled or outside its scheduled window. Other files remain stopped, the global parallel limit applies, and Grabber retains its per-project limit. Individual starts do not arm queue completion actions.
- About, file metadata and native protocol version identify 0.79.0 consistently.

## Evidence

The installed reference is IDM 6.43 build 10, SHA-256 `03cc62e9adb77a380f9dc12f67ccaaee5106f12844aa73ce32c914ddd16d607c`. Its menu resource 129 and reader at `00550270` establish the observed file choices, delimiters and four request fields. The [format literals](evidence-candidate-0.79.0/reference-evidence.json) are retained. UDM uses an independently written parser and serializer.

IDM documents extracting links from text before selection ([Import downloads](https://www.internetdownloadmanager.com/support/import_downloads.html)). Its documented complete backup/transfer procedure separately preserves temporary/download folders and a registry export ([backup guidance](https://www.internetdownloadmanager.com/register/new_faq/functions17.html)); an EF2 list is not a full backup.

- 2,462 native checks passed, including 59 focused import/export and individual-start checks. The actual HTTP fixture received exactly one POST, no GET/HEAD/range probes, correct request headers and body, and produced exact output bytes.
- 50 owned-window MFC checks passed: selection, cancellation, request-context import, export format controls, existing export scopes/startup scheduling, menu placement and actual main-window EF2 export dispatch. Rendered dialogs were inspected.
- Rejected input and persistence failure leave the existing catalog intact; imported request context survives reopening. The personal catalog remains byte-identical with 25 records.
- The first full run found a Grabber concurrency regression, which was corrected before the passing rerun. UI failures exposed a pre-show visibility assertion and an index-based assumption after queues were sorted; both test corrections and original failure records are retained.

## Remaining scope

No live IDM GUI export/import round trip was performed because personal desktop control remains paused after Escape. Legacy `.ief` uses the same accepted record grammar with a Windows code-page fallback; a real legacy reference fixture is still needed. Unknown fields fail explicitly. Media plans, proxy/session objects, arbitrary headers, non-form or multiline bodies, settings, resume chunks and completed file contents require other handling; they are not silently flattened into EF2. Full backup/restore and catalog interoperability remain open.

The network driver and browser payloads are unchanged. Broader capture/video/proxy/driver, GUI/accessibility, lifecycle and speed acceptance remain in the [gap register](idm-research-current-status-2026-09-29.md).

Installer: [UDM 0.79.0 / browser 0.53.1](D:/UDM/installer-out/UDM-0.79.0-Browser-0.53.1-Setup-x64.exe)

SHA-256: `6cd36bf85e24f9fdb9ee3873107b2d215c28d5ade1c3eea5f279820b00792051`

Recoverable source backup: `D:\UDM\backups\source-0.79-import-export-20260930`. No Git commit or push was made.
