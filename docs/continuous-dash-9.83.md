# UDM 9.83: continue live audio and video when chapters rotate

UDM 9.83 fixes an uninterrupted DASH recording failure when the server removes the current Period from its manifest. It checks the verified adjacent transition before attempting to rematch expired track IDs. Five native programs build as **9.83**; both extension source manifests advance to **0.73.47**, with browser JavaScript unchanged. This candidate is **uninstalled**. Overall IDM/UI and finishing-speed parity remain open.

## Confirmed cause and change

9.82 recovers this condition on Resume, but its active recording loop still refreshed the manifest and immediately rematched old IDs. The frozen worker captures a complete audio/video chapter, then fails with `Selected DASH track is no longer present in the refreshed Period.` No Pause or restart is involved. Its four verified resources remain available. [Frozen reproduction](validation/continuous-dash-9.83/workflow/baseline-reproduction.json).

[DashManifest.hpp](../native/DashManifest.hpp) gives a live update an optional owned transition. When refreshed metadata establishes an adjacent compatible successor at the verified retained end, [DashManifest.cpp](../native/DashManifest.cpp) returns that transition before rematching the old IDs. This result has no current-Period resource windows; it cannot be treated as another download window.

[LiveDash.hpp](../native/LiveDash.hpp) checkpoints the protected timing revision, reconciles and checkpoints the recording boundary, and updates the durable session before using its existing seal/advance path. The caller retains ownership of joining and publication. Incomplete or discontinuous coverage, a different presentation, incompatible selection, and a gap/overlap remain rejection conditions. The default update path is retained when no complete boundary proof is available. Six additional native assertions reject invalid refresh-boundary arguments before network/clock work.

This extends existing recording behavior. It does not remove a receipt, checksum, file-ownership, flush, cancellation or publication barrier. [Isolated production and maintained-fixture delta](validation/continuous-dash-9.83/continuous-refresh.patch).

## Actual audio/video qualification

All **1,601 assertions in 24 affected backend suites** pass, including the now-42-check boundary suite. [Build](validation/continuous-dash-9.83/build/build-result.json), [suite results](validation/continuous-dash-9.83/build/suite-results.json).

The maintained [native actor](../benchmarks/speed/live_dash_continuous_workflow.cpp) and [owned HTTP runner](../tests/live-dash-continuous.native-live.py) generate a three-Period program. Each chapter is 1.024 seconds. The source video has **24 distinct moving frames**; source audio has **144 AAC packets** and **147,456 decoded mono samples at 48 kHz**. Fragments are rebased without changing their compressed media payloads. The origin rotates after serving a chapter's selected media and ends as a static manifest.

| Current workflow | Accepted outcome |
| --- | --- |
| Resume the failed frozen-9.82 audio/video recording | Retains its old chapter and completes all three Periods |
| Fresh uninterrupted audio/video | All three Periods complete without Pause/Resume |
| Fresh uninterrupted video only | All three Periods complete |
| Fresh uninterrupted audio only, M4A | All three Periods complete |
| Save while the next manifest's headers are held | Publishes only the retained first chapter before headers are released |
| Gap before the successor | Failed, no target/publication or successor-body fetch |
| Audio shorter than video | Failed, no target/publication or successor-body fetch |
| Changed presentation identity | Failed, no target/publication or successor-body fetch |

All full video outputs match every source frame **and its timestamp/duration**. All full audio outputs match the complete AAC packet sequence, every packet PTS/DTS/duration, and the SHA-256 of the entire decoded PCM stream. Save matches the first eight video frames, 48 AAC packets and 49,152 decoded samples. Each successful file's assembled size, final size and saved SHA-256 agree, with one publication. Live size before recording starts is not claimed to be known.

Every expected media resource is fetched once. Each negative retains exactly the four served initialization/segment hashes and has no final target. Expected negative actor exits and failed completion assertions remain in evidence; they are not counted as passing assertions. [Eight workflow gates](validation/continuous-dash-9.83/workflow/qualification.json), [all requests and actors](validation/continuous-dash-9.83/workflow/server-state.json), [independent audio-timing and retained-file audit](validation/continuous-dash-9.83/actor/independent-audit.json).

Save deliberately cancels an outstanding HTTP request. Releasing the held response afterward produces a fixture-server `ConnectionResetError 10054`; the original log is retained. Native Save had already completed successfully, before header release, and matched its source prefix. [Expected disconnect record](validation/continuous-dash-9.83/expected-save-disconnect.json).

## Fixture setup corrections

The new standalone actor initially omitted its explicit StreamHash header. Compilation failed without producing an actor; adding that include corrected the fixture, with no product-source change. That error log and initial source hash remain.

The first generated AAC asset contained 145 packets, including a one-packet fourth fragment after encoder draining. Generation stopped before launching any native job. Changing the fixture's frame cap from 144 to 143 yields the intended 144 delivered packets; changing 24 to 23 yields the 24-packet short-audio control. A new owned fixture root is used; the valid compiled actor is reused. Actual packet counts, decoded sample counts and the three-fragment layout are checked before the baseline runs. [Correction record](validation/continuous-dash-9.83/baseline/asset-correction.json), [initial generation log](validation/continuous-dash-9.83/baseline/baseline-initial-generation.log), [corrected run inputs](validation/continuous-dash-9.83/baseline/corrected-run-inputs.json).

No completed recording is rerun to obtain a favorable result. The failed 9.82 recording is subsequently resumed by the candidate. Production edits begin after its baseline controller terminates. All 702 candidate inputs are captured before compilation and still match the final tree.

## Ownership, evidence and cleanup

Adaptive, DashManifest and Bridge are rebuilt; **39 core objects** are retained. All **702 inputs**, **42 referenced objects**, five product hashes and the resource verify. All five Windows PE versions are 9.83. All owned actors and origins are terminal. [Final audit](validation/continuous-dash-9.83/final-audit.json), [product versions](validation/continuous-dash-9.83/product-versions.json), [export hashes](validation/continuous-dash-9.83/export-manifest.json).

After sanitized export, **1,187 individually path/type/hash-checked files**, totaling **361,521,791 logical bytes**, are removed: generated media, downloaded outputs, private test catalogs, source snapshots, fixture programs and disposable compiler files. Seven superseded helper/object artifacts are retired; the frozen 9.82 App remains. Both temporary junctions are removed without traversing the shared tools directory. Current products/resource/objects remain. Physical space reclaimed was not measured. [Cleanup receipt](validation/continuous-dash-9.83/cleanup-receipt.json), [retired artifacts](validation/continuous-dash-9.83/retired-artifacts.json).

Installed programs/manifests, both personal catalogs and shared FFmpeg/FFprobe retain all eleven protected reference hashes. No helper executable is copied. Reproduction drivers replace private roots with `[repo]`, `[local-diagnostics]` and `[user-profile]`.

## Remaining requirements

These cases use aligned chapter ends. The proof still requires all retained tracks to reach the same end; valid non-aligned audio/video ends, segment overhang and exact sample-level clipping need separate implementation and qualification. Unequal chapter lengths, captions, long recordings and storage-failure checkpoints also remain to be covered. The successful matrix is not proof for every DASH source or codec.

A fresh desktop retry still fails with `failed to write kernel assets` / missing path. Existing IDM processes in both Windows sessions are left alone. [Blocker](validation/continuous-dash-9.83/desktop-blocker.json). Installed browser/UI acceptance, a fresh IDM comparison and the previous whole-app speed losses remain open. No installer, installation, approval prompt, commit, push or release occurs in this increment.
