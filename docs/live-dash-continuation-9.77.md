# Resume a retained DASH Period after it leaves the manifest

UDM **9.77 / both source extensions 0.73.41** resumes a completed retained live DASH Period when the refreshed manifest has already removed it. Five native products build with 9.77 resources; all 1,532 assertions in 22 affected suites pass. Real fresh-process recovery, rejection and cancellation controls qualify this change. Installed files remain unchanged, and the broader IDM performance/parity gates remain open.

## Reproduced failure

The actual 9.76 worker captures a complete one-second Period, pauses without publishing and closes normally. The origin then retires that Period, removes it from the MPD and refuses its resources with HTTP 410. A fresh 9.76 process reopens the catalog and fails Resume with “Selected DASH track is no longer present in the refreshed Period.” The verified local resource hashes remain unchanged.

The old recovery path planned a transition only against its saved manifest, which did not yet announce the continuation. It then refreshed the MPD and tried to rematch the disappeared old track before looking for the new Period. Having all the old bytes was therefore insufficient to advance.

## Change and boundaries

After reconciling retained resources, the worker checks that the current Period reaches its known saved duration. For a completed dynamic Period it fetches a fresh MPD and uses the existing presentation/format/adjacency validator to find the continuation before rematching old IDs. It then uses the existing advance, seal, prepared-next-Period and publication journals. It does not guess an end, skip a gap, change quality or fetch the retired body again.

If the old Period remains visible and no continuation is announced yet, its rematched fresh state is checkpointed and handed to the capture loop, preserving ordinary waiting/refresh behavior. A saved static end keeps its existing local finalization path. Parent cancellation still pauses normally; a manual Save that cancels only the capture token during the new refresh can proceed to seal the verified prefix. No Manager lock is held around the new network request.

Incomplete retained coverage and a missing/ambiguous/incompatible or nonadjacent continuation retain their failure behavior. Known Period duration is still required for this early completed-Period recovery route. Unknown-duration retirement, other live addressing modes, changed codecs/languages, outage matrices and abrupt-process/power-loss recovery are not established by these controls.

## Real worker qualification

Every scenario uses actual adaptive admission, public Resume/dispatch, durable catalogs and resource receipts, normal pause/stop/join and fresh native processes. The continuation test retries the **same failed 9.76 catalog** with the linked 9.77 core. The origin remains the same, and the old resources are unavailable after pause. Original and moving-pattern fixtures independently reproduce the old failure and pass on the new core.

| Scenario | Result | Exact assembled/final bytes | Decoded content |
| --- | --- | ---: | --- |
| Retired complete Period, simple source | Resume completes once without old-body replay | 3,848 / 3,848 | All twenty source frames in order |
| Retired complete Period, moving source | Resume completes once without old-body replay | 211,911 / 211,911 | All twenty **distinct** source-frame hashes match |
| Current Period still visible, next announced later | Resume waits, then completes once | 3,848 / 3,848 | All twenty source frames in order |
| Manual Save while fresh MPD headers are withheld | Completes from retained bytes before headers are released | Final file 2,311 | All ten retained frames match |
| Only one second retained from a known two-second Period | Resume rejected; no publication | None | Retained resource hashes unchanged |
| Next Period starts after a timeline gap | Resume rejected; no publication | None | Retained resource hashes unchanged |

Both rejection cases run with simple and moving media. The still-visible and stalled-refresh Save controls use 9.77 for both capture and restart. Their ten native assertions each pass. The retired-Period positive flow has three capture assertions on 9.76 and seven successful Resume assertions on 9.77; expected failed assertions from the frozen/rejection controls are retained, not counted as passing tests. The seven Save assertions exercise the new cancellation boundary while the origin deliberately supplies no response headers.

Motion frames come from independently generated animated test patterns with a hue change in the second Period. All twenty original decoded source-frame hashes are distinct. This strengthens the earlier constant-color oracle against frame loss, duplication or reordering inside a Period. It is still a tiny video-only controlled fixture, not a bitrate benchmark or browser/audio/caption acceptance claim.

The motion generator initially omitted its `struct` import while splitting MP4 boxes. That setup attempt started no origin or native actor. The import was corrected, only the owned tiny assets were regenerated, and the correction is recorded separately. No product code changed during qualification.

## Evidence and remaining work

[Old failure and preserved bytes](validation/live-dash-continuation-9.77/restart/baseline-reproduction.json), [simple restart/rejection qualification](validation/live-dash-continuation-9.77/restart/qualification.json), [moving-frame qualification](validation/live-dash-continuation-9.77/motion/qualification.json), [still-visible control](validation/live-dash-continuation-9.77/restart/visible-qualification.json), [held-refresh Save](validation/live-dash-continuation-9.77/finish/qualification.json), [source delta](validation/live-dash-continuation-9.77/build/continuation.patch), [final audit](validation/live-dash-continuation-9.77/final-audit.json).

Maintained fixtures: [live_dash_restart_workflow.cpp](../benchmarks/speed/live_dash_restart_workflow.cpp), [live_dash_finish_refresh.cpp](../benchmarks/speed/live_dash_finish_refresh.cpp). The browser source change is a version increment; its JavaScript behavior is unchanged. No installer, installation, commit, push or new IDM comparison is part of this delivery. Finishing-speed, installed/browser/UI behavior and full inside/outside parity remain unproven.

## Cleanup and retained state

The final audit verifies 158 sanitized exports, including the cleanup receipt. Cleanup individually checked and removed 404 generated compiler/test/payload files, totaling 314,597,510 logical bytes (about 300 MiB). This includes two superseded core objects. Three temporary media-helper junctions were removed separately; their installed targets were preserved. Current five native products and all 42 referenced core objects remain. All 694 captured inputs and eleven protected installed/catalog/helper files match their hashes. Physical space reclaimed was not measured.
