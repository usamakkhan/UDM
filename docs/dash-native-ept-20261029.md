# Native DASH earliest-presentation offsets

UDM's native duration-template resolver now accepts eptDelta at or below zero when presentationTimeOffset + eptDelta remains nonnegative. The nominal timeline begins at that sum. Segment URL expansion subtracts eptDelta from the nominal timestamp, preserving the original $Time$ URI values. Numbering is unchanged. Existing window overlap, server-clock availability, finite-end inference and recording assembly consume the nominal timeline.

This implements the same distinction described by [DASH-IF section 18.4](https://dashif.org/Guidelines-TimingModel/#simple-addressing) and the [recorded browser path](dash-recorded-ept-20261029.md). The native resolver serves live recording and finalized-manifest draining. It rejects malformed/oversized offsets, positive initial gaps, negative media origins, eptDelta with explicit timelines or list/base addressing, and pdDelta. Incomplete low-latency chunks remain unsupported.

Nonzero eptDelta is included in segment/track identity and protected recovery checkpoints. A refresh that changes the offset within the same Period is rejected; saved files remain available rather than being silently reused under new timing. Zero-offset identities retain their prior representation. Broader changes to sliding duration-template numbering and offset rebasing remain outside this implementation.

Extension 0.63.78 marks live tracks whose inherited template declares eptDelta and requires the new live-dash-ept-offset desktop capability before handoff. Native parsing remains authoritative for the manifest contents, including refreshes and Period transitions. Both browser copies include the same catalog and handoff changes.

Four JavaScript syntax checks passed. All five production targets compiled and installer packaging passed. See [build evidence](validation/dash-native-ept-build-20261029.json); the production bundle manifest records all 147 payload hashes. Runtime manifest, availability, restart, browser and completed-recording tests remain deferred at the user's request. No installation or publication is performed; this does not prove IDM parity or working live downloads.
