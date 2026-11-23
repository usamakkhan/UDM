# Pending DASH alias recovery — 10.7

UDM can resume an unfinished full-resource SegmentList entry when another saved entry has already pinned the exact same ordered URL list. This also applies when a saved segment pins another track's pending initialization resource. Native version advances to 10.7 and both source extension manifests to 0.73.71; browser scripts are unchanged.

## Observed failure and correction

The 10.6 request reduction introduced explicit pending-version intents. On restart, version discovery reused a saved descriptor for matching source locations. The capture planner then compared this newly known version with the pending record's previous null version and rejected it as a replacement. A correctly rejected source conflict could leave the unfinished alias unable to resume even after the original version returned. Accepted bytes remained intact.

For a segment, capture still requires identical number, time, duration and byte range. A differing version may be adopted only when the old record is explicitly marked as pending, has no version or accepted resource, has no byte range, and its original ordered URL list exactly matches the current one. A non-null known version is required. Capture updates the binding and planned descriptor together, validates all source-version constraints, and saves the complete plan before workers issue any body request.

For initialization, the exception requires an explicit unbound pending-initialization policy and exactly matching locations. Existing accepted initialization, legacy unmarked records, changed locations, changed coordinates and differing already pinned versions retain their rejection rules. No accepted data is relabeled, and no fresh response can bypass version admission.

This is a restart-planning correction. The response-body writer, accounting, length/hash checks, flushes, receipts, worker bound and cancellation ownership are unchanged. Frozen 10.6 core objects remain available for controls.

## Qualification

The single-track actor reproduces the 10.6 segment failure after a conflicting response, then demonstrates a successful 10.7 retry with exactly one conditional media GET for the unfinished entry and no refetch of the accepted entry. Final records and physical bytes are checked.

A separate two-track actor runs against frozen 10.6 and current core objects. The control is terminated after its first track has committed a receipt and its second track has requested a shared initialization file whose headers remain held. Recovery confirms an accepted first track and an unbound pending initialization. Frozen 10.6 rejects the retry before any media request. Current code resumes with one conditional initialization GET and one audio-segment GET, retaining the first track. All three physical files are compared with fixture bytes and SHA-256 values. Both retries start from copies of the same small protected checkpoint and receipt set.

Abrupt-stop controls terminate only owned actors before headers, after durable version admission but before body release, and after a partial body is physically written. Restart preserves the expected bound/unbound policy, rejects a changed source when already pinned, and completes one full GET of the original version. Client accounting, successful server write count, stored length and SHA-256 agree. These are process-interruption checks; they do not simulate power loss or cover every final receipt transition.

The first crash observer encountered a transient sharing error during atomic checkpoint replacement. Its bounded observer now tolerates missing-file/sharing races; the failed attempt and corrected runs remain recorded. Native code was unchanged for that observer correction.

All five native programs build as 10.7. Verification pins 753 main source inputs, 42 core objects, 17 main executables and two supplemental alias controls. Only DashManifest and Bridge change from 10.6; the other 40 core objects retain their hashes. The supplemental control links current actor code and version resources against the complete frozen 10.6 core set; it is not an installed 10.6 application.

The current binaries pass 107 focused assertions, 17 single-pass HTTP workflow cases, three real process-crash cases, both segment/initialization alias regressions, seven continuous-recording cases and five direct-media plus four caption-session cases. The four positive continuous cases each publish once; physical final sizes and SHA-256 values match accepted assembly records. Gaps, short audio and changed identity publish nothing. Direct/caption fixtures preserve decoded-source oracles and final physical publication checks; their frozen capture baselines remain explicit. All actors terminate and origins close. Nine protected host-install hashes remain unchanged.

The [verification record](validation/dash-pending-alias-10.7/verification.json), [segment control comparison](validation/dash-pending-alias-10.7/segment-alias-current.json), [initialization comparison](validation/dash-pending-alias-10.7/initialization-alias-qualification.json) and [hard-stop results](validation/dash-pending-alias-10.7/crash-qualification.json) distinguish current passes, frozen failures and remaining gates. New metadata transport/clock behavior is unchanged; the 10.6 HTTP isolation/freshness records are historical evidence, not newly rerun 10.7 claims. The full build entrypoint, MIDL/tool discovery and installer packaging remain unexercised.

The build-volume reserve passes on C:, but D:'s workspace reserve still fails. Combined verification reports `passed: false`, `technicalChecksPassed: true` and `releaseQualified: false`. No installer, installation, release, explicit temporary-file deletion, browser comparison or actual IDM timing result is included.

## Storage and build placement

D: remains below the existing 1 GiB reserve. Attempted lossless NTFS compression of 24 retained 9.99 compiler objects preserved every logical byte and SHA-256 but recovered zero allocated bytes. A direct error check established that the volume does not support this compression configuration: unsupported filesystem compression or cluster size above 4096 bytes. The initial `/I` invocation suppressed its error status; exit status alone was not evidence of compression. No files were deleted.

C: currently has over 6 GB free. The bounded new candidate, compiler outputs and TEMP/TMP use its existing UDM-Development folder. Shared compiler/core/media tools are referenced at existing locations; a directory junction avoids copying FFmpeg. This build placement does not satisfy D:'s low-space gate or make the candidate release-qualified.

## Ordered remaining work

1. Run hard stops during final flush, completion receipt writing and receipt/index replacement; verify accepted states and orphan handling independently. Keep process crashes distinct from hardware power loss.
2. Add parallel workers pinning a shared URL with conflicting responses, source-list changes, alternative/redirect chains and credential rotation through full-resource admission. Retain strict legacy and accepted-record rejection boundaries.
3. Measure request counts, server writes, client reads/accounting, header latency and final background work together under repeatable conditions. Current fixtures prove specific identity/recovery behavior, not IDM timing parity.
4. Resolve non-aligned video preroll and interior AAC continuity across multiple Periods with decoded-source oracles. Preserve complete single-Period/caption results.
5. Resolve D: capacity before another installed/lab delivery cycle, then repeat browser handoff, File Info latency, exact pre-Start size and completion timing against installed IDM.
