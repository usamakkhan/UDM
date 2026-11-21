# Silent launch failures and current parity evidence — 9.59

Fix caught startup failures for silent command-line requests: report to inherited error output when available and exit with status 1 instead of waiting on an error dialog. Validate instance tags before opening a catalog, while preserving quiet-looking option values and successful/recovery exit behavior. Build five native 9.59 products; pass 199 backend and 40 real App launch assertions. Source browser manifests advance to 0.73.23. New 9.58-versus-IDM observations win all nine whole-process and output-durability pairs but lose six first-availability pairs; full parity remains open. Installed 8.68 is unchanged.

## Actual app fix

A silent `/n` command could still block on `AfxMessageBox` if launch parsing, instance-name validation or storage initialization failed. Its caller could wait indefinitely for a UI action, and after dismissal the startup failure could return a success-like exit status. The first private transfer launcher accidentally exceeded the supported tag length and exposed this behavior. That invalid launch never started a download and is excluded from timing totals.

`LaunchFailure.hpp` determines the error-presentation intent before full argument parsing can throw. It skips values for every supported value-taking option: a filename or another value spelled `/n` or `--silent` is data, not a quiet-mode switch. A genuine quiet switch is recognized even when another argument later fails. Normal parsing remains authoritative for the request itself.

Caught silent startup failures now write a UTF-8 `UDM: ...` diagnostic line to inherited stderr when available, then return exit status 1. A missing, invalid or unavailable error handle does not cause a replacement dialog or create a console or log file. Interactive failures retain their error dialog and return error status after dismissal. This does not cover crashes or errors outside the application's existing catch boundary.

`Launch.hpp` validates nonempty instance tags against the pipe's existing limit: 1–32 ASCII letters, digits, underscores or hyphens. Valid tags and the existing empty default-context behavior are preserved. Successful download shutdown and recovery cancellation retain their prior exit semantics; only a caught startup failure overrides the final exit code.

## Verified behavior

All 199 backend assertions pass, including 49 new quiet-hint/parser/error-handle checks, 44 existing CLI/queue completion checks and affected settings/close/Exit tests. The real product also passes 40 black-box assertions across ten malformed or failed startup requests. These cover quiet switches in different positions, long or malformed tags, missing values, an unknown option, a bad process identifier, a missing Internet Shortcut, a file obstructing the data directory and discarded stderr. They check error status, explanatory output where redirected and absence of an opened catalog. The obstructing file remains intact.

The old 9.58 product does not exit within its three-second bounded control. It runs on an explicit private desktop, then is stopped through its exact owned process handle. That control's desktop is not child-attested, and the modal cause is inferred from matching source rather than a visual capture. No desktop is switched and no GUI input is injected. The new product's tested failure requests exit in approximately 50–317 ms, including process startup; these are not general startup or download-dialog latency measurements.

A successful 4 MiB `/n /q` transfer also completes with the expected file hash, a Complete catalog record and normal UDM exit status 0. The paired IDM control completes the same payload; both files are removed after verification. That single current-version pair is a success-path control, not a complete new performance qualification.

[Black-box harness](../benchmarks/ui/silent_launch.py) and [sanitized results](validation/silent-launch-9.59/status.json) record the exact binaries, 676 source inputs, dependency closure, core hashes, resources, compiler logs, backend checks and failed-launch source reconstruction. The repository ignore rules now admit this harness and its shared process/desktop helper sources; the regression no longer depends on ignored local test code. Existing compiler warnings remain. Private build wrappers are not a substitute for the still-pending clean-checkout GUI build profiles.

## Fresh actual download comparison

Before compilation, the hash-pinned 9.58 App was compared with installed IDM on a deterministic loopback server: 4 MiB sequential, 64 MiB sequential and 64 MiB split, three trials each with alternating client order. Both clients use the same configured connections and endpoint; split-request behavior and actual concurrency differ and are retained in the raw data. No sample or outlier is discarded.

All 18 final files match their independently expected hashes. Actual process-exit times include app-owned finishing and shutdown work, and an additional symmetric output flush is measured after exit. These measures do not include every Windows service or establish visual completion-dialog timing. The first-availability observer samples an exact-size output that can be opened with exclusive sharing; its 2 ms polling interval cannot establish a universal 1 ms bound.

