# DASH caption assembly recovery compatibility

Development version 1.16.1; browser source remains 0.63.85.

Live-DASH Period and joined-session output receipts now include caption assembly revision 2 in their recording binding when the selection contains subtitles. Outputs assembled before the ctim/inline timestamp mapping changes are rebuilt from retained verified fragments instead of being accepted solely because the recording index is unchanged. The session also binds each Period using its caption assembly identity.

The revision is confined to assembled-output reuse. Recording indexes, segment journal bindings and source-fragment hashes retain their existing identity. Audio/video-only assembly keeps its previous binding. Future caption decoding, clipping or mux changes must advance the revision in dashAssemblyRecordingDigest.

Recorded non-live adaptive output is assembled anew in Adaptive.cpp; its confirmation wait retains the exact muxed bytes within the current transfer, rather than restoring an assembly receipt across app versions.

Build status: passed for all five production targets; embedded versions and SHA-256 hashes recorded. Source hashes matched the pre-build snapshot. Runtime tests, installer creation, installation and publication are deferred. The installed app remains 1.6.0. See [build receipt](validation/dash-caption-assembly-recovery-build-20261104.json).
