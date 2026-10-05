# UDM 0.76.0 / browser 0.50.0

Installed 30 September 2026. The native app and paired extension now include the previously staged browser ownership, recovery dialogs, file recognition, multipart form capture and nested HLS catalog changes. Personal Edge Profile 1 visibly loaded 0.50.0 and its desktop connection check succeeded. Full IDM parity is not established.

## What shipped

- Browser/native ownership and atomic admission improvements, including lost-reply recovery and durable download/refresh presentation.
- MIME and Content-Disposition recognition for supported files with generic or missing filename extensions, while respecting browser/desktop exclusions and negative cases.
- Supported UTF-8 multipart File/Blob capture within the existing 1 MiB POST bound.
- Nested HLS video catalogs, scoped audio tracks, duplicate suppression, bounded traversal and quality/container selection.

## Evidence and limits

The retained native suite passed 2,309 checks. This continuation passed 1,163 browser/native-host checks across 44 scripts, 31 actual MFC checks, 13 Edge form scenarios, two lost-reply cases, 13 Firefox form scenarios and six public HTTPS ownership transfers. The complete controlled HTTPS recognition matrix passed all 13 cases / 23 assertions with normal TLS verification and exact output hashes, including original POST-body preservation without GET/HEAD probes.

The first HLS run timed out after its first download, waiting for an audio-only job. Its cause remains unknown. Three later serial runs covered four complete iterations / 20 downloads successfully; the last ten transfers also had successful traced panel and native acknowledgements. This is a known unresolved intermittent failure, not a claimed fix. All retained failed evidence remains available.

Both localhost-only test certificates were removed; independent CurrentUser and LocalMachine Root reads confirmed absence. Test-native-host registrations were removed. No driver, proxy, hosts-file or test-mode change was made.

## Installation and preservation

All 83 deployed files matched after restart. Native-host diagnostics identified the running 0.76.0 app and the original `D:\UDM\user-data` catalog. Its 25 records, queues and preferences remained byte-identical. The network runtime was unchanged.

The recoverable file backup is `D:\UDM\backups\release-0.76.0-20260930`. The paired installer is [UDM 0.76.0 / browser 0.50.0](D:/UDM/installer-out/UDM-0.76.0-Browser-0.50.0-Setup-x64.exe), SHA-256 `0a52f3daed5c01de5ab822a4d928ae841d3afa76c438fed04143cc2a6a4adcd3`. Source and test changes are installed under `D:\UDM`; no commit or push was made.

Edge's activation and connection were verified in its actual personal profile. No test downloads were added there. Personal Chrome/Firefox activation and broader real-site playback/download acceptance remain unverified. The earlier C-to-D project junctions are unchanged.

## Remaining work

Resolve the intermittent HLS timeout and finish legacy uncertain/changed-target recovery, broader player/ad association, missing media/proxy modes, automatic driver handoff, COM cold activation/integration, backup interoperability, GUI/accessibility, physical recovery and matched-route speed testing. The separate native 0.75 COM work is not integrated.

[Detailed acceptance](evidence-0.76.0/ACCEPTANCE.md) · [Deployment verification](evidence-0.76.0/deployment-verification.json) · [Current gap register](idm-research-current-status-2026-09-29.md)
