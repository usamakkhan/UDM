# Automatic native DASH Period advancement

Build status: all five production executables and the installer compiled successfully. The candidate is unsigned, staged, uninstalled and unpublished. Runtime advancement and playback remain unverified under the testing deferral.

The live DASH worker now uses the next-Period planner, persistent session journal and session assembler as one capture loop. An initially single-Period stream can continue through later compatible Periods that appear in refreshed manifests. The original selected quality, language/track metadata and presentation identity remain fixed; selection ambiguity, changed formats, gaps and overlaps remain errors rather than implicit quality changes.

## Capture and commit order

After a refreshed MPD exposes a compatible adjacent Period, capture continues until every selected track covers the current Period end. Merely learning an end no longer terminates the capture loop. The worker records the `advancing` phase, seals and checkpoints the current Period, creates the next generated folder's empty recording index and protected continuation bootstrap, then atomically appends the new Period to the session. Capture begins at zero in the new Period only after that commit. Failed preparation retains the completed old Period and any prepared files.

Continuation bootstraps support both dynamic and final static manifests and bind their selected track identities to the recording index. They preserve the original session presentation identity even when a final static MPD omits its availability start time. Prepared folders cannot be silently rebound to another transition. Known retained Period durations are preserved during preparation. On restart, normal revision freshness and publication checks still apply; persisted clock samples are never treated as fresh.

## Recovery and stopping

An interrupted advancing phase seals an already captured boundary and resolves the next Period from the latest manifest. If capture reached a boundary just before the advancing marker was written, recovery can recognize the cached compatible transition before trying to rematch the old Period online. A fully captured final static Period can enter saving from its saved manifest. A sealed legacy/ordinary recording without advancement intent enters saving rather than waiting for another live Period.

Stop prevents further advancement. If the newly prepared last Period has no committed media segments, it is removed from the session list and the completed prefix is saved; its files are retained. A partially captured tail still uses the common-track sealing rules and can fail if it has no usable common interval. It is not silently discarded. Pause preserves the active session and pending media for resume.

## Progress and output

Received bytes, segment counts and captured duration include completed Periods plus the active one. Earlier sealed progress is cached during the running job to avoid rehashing all preceding media at every transition; restart and final assembly still reconcile retained resources. Missing initialization now resets a track's recovery window to the capture start so initialization can be recovered even when media segments were already committed.

Stop or presentation end feeds the journal's ordered folder list into session assembly, then the existing expected-hash and publication paths. Compatible Period outputs are joined; source resources and individual output receipts remain retained. A final known endpoint is considered complete only when the selected tracks actually reach it.

## Scope and validation

The browser still admits only an initially single-Period live catalog. Initial multi-Period selection, live captions, SegmentList/SegmentBase and incomplete low-latency chunks remain unsupported. If the active Period disappears from the MPD before its required boundary has been captured, the current rematch path can still fail and retain the recording; this change does not invent unavailable segments. Joined-output receipt reuse and broader retry classification remain unfinished.

Runtime recording, interruption, Stop, synchronization and playback tests remain deferred at the user's request. Compilation is not evidence of those behaviors. Browser 0.63.62 is unchanged. Build/package results are recorded in `validation/native-dash-period-advancement-build-20261028.json`; the candidate is staged, uninstalled and unpublished.
