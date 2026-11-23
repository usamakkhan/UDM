# Bounded recovery when a live DASH manifest expires — 10.3

The retained 10.2 continuous-recording failure occurred after 1.024 seconds had been captured: the planner reported `DASH manifest expired while synchronizing its clock`. No final file was published. A later unchanged run passed, leaving the exact cause of that original scheduling delay unproven.

The existing planner refreshed an expired snapshot once, synchronized its clock, then threw if the refreshed response had expired in the meantime. A short update interval could be consumed by response receipt, metadata accounting, parsing or a separate HTTP clock request. There was no internal route to fetch a fresh response and finish this planning operation, even when the next response was immediately usable.

## Controlled evidence and implementation

The diagnostic actor links the actual retained 10.2 assembler/planner object and uses an owned HTTP origin advertising a 200 ms manifest interval. Explicitly expiring the initial snapshot schedules a refresh without altering its original body-receipt clock anchor. A 350 ms accounting hold, a 350 ms cached-response delay and an independently delayed HTTP clock source reproduce the old failure. These reproduce the failure mechanism; they do not identify which delay caused the historical run.

The updated planner permits at most three freshness attempts within one call. Each retains the existing request deadlines, accounting and parent cancellation. A response already expired at receipt/processing is kept only for its conditional validator and monotonic publication checks. Its selected tracks and media windows are not committed. The next attempt requests another response. After clock synchronization, the existing expiry check can likewise request another bounded refresh. If all three attempts expire, the call fails explicitly and the caller retains its prior state.

Presentation identity, publication ordering, origin, rematching, clock-source qualification and strict Period-transition proof remain required. A transition found during recovery is checked for freshness again before return. A 304 response can renew response validity, but cannot invent a new direct-time body anchor. No validity interval, media-availability uncertainty, clock-source age check or network timeout is relaxed. Missing resources, identity changes and HTTP errors do not become generic successful retries. Normal calls with a usable manifest add no request.

The original guard still precedes media-window construction. This change does not establish an atomic guarantee that no scheduling time can elapse after that guard, nor extend freshness enforcement to every later capture step. That broader lifecycle remains a separate audit.

## Qualification

All five native programs build as 10.3; both source browser manifests advance to 0.73.67 and browser scripts are unchanged. All 740 pinned build inputs, 42 core objects and 12 produced executables verify. Forty retained core objects are reused; Bridge and DashManifest are rebuilt. A separately linked diagnostic actor verifies the retained 10.2 planner using its complete previous core set. Cached compiler setup is used; the complete build/packaging entrypoint is not exercised.

The 20 AAC structural and 42 strict retained-boundary assertions pass. Eight targeted freshness controls reproduce the expected old failure and qualify the new bounded behavior:

| Control | Retained 10.2 planner | Current planner |
| --- | --- | --- |
| One accounting delay | Fails at expired response | Fetches fresh response and plans media |
| Every accounting response is late | Fails on first late response | Rejects after three attempts |
| Presentation changes on recovery | Stops at earlier expiry | Rejects changed identity |
| Recovery returns HTTP 503 | Stops at earlier expiry | Preserves HTTP failure |
| One slow 304 response | Fails at expired response | Recovers, preserving original body anchor |
| Parent cancels recovery | Stops at earlier expiry | Propagates cancellation |
| One slow HTTP clock response | Fails after clock request | Refreshes and resynchronizes successfully |
| Every HTTP clock response is late | Fails after first clock request | Rejects after three attempts |

Each successful control returns a fresh snapshot with planned media. Failed calls leave the caller's snapshot unchanged. The accounting/cached-response fixture and HTTP-clock fixture record request counts, deliberate holds, monotonic callback durations and terminal outcomes. A normal successful recovery consumes two refreshes after the seeded initial fetch; persistent failures stop at three. The clock-source controls distinguish MPD GETs from time-source GETs.

**Full continuous-recording acceptance remains failed.** The ordinary current actor reached 2.048 retained seconds, then all three refreshes expired; it published no file. A separate diagnostic actor using the same planner plus buffered phase measurements failed at 1.024 seconds and likewise published nothing. Both actors and origins terminated. These failures are retained and are not replaced by a passing targeted-control label.

The diagnostic trace measured header-constructor stages of 589.910, 234.937 and 208.212 ms on the three late responses. Body reads took 0.237–0.253 ms, metadata accounting 0.022–0.077 ms and parsing 0.238–0.564 ms. This eliminates parsing and accounting as the dominant cause in **that measured run**. The header stage includes local HTTP/session initialization, admission and response-header work; it does not prove that the remote server or wire caused the delay.

The request implementation creates a new `HttpSession` whenever no caller-owned pool is supplied. DASH metadata calls currently supply none. A separate private reuse experiment passes all seven continuous-workflow cases, including exact audio packets/PCM, video timing, strict gap/short-audio/identity rejection and Save during held refresh. Its later audio/video header stages include 19.483, 12.797, 5.880 and 2.902 ms. All six observed header stages range from 2.902 to 60.467 ms. All sampled header durations are retained in the evidence, including the initial cold request. These are separate runs under changing host load, not a paired speed benchmark or proof of one specific WinHTTP internal delay.

The experimental cache is diagnostic only and is not part of the native candidate. A product implementation must use a bounded transfer-owned lifetime and separate origins/settings/credentials, rather than adopting an unrestricted thread cache. Verify cookie isolation, changed preferences and normal/exceptional teardown before enabling it. The prototype's full workflow pass does **not** turn the production candidate's failed full run into a pass. [Experimental result and complete timing series](validation/dash-freshness-10.3/reuse-experiment.json).

See [scoped verification](validation/dash-freshness-10.3/verification.json), [targeted comparison](validation/dash-freshness-10.3/comparison.json) and [phase trace](validation/dash-freshness-10.3/phase-timings.json). Nine protected installed file hashes remain unchanged. No installed browser or IDM timing acceptance is claimed.

## Follow-up order

1. Implement a bounded transfer-owned metadata transport using the now-qualified prototype as evidence; verify isolation, cancellation, shutdown and changed settings before using it in products. Keep the precise cause of the older historical expiry separate from this measured mechanism. The prototype itself must remain outside product code.
2. Repeat the full continuous-recording suite with the resulting product code, retaining every failed attempt. The current three-attempt change alone is insufficient.
3. Audit freshness across expensive window construction and later capture/admission boundaries; preserve verified retained media and cancellation throughout.
4. Implement exact non-aligned video packet/decode ownership and preroll, followed by interior AAC continuity.
5. Investigate repeated capture-resource GETs and qualify installed browser, desktop and cross-player flows in the persistent lab when usable.

No installer, installation, release or temporary-file deletion is part of this candidate.
