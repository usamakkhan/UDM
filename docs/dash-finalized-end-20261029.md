# Completion after a finalized DASH manifest

The live recording loop previously required a static manifest and an explicit resolved Period duration before setting its final capture boundary. A final static manifest with a finite timeline, list or index but no duration could therefore keep waiting after all its media had arrived.

The loop now derives a final common interval only after the manifest is static. Explicit Period duration remains authoritative. Otherwise listed/indexed addressing uses its final reference; template addressing walks finite timeline runs, resolving open repeats through the next explicit timestamp or endNumber. Unbounded final runs remain unresolved. Integer additions and multiplications are checked against the supported exact clock range before converting the resulting boundary to seconds.

Every selected track must supply a finite end. The shared end is the shortest selected-track interval, matching the existing common-interval sealing behavior. Capture still waits for verified initialization and committed contiguous media to reach that boundary before saving. A boundary at or before the recording start produces an error instead of an endless wait. Existing transitions and user Stop handling remain in place. The capture boundary is persisted through the existing recording checkpoint.

A dynamic manifest is never finalized merely because its current segment list or endNumber is finite: those may grow on a later refresh. No inference from a request failure, timeout or temporary absence of segments was added.

Extension 0.63.68 is unchanged. All five native production targets and the installer compiled successfully; compiler warnings remain. See the [build record](validation/dash-finalized-end-build-20261029.json) for source and installer hashes. Runtime final-refresh, multi-track, interruption and playback tests remain deferred at user request. The candidate is unsigned, uninstalled and unpublished.
