# Engine Stop/Resume guards — staged candidate

Manager::resume now ignores an already-active or completed job before checking encrypted browser context, capture requirements or duplicate decisions. Manager::pause returns immediately for completed jobs before mutating queue/capture metadata or saving. Both checks execute under the manager lock, covering the engine entry point even when a caller's earlier selection check becomes stale.

Baseline: seven of twelve checks failed. Completed Resume produced errors for capture flags, duplicate state or invalid saved browser context. Completed Stop changed metadata and wrote the catalog. Resume of an active job could raise a capture error.

After correction: all twelve checks passed. Unfinished media still requires fresh capture; real active Stop and eligible Resume still work, and the final loopback output hash matches. The existing 41 actual native menu/transfer assertions and six app menu/recovery handoff checks also passed against the corrected core.

Unchanged GUI source hashes were verified against the previous acceptance before reusing their compiled objects. The app and native menu fixture were relinked with the new Core/Bridge objects. A normal full build still uses build-app.ps1. No full native regression-suite claim is made here.

Evidence: D:/UDM-Workspace/candidates/native-084-backup/control/terminal-actions-acceptance.json. Both failed baseline and successful results are retained. Personal catalog and installed app/host/monitor hashes are unchanged. Candidate remains staged, without versioning or installer integration. This is not proof of every completion/scanner/queue race or full IDM parity.


Follow-up correction: completed jobs may still have an active scanner wait. [Scanner-stop acceptance](scanner-stop-candidate-20261001.md) supersedes the blanket completed-job Stop guard while preserving idle completed records.
