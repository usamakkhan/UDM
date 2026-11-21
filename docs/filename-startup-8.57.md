# UDM 8.57: reuse the reserved-filename pattern

Native source is **8.57**, and both extension source manifests are **0.72.21**. Browser JavaScript is unchanged. Five products built; installed desktop remains **8.32**. Installation is deferred, with no setup creation, extension deployment, push or release.

## Measured work and implementation

`safeName` is used by loaded Jobs and download admission. It stripped paths, normalized unsafe characters and whitespace, then constructed the same reserved-device-name regex on every call. A separate control against qualified 8.56 measured 7,500 normalizations at **50.509 ms** before any production change. That calibration is retained separately from the final balanced comparison.

The pattern is now a function-local `static const std::regex`. Its literal expression and matching options are unchanged. Each process initializes it on first use and reuses it thereafter. Case folding and Unicode conversion still occur per call; existing path stripping, device prefixes, illegal/control-character replacement and length/extension logic remain. The public function and Job interfaces, catalog format and recovery ordering did not change.

## Compatibility and qualification

A new fixture passes **194 checks** on each drive. Twelve workers reach their first filename calls together, before any Manager or earlier `safeName` call, and perform 64 exact device-name normalizations apiece. Independent expected cases cover reserved names and near misses, paths, unsafe characters, embedded NUL, control characters, ordinary Unicode names, length/extension truncation, a surrogate boundary without an extension, idempotence and invalid UTF-8 refusal. The same fixture also passes against the qualified 8.56 engine.

All **1,926 checks across eleven final-binary-matched runs** pass. The retained suites cover catalog loading/failure preservation, Content-Disposition names, Properties decisions, catalog checkpoints, recovery catalog transactions, publication recovery and synchronization replacement. Filename rules, catalog loading and recovery catalog cases also run on D:. The old-engine 194 checks and initial new-engine focused 194 checks are excluded from the final aggregate. [Suite records](validation/filename-startup-8.57/suite-results.json), [validation notes](validation/filename-startup-8.57/validation-notes.json).

Five products, eight backend fixtures and one controller built from **515 frozen inputs and 42 core objects**. Core and Bridge were rebuilt; twenty-two qualified native and eighteen external units were retained after include-closure verification. All fourteen candidate file versions are 8.57. Baseline controller and fixture use qualified 8.56 core objects/resources and were compiled before changing version headers. Seven exact source inputs reconstruct both sides of the change with per-side line endings. [Build evidence](validation/filename-startup-8.57.json), [source reconstruction](../benchmarks/speed/experiments/filename-startup-8.56-to-8.57.json).

## Paired production-core measurements

The maintained [console controller](../benchmarks/speed/filename_startup_control.cpp) is compiled separately against each engine. It constructs the real Manager first in Desktop recovery mode, validates the loaded history, then measures a batch of 7,500 `safeName` calls. Constructor and filename boundaries record wall time and current-thread cycles. Normal snapshot/save/shutdown occur after the measured intervals; history is persisted and each process exits normally.

Three balanced fresh-process pairs (A/B, B/A, A/B) at each of 0, 2,500 and 7,500 records use identically reseeded owned catalogs and 2 KiB descriptions. The exact same controller body produces both engines' reports. All 7,500 normalized names are compared exactly across every process before retaining their compact SHA-256 evidence.

Median wall milliseconds were:

| History rows | Constructor 8.56 | Constructor 8.57 | 7,500 names 8.56 | 7,500 names 8.57 |
| --- | ---: | ---: | ---: | ---: |
| 0 | 6.4198 | 6.7617 | 40.2226 | 11.9331 |
| 2500 | 147.8488 | 140.3093 | 34.7222 | 11.2685 |
| 7500 | 810.0499 | 775.5463 | 75.4342 | 17.4948 |

At 7,500 history rows, median filename-batch thread cycles fell from **132,761,189 to 39,649,085** (about **70.1%**). The corresponding reductions at the other row counts were about 68–71%. The isolated filename batch consistently did less measured CPU work.

Whole-constructor wall time is much less conclusive. The candidate was slower in **two of the three 7,500-row pairs**, despite its lower median. Old-engine observations ranged from 447.5805 to 1,329.2641 ms; candidate observations ranged from 711.0681 to 881.0762 ms. The 2,500-row candidate includes a 595.6569 ms observation. Zero-row medians were slightly slower in the candidate. No observation was removed or rerun to obtain a win. These are not comparable directly to the preceding hidden-App timing series: this controller, catalog shape and measurement boundary differ.

The evidence supports reduced filename CPU work, not a consistent full-startup or universal latency bound. Constructor time includes catalog IO, parsing and recovery. First pattern setup is included where reached; OS caches may be warm and external activity is uncontrolled. Three samples per engine per count do not establish statistical significance. No visible paint, File Info delay, external pipe acceptance, installed startup, reboot-cold launch, IDM comparison or total background-work timing is measured. No kernel cause is inferred from the wall-time variation. [All observations](validation/filename-startup-8.57/startup-comparison.json), [pre-change calibration](validation/filename-startup-8.57/baseline-calibration.json).

An ordinary source-app 4 MiB download independently passed exact length/hash, a Complete catalog digest, one origin request, normal process exit after worker work and a separate completed-output flush. Its payload was removed immediately. This single run is an integrity/lifecycle check, not a finishing-speed comparison. [App record](validation/filename-startup-8.57/app-4m-smoke.json).

## Separate Unicode finding and next work

The qualified 8.56 core also reproduces a correctness issue outside the regex change. A valid UTF-8 name containing 139 ASCII stem characters, an emoji, further stem characters and `.mp4` is truncated at the 140-unit UTF-16 stem boundary. The resulting high surrogate sits before the appended extension rather than at the end, so the existing final-character check misses it and UTF-8 conversion throws `Invalid Unicode text.` A 138-character prefix leaves the pair intact and succeeds. The exact two-case source and report are retained; this probe is exploratory and excluded from the passing suite aggregate. Truncation code is unchanged in 8.57. [Baseline finding](validation/filename-startup-8.57/unicode-boundary-baseline.json), [probe source](validation/filename-startup-8.57/unicode-boundary-probe.cpp).

Next fix truncation at each independently clipped UTF-16 boundary before combining stem and extension. Add cases around both sides of surrogate pairs, preserve ordinary ASCII/Unicode names and extension limits, then recheck actual admission and catalog loading. Keep this distinct from the regex performance evidence. The constructor's remaining parsing/IO/recovery work needs finer phase evidence rather than conclusions from these noisy totals.

The original final-file flush stalls, kernel cause attribution, installed/browser acceptance, external namespace/caller audits and IDM finishing-speed parity remain open. No elevated trace was run for this filename change.

## Cleanup

Removed **670 owned generated files / 267,417,915 logical bytes**, plus the separate 4 MiB payload, and 644 empty owned directories. Synthetic catalogs, fixture/controller/probe executables and entry objects were retired after exporting compact evidence. Cached canonical roots, bounded resolved paths, reparse refusal and complete hard-link-group checks constrain deletion. Five products, Core/Bridge objects, forty shared core references, resources, prior qualified references, shared tools, personal catalogs and installed executable remain. Logical size is not a physical-reclamation claim. [Cleanup receipt](validation/filename-startup-8.57/cleanup.json).
