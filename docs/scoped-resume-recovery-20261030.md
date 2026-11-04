# Scoped Resume recovery — 1.2.14

Resume previously called the global recovery routine only when the selected top-level download had a publication journal. Replacement journals and private audio/video track journals were missed, while a triggered global pass could inspect unrelated downloads.

Resume now passes its selected job into recoverReplacements. The routine builds a scope containing that job and saved child tracks, filters publication/replacement journal filenames before parsing, and limits legacy failed-save reconciliation to the same scope. Its existing journal/hash/rollback rules are retained. Replacement journal filenames must match their recorded job ID. Scoped recovery rejects an active selected job/track or active replacement original before touching that recovery operation. Startup still uses the full-catalog recovery mode.

Recovery uses the manager lock, as before, and can hash files; this change is not a responsiveness claim. A recovered completed parent is left complete, while a recovered child stays inside its parent and the parent continues through ordinary Resume. Invalid or missing evidence still reports an error and preserves recovery files. Successful recovery is persisted before journals are removed.

The common pending-recovery message now offers Resume after the download stops, with restart as an alternative. Native version is 1.2.14; browser 0.63.82 is unchanged. All five native production targets compiled successfully as 1.2.14. Source and binary hashes are recorded in [build evidence](validation/scoped-resume-recovery-build-20261030.json). Tests and installer creation remain deferred. Installed UDM remains 1.2.8.
