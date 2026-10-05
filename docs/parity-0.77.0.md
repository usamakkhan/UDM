# UDM 0.77.0 / browser 0.51.0

Installed 30 September 2026. Personal Edge Profile 1 visibly loaded extension 0.51.0 and its desktop connection check succeeded. Native-host diagnostics connected to the installed 0.77.0 app and original `D:\UDM\user-data` catalog. Full IDM parity remains unestablished.

## What changed

A captured replacement link now remains available for review when its original download changes, completes or is removed before acceptance. The native dialog offers a reviewed replacement, a separate download, discard, or Later. It preserves original partial files and prevents stale choices. Browser recovery keeps ownership after cancellation without silently replaying or resuming the response. Failed catalog writes roll back the decision. Later can be reopened with the extension's Recover interrupted downloads button.

The installer build now carries the actual paired browser version and rejects mismatched Chromium/Firefox packages. The paired [0.77.0 / 0.51.0 installer](D:/UDM/installer-out/UDM-0.77.0-Browser-0.51.0-Setup-x64.exe) has SHA-256 `1e0c20a45532123bba005adcf746b9e79a7d4a36a19e72253bb9e80a8ffb8695`.

## Validation and preservation

- 2,381 native checks, including 72 focused review checks.
- 1,171 browser/native-host checks across 44 scripts.
- 67 actual MFC checks; rendered review dialogs inspected.
- Six isolated Edge restart/popup checks and ten real transfer assertions covering binary upload, redirect, lost commit reply, exact bytes, cancellation order and persistent connection.
- All 37 deployed files verified after restart and Edge activation.
- The personal catalog's 25 records, queues and preferences remain byte-identical. The network runtime is unchanged.

The rollback backup is `D:\UDM\backups\release-0.77.0-20260930`. Source, build output, archived candidate and evidence remain available. Existing C-to-D junctions remain in place. No driver, proxy, hosts-file, test-mode or certificate setting was changed. No commit or push was made.

## Evidence boundaries and remaining work

The Edge restart fixture seeds a native review and interrupted journal, then checks a real canceled response and the actual popup. Native model/MFC tests exercise target changes. It does not independently reproduce every live target-change timing in a personal profile. Chrome/Firefox personal-session activation remains unverified.

Fresh desktop observation found IDM dialogs for the two earlier failed direct-worker fixture URLs (`127.0.0.1:58022/review.zip` and `127.0.0.1:51317/review.zip`). IDM was intercepting those downloads outside the isolated extension setup. The corresponding test prompts and cancellation-exclusion prompt were dismissed without accepting an exclusion. This supports an interference explanation for those failures; the final review test used its own page and a dedicated `.udmreview` fixture. It is not proof of the cause of the separate HLS timeout.

Legacy uncertain protocol-0/1 handoffs, the earlier intermittent HLS timeout, broader media/player/ad association, missing proxy and transport modes, automatic driver handoff, COM cold activation, remaining GUI/accessibility and backup/restore compatibility, physical recovery and matched-route speed comparisons remain open.

[Deployment verification](evidence-0.77.0/deployment-verification.json) · [Detailed candidate acceptance](evidence-0.77.0/ACCEPTANCE.md) · [Current gaps](idm-research-current-status-2026-09-29.md)