| Median after the last origin body send | UDM 9.58 | IDM |
| --- | ---: | ---: |
| 4 MiB sequential, first availability | 152.5 ms | 81.9 ms |
| 4 MiB sequential, through app exit | 237.4 ms | 827.1 ms |
| 64 MiB sequential, first availability | 193.1 ms | 221.0 ms |
| 64 MiB sequential, through app exit | 289.5 ms | 849.0 ms |
| 64 MiB split, first availability | 301.8 ms | 254.8 ms |
| 64 MiB split, through app exit | 378.9 ms | 751.9 ms |

UDM wins all nine paired full-process tails, total elapsed times and symmetric output-durability measurements. It loses six of nine first-availability tails. The availability gate fails; the earlier whole-process wins do not close that gap or establish universal performance parity. These are development-binary observations, not an installed 9.58/9.59 comparison or proof that this CLI fix improved throughput.

All nine IDM launches return status 1 despite complete hash-correct output and process exit; the retained historical 8.68 controls show the same value. Its meaning has not been independently established. Raw exit statuses remain in the evidence; they are not converted to zero or silently dropped. UDM's compared transfers return 0.

A separate six-transfer preflight compares the real 9.58 App with installed 8.68 using isolated catalogs. The newer App wins full elapsed time in each single pair but loses both sequential post-network tails. The corresponding recorded file-flush and publication-intent waits account for much of those observed differences. This is a phase association, not proof of a causal code regression from one sample per configuration.

[All 24 initial/preflight transfers and every loss](validation/production-comparison-9.58/status.json), [paired raw metrics and gates](validation/production-comparison-9.58/paired/summary.json), [current 9.59 success control](validation/silent-launch-9.59/success-control/terminal.json).

## Next architectural work, in order

1. Close first availability before adding more menu optimizations. Correlate the final origin byte, writer retirement, metadata, published-file flush, durable intent, rename, final checkpoint and proof-handle release on one clock. Start with `Transfer.cpp`, `Duplicates.cpp`, `PreparedTransferCommit.hpp`, `Core.cpp` and the existing publication/disk-credit tests. Preserve raw first-availability, full elapsed, full process-exit and post-exit durability metrics separately.
2. Test whether exclusive-share availability is delayed by a retained publication proof handle or by work that must finish before rename. Distinguish the path becoming visible from an exclusive open, a normal reader, the UI completion event and actual application exit. Do not treat those as interchangeable endpoints.
3. Only after identifying a dependency, prototype overlap between independent publication preparation and final writeback. Preserve durable intent before rename, required hash checks, replacement/rollback behavior, catalog completion invariants, retained recoverable parts and joined worker lifetime. Do not remove a flush, weaken a durability check or add pacing merely to make an availability chart green.
4. Qualify ordinary, split, copied/across-volume, removable/network, failed-flush, cancellation, replacement and restart-recovery paths before promoting a publication change. Repeat the matched cases with all results retained and compilation stopped. A single favorable pair is insufficient, and no extra background work may disappear from the measured endpoint.
5. Then move toolbar directory/descriptor discovery and passive custom-skin reads off the UI thread. Menu preparation must preserve the complete file/name snapshot and its stable command-ID mapping; it must not show an incomplete menu as an apparent speed improvement. Preserve custom-skin freshness, error fallback, selection identity, cancellation and joined shutdown.
6. Complete installed/browser parity and visual checks when Windows control works again, then update the deployment as a coherent app/extension pair. Current control failed twice with `failed to write kernel assets`; no visual identity or live browser comparison is claimed here. Startup, remaining preference paths and clean-checkout native GUI profiles also remain open.

## Delivery and cleanup

Five native products are built at 9.59: app, browser host, monitor, setup support helper and automation host. Both source browser manifests are 0.73.23; the extension increment is paired version bookkeeping, without a new runtime capture change in this increment. No installer, installation, commit, push or release is created. Installed app 8.68 and extension files 0.72.32 remain unchanged.

All 26 downloaded test payloads were verified and removed (1064 MiB logical total). Artifact cleanup removed another 195 task-owned files (114.7 MiB logical size). Current and previous products, referenced core objects, installed files, both personal UDM catalogs and shared ffmpeg/ffprobe are preserved. IDM connection values are restored; the other Windows session's IDM is not stopped. Test download history entries in IDM are not claimed to be removed.
