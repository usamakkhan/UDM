# UDM 0.82.0 candidate — Dial-up/VPN credentials

Source and installer delivered; installed personal runtime remains unchanged. Browser files remain 0.53.1. Full IDM parity is not established.

## Change

Options > Dial Up / VPN now has User name, masked Password, Save password, and per-connection Apply fields at the corresponding reference dialog-unit positions. Drafts stay separate for connections with the same name in different phonebooks. Cancel discards unapplied drafts; explicit Apply remains saved. Global Apply validates all drafts and reports partial success if a later Windows update fails.

Windows stores saved credentials through RasGetCredentialsW/RasSetCredentialsW. Unchanged masked placeholders are never submitted as new passwords. Session-only passwords remain DPAPI-protected in process memory and are supplied to automatic dialing only when needed. Clearing a stored password removes its Windows password field. Newer Windows credentials supersede stale session values. Passwords are not written to UDM settings/history.

## Evidence

- 2,482 full native checks passed.
- 30 scoped checks passed, including actual Windows credential save, preserve, clear, session handling, external replacement, and private-entry cleanup. The fixture used synthetic credentials in a private phonebook; it did not dial.
- 37 MFC GUI checks passed: masking, reference field positions, Apply/Cancel, connection switching, duplicate names, empty lists, failure/retry and no plaintext in settings/history.
- Final screenshot inspected for readable controls and overlap. Existing dialog styling differences remain.
- Personal catalog and installed executable hashes remain unchanged.

The first credential run mistakenly executed unrelated HLS prerequisites and recorded one stop/save timing failure. All credential checks in that run passed. Scoped dispatch was moved before those prerequisites, and HLS failure diagnostics were added without relaxing its assertion or changing HLS production behavior. That initial failure is retained; no HLS fix is claimed. The first GUI fixture stopped before assertions because its owned window was not yet visible; the harness now makes its window visible before interactions. Final acceptance uses rebuilt executables.

[Reproduction instructions](../tests/dial-credentials/README.md). Evidence includes both unsuccessful initial fixtures and final runs.

## Remaining qualification

Real modem/VPN connection, timeout/cancel, EAP, all-user/policy restrictions, Windows credential-clear rollback failures, and manual Connect with session-only credentials remain unqualified. The existing manual Connect command uses the Windows dialog and may prompt again. Multiple Windows credential updates are not an atomic transaction. This candidate closes the direct credential-editor gap only; full R11 and full IDM parity remain open.

No personal app/browser activation, certificate trust, driver, hosts-file or Git commit/push change was made.

Installer: [UDM 0.82.0 / browser 0.53.1](D:/UDM/installer-out/UDM-0.82.0-Browser-0.53.1-Setup-x64.exe)

SHA-256: 187ba05efd1f5ab441b6b8a810752873c1ef62d3ab3e9ba7cb6b8fc1571acb16

Source backup: D:\UDM\backups\source-082-dial-20261001
