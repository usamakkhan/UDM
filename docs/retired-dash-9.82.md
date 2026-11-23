# UDM 9.82: resume a retired live chapter whose duration was unknown

UDM 9.82 can continue a live DASH recording when a fully retained Period disappears from the server before its duration was declared. It joins the next compatible Period only when a fresh manifest's explicit start agrees with complete verified coverage in every selected track. Both extension source manifests advance to **0.73.46**; browser JavaScript is unchanged. Five native programs build as 9.82. This candidate is **uninstalled**, and complete IDM behavior and speed parity remain unproven.

## Reproduced failure and implementation

The frozen 9.81 worker captures a complete one-second video chapter with no declared Period duration. After pausing, the owned origin retires that chapter and returns its adjacent successor. Fresh-process Resume fails with `Selected DASH track is no longer present in the refreshed Period.` Both retained media resources remain unchanged. [Frozen reproduction](validation/retired-dash-9.82/workflow/baseline-reproduction.json).

[DashManifest.cpp](../native/DashManifest.cpp) now checks whether the reconciled recording has a complete contiguous interval: initialization must be verified, each media resource committed, segment numbers and media ticks contiguous, and every selected track must reach the same end. Existing requested or declared boundaries cannot be shortened. Supported numeric bounds and cancellation remain enforced. This establishes observed coverage; it does not by itself establish that a live Period ended.

The transition planner also requires a fresh explicit successor start at that boundary. It retains the existing presentation identity, chronology, adjacency, stable selection and compatible-format checks. A gap, overlap, missing coverage, incompatible track or different presentation cannot be bridged by an inferred duration. The existing timing tolerance applies. No bitrate or elapsed-time estimate supplies this boundary.

[LiveDash.hpp](../native/LiveDash.hpp) uses that proof before rematching expired track IDs. It persists the protected timing revision first, then reconciles and checkpoints the recording end, updates the durable session, and enters the existing advance/seal path. A restart between revision and recording-index updates can restore the revision and finish the checkpoint. A still-visible chapter retains its refresh/rematch path; a shorter observed prefix cannot cause an early transition to a later successor. The existing known-duration continuation remains supported.

The [isolated source delta](validation/retired-dash-9.82/retired-period.patch) includes the new maintained boundary suite and the native workflow fixture's interruption stage. This is a recording-recovery improvement, not a measured change to ordinary-file finishing latency.

## Qualification

All **1,595 assertions in 24 affected backend suites** pass, including **36 new boundary checks**. The new suite covers contiguous and missing numbers, tick gaps/overlap, unfinished resources, initialization, supported clock precision, different audio/video timescales, differing track ends, preserved stop boundaries, cancellation, retired/current Periods, missing proof, changed presentation and incompatible codecs. [Suite results](validation/retired-dash-9.82/build/suite-results.json).

The real HTTP workflow uses two generated moving-pattern chapters with twenty distinct source frames. The same catalog and resources that failed with frozen 9.81 are resumed with 9.82. Every positive output is decoded through EOF and compared with the independently decoded source frame sequence.

| Case | Verified result |
| --- | --- |
| Retired chapter, duration originally unknown | Reuses the failed 9.81 recording; appends its successor and publishes once |
| Previously known duration | Existing continuation still completes |
| Old chapter still visible | Ordinary manifest-derived transition completes |
| Revision committed, index update interrupted | Fresh-process Resume finishes the checkpoint and completes |
| Gap before the next chapter | Rejected, no publication |
| Overlap with the next chapter | Rejected, no publication |
| Incomplete known-duration chapter | Rejected, no publication |
| Different presentation | Rejected, no publication |
| Interrupted revision followed by a gap | Rejected, no publication |

All **four successful cases** match all **20 source frames in order**. The main resumed output is **211,911 bytes** both at the assembled pre-publication checkpoint and as the final file; its saved SHA-256 matches the file. This is an assembled-size check after recording, **not a pre-Start size claim for an unbounded live stream**.

