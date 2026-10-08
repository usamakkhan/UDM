# Validated local update — 8 October 2026

The current source was rebuilt and installed into `D:/UDM/release`. This includes 4 MiB POST handling, the 16 MiB protected prepared-request aggregate budget, durable completion gates for `/q` and `/h`, Windows FTP proxy policy handling, live subtitle offer filtering, strict resumed-response validators, and SABR handling for unselected compressed or incomplete side segments. Selected compressed streams remain unsupported.

Fresh validation passed with zero failures:

- 2,572 full native checks, including the latest SABR truncation regression.
- 244 browser/protocol checks against the newly built native host.
- 40 focused durable completion and launch checks.
- 10 isolated real-app CLI checks; no actual network disconnection was performed.
- 13 isolated Edge POST checks, including exact 4 MiB forms and browser-restart safe release. Both temporary native-host registrations were removed.

All native input hashes remained unchanged across the build and tests. The four installed executables were backed up to `D:/UDM/backups/before-update-20261008`, copied and hash-verified. The native bridge successfully connected to the installed app. All 26 personal download records and the data-location configuration were preserved. Raw logs and receipts are under `candidates/release-validation-20261008`; the portable receipt is [validation/validated-update-20261008.json](validation/validated-update-20261008.json).

Native version remains 0.84.0 and browser version remains 0.62.8, so use the receipt hashes to identify this build. Existing browser sessions have not been verified to have reloaded the modified extension source. No new installer was built, published, or qualified on a clean machine. The live Edge restart case verifies release of a capture whose original browser response no longer survives; successful crash adoption and Firefox live coverage remain open. Public-site acceptance, equal-source speed comparison, driver parity, and full IDM parity remain unestablished.
