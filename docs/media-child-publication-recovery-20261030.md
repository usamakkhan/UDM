# Media child publication recovery — 1.2.13

Job::snapshot serializes private Video and Audio jobs beneath their parent, and Job's constructor reconstructs those child objects on startup. Individual track transfer still creates a publication journal keyed by the child's identifier. recoverReplacements previously searched only Manager::jobs, so an interrupted child publication could reject startup with a missing-record error even though its record was saved in the parent.

Publication recovery now lazily indexes the saved job tree, including private video/audio tracks. It retains the existing identifier, journal filename, destination, staging and completed-file hash checks. Restored child state is saved through the parent's normal recursive snapshot before the journal is removed. Staged output follows the existing paused-recovery path; an already published, hash-matching track can recover as complete without marking the parent media download complete prematurely.

The index visits each object once and refuses duplicate identifiers belonging to different objects, rather than guessing a recovery target. Replacement-history recovery keeps its original top-level lookup. The change does not reconstruct missing catalog records or infer ownership from filenames.

Native version is 1.2.13 with browser 0.63.82 unchanged. All five native production targets built successfully as 1.2.13. Source and binary hashes are recorded in [build evidence](validation/media-child-publication-recovery-build-20261030.json). Tests and installer creation remain deferred. Installed UDM remains 1.2.8; the last historical installer is 1.2.9. This implementation does not establish full restart, rollback or IDM parity acceptance.