All five rejection cases preserve their old media hashes and fetch no successor resources. Each original initialization/segment is fetched only once across capture, failed Resume, interruption and successful/denied recovery. Nonzero negative-actor exits and their failed completion assertions are retained as expected rejection evidence, not counted as passing assertions. [Workflow acceptance](validation/retired-dash-9.82/workflow/qualification.json), [all request/actor records](validation/retired-dash-9.82/workflow/candidate-terminal.json).

The native workflow proves video-only continuation. The multi-track unit checks prove clock/coverage rejection logic; they do not establish real audio/caption mux or synchronization for this new case. Three-Period unknown-duration chains, partially retained still-visible chapters, new-boundary storage faults and a fresh held-refresh Save comparison remain useful follow-ups.

## Test fixture and controller corrections

The initial synthetic planner fixture used a live SegmentList without its required explicit SegmentTimeline. Its first 25 coverage assertions passed, then fixture setup failed. The preflight result and initial build result remain. After that build terminated, only the test declaration was corrected and recaptured before recompilation; the corrected suite passes all 36 checks. All other 23 suites then run successfully. No production source, object, resource or product hash changes for this correction. [Explicit test-input amendment](validation/retired-dash-9.82/fixture-correction/amendment.json), [initial failed result](validation/retired-dash-9.82/fixture-correction/initial-build-results.json), [test-only patch](validation/retired-dash-9.82/fixture-correction/fixture.patch).

The original toolchain configuration exported its `area` variable, placing baseline artifacts in the baseline root. The candidate controller was adjusted to use that actual root; no capture or baseline was repeated. The baseline actor and origin had terminated before the first production edit. [Root correction](validation/retired-dash-9.82/workflow/driver-root-correction.json).

The cleanup controller initially retained the prior receipt filename. Its actual 9.82 receipt was copied to the correct filename, and the prior 9.81 receipt restored from unchanged tracked evidence. No deletion was repeated. Both executed/corrected controller versions and the [receipt correction](validation/retired-dash-9.82/cleanup-record-correction.json) are retained.

## Build ownership and cleanup

Adaptive, DashManifest and Bridge are rebuilt; **39 core objects** are retained. The final **700-input inventory**, **42 referenced core objects**, five native product hashes and resource are verified. The initial capture and test-only amendment distinguish product compilation from corrected-fixture compilation. All five PE versions are 9.82. [Build](validation/retired-dash-9.82/build/build-result.json), [final audit](validation/retired-dash-9.82/final-audit.json), [export hashes](validation/retired-dash-9.82/export-manifest.json).

After sanitized export, **1,118 individually path/type/hash-checked files**, totaling **374,207,402 logical bytes**, are removed. These include generated videos, downloads, private fixture catalogs, source snapshots, test actors and disposable compiler files. Seven superseded helper/object artifacts are retired; the frozen 9.81 App remains. The task's shared-tools junction is removed without traversing its target. Current five products, resource and all referenced core objects remain. Physical space reclaimed was not measured. [Cleanup receipt](validation/retired-dash-9.82/cleanup-receipt.json), [retired artifacts](validation/retired-dash-9.82/retired-artifacts.json).

All owned native processes and origins are terminal. Installed programs/manifests, both personal catalogs and shared FFmpeg/FFprobe retain all eleven protected reference hashes. Reproduction drivers replace private roots with `[repo]`, `[local-diagnostics]` and `[user-profile]`.

## Still open

A fresh desktop-control retry still fails with `failed to write kernel assets` / missing path. IDM's existing session-2 background process remains; the other session is left alone. [Current blocker](validation/retired-dash-9.82/desktop-blocker.json). No process is forced closed and no administrator prompt is requested.

Installed browser capture, visible UI behavior, a fresh IDM comparison and the earlier finishing-speed gate remain open. Restore supported desktop control before paired UI/lifecycle measurements. No installer, installation, commit, push or release occurs in this increment.
