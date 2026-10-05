# UDM 0.83.0 / browser 0.54.0 candidate

Installer built and delivered. Source remains staged at D:\UDM-Workspace\candidates\native-083-media-receipts\project. Personal runtime remains 0.78.0; D:\UDM browser sources remain 0.53.1. No source promotion, installation, browser reload, driver change or certificate operation occurred.

## Changes
Adaptive HLS/DASH handoffs have durable receipts. A lost reply or background restart can identify the existing native download without sending it again. A recovery page distinguishes accepted, released and uncertain requests. It stores no media URLs, cookies or headers in its journal.

Native media-session recovery preserves captured replacements after restart. Save requires another review if the candidate changes before the click; Cancel leaves undisplayed newer replacements available. Applying a session and consuming its prompt now share one catalog save, including rollback on failure.

## Verification
- 2,541 full native checks; 59 focused receipt checks; 81 real MFC dialog checks; 11 actual host/desktop checks.
- 47 browser regression scripts; 42 receipt module checks.
- Edge normal/lost replies: 58/68 checks, ten decoded downloads each.
- Edge actual worker restart/recovery controls: 14 checks.
- Mixed host/desktop versions: 68 checks each.
- Firefox normal/lost/reload/recovery UI: eight checks, plus three grouped checks for actual video discovery and a trusted format click.
- Catalog write failure and forced process termination after session application retain consistent job/receipt state.
- 561 source files and 143 package inputs hashed; 39 changed source files have no baseline conflicts.

Earlier fixture failures are retained. Synthetic playback and downloads do not prove arbitrary site compatibility, speed superiority, driver equivalence or complete GUI parity. Native-host cold launch now passes 11 additional checks with a private catalog and no duplicate job; browser interruption during desktop exit and in-flight desktop upgrade acceptance remain open.

## Artifact
[Installer](D:/UDM/installer-out/UDM-0.83.0-Browser-0.54.0-Setup-x64.exe)

SHA-256: 795206bedbc3d532666397bcae77d29fc6c09397d95181fbf270494857b7aa63

Size: 60,441,290 bytes. Installer compilation and input integrity passed; clean-machine installation remains untested.

[Acceptance evidence](evidence-candidate-0.83.0/refresh-final-acceptance.md). Full IDM parity remains unfinished.

Additional evidence: [native-host cold-start acceptance](evidence-candidate-0.83.0/cold-host/ACCEPTANCE.md). Activation remains awaiting the user’s choice.
