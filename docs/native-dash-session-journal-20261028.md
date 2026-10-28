# Persistent live DASH recording sessions

Build status: all five production executables and the installer compiled successfully. The candidate is unsigned, staged, uninstalled and unpublished. Runtime session recovery remains unverified under the testing deferral.

`DashRecordingSession` stores the original selected plan and an ordered list of recorded Period descriptors in DPAPI-protected `session.json`. Each entry contains its generated folder name, full selected track identities, capture start/end, known Period duration and sealed state. The session phase is `recording` or `saving`; saving requires every listed Period to be sealed.

The live worker now opens this journal on resume, derives the current folder and selected track IDs from it, checkpoints accepted recording descriptors after capture, and commits the sealed state before entering saving. Final assembly receives the journal's ordered folder list instead of a hard-coded list. Existing recordings without a session journal are adopted from their reconciled index; old indexes without a capture interval are adopted once capture or sealing supplies one. A session whose recorded index or interval has disappeared fails without rebinding retained files.

The journal enforces a stable initial selection, one presentation identity, consistent track formats, unique Period IDs and generated folder ordering (`.` followed by `period-0001`, etc.). Only the final Period may be unsealed. Advancing requires the preceding Period to be sealed at its known end, an adjacent next Period start and a zero capture start in the new Period. Existing track identities and capture starts cannot change. Previously known capture ends and Period durations cannot change or disappear, and a sealed entry cannot be reopened.

Writes validate a copy, flush and atomically replace the protected file, then update the in-memory state. Unchanged descriptors cause no write, so ordinary segment progress does not rewrite the session. Media completion remains owned by per-resource receipts and recording indexes; the session does not claim to verify their bytes. Parsing is bounded to 128 Periods, 8 MiB decoded content and a 16 MiB envelope. Ancestor directories, the journal, temporary path and current Period directory are checked for reparse redirection.

This is the durable ordering and saving layer. The current worker still captures a single Period. Preparing the next Period bootstrap, committing capture advancement, aggregating progress across Periods and automatically invoking that transition remain unfinished. The browser's live multi-Period restriction stays in place. Browser 0.63.62 is unchanged, and runtime tests remain deferred at the user's request.

Build results are recorded in `validation/native-dash-session-journal-build-20261028.json`. The candidate remains staged, uninstalled and unpublished; successful compilation does not prove live interruption or multi-Period behavior.
