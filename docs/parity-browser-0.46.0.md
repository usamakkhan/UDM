# UDM browser 0.46.0 with native 0.69.0

YouTube format display now binds directly to the requesting browser document and avoids redundant current-tab lookups. Download actions retain strict current-tab checks. No new permissions are required.

The actual public Edge workflow passed: download, pause, refresh links on the original history record, resume, and decode the complete 360p video with audio. The result is 28.86 MB and 10 minutes 34.6 seconds long. Three traced format reads took 0.3–1.3 seconds. These observations do not establish general speed or full IDM parity.

**807 checks passed:** 794 browser unit checks, five public recovery assertions, seven controlled recovery assertions and a full public-video decode. [Evidence](evidence-browser-0.46.0/summary.json), [scope and limitations](evidence-browser-0.46.0/DEVELOPMENT.md).

Native UDM remains 0.69.0. The driver, history and settings are unchanged. Reload UDM Browser Integration in **edge://extensions**, then refresh video tabs. Personal-session activation and rendered Windows controls remain unverified because computer control cannot initialize. Chrome/Firefox files are synchronized, with live testing focused on Edge.

## Installation on this PC

Browser files and the combined installer are installed. All 23 history records, queues and preferences are unchanged. Native executable and network runtime hashes are unchanged. Edge requires an extension reload; activation of the personal session is not yet verified.

[Installer](../installer-out/UDM-0.69.0-Browser-0.46.0-Setup-x64.exe), SHA-256 `e682fad6ba45ff1520f406528d06673c6879c3be5ed71fc7d801db55ef31bca7`.
