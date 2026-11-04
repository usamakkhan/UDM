# Live HLS publication recovery — 1.2.10

The live HLS finalization catch block unconditionally removed its assembled staging file on any error. If publication had already created a durable publication or replacement journal, that deletion could remove the new bytes required for recovery.

Cleanup now preserves staging whenever either journal exists or its existence cannot be read reliably. The shared media publication helper uses the same conservative predicate. Before running FFmpeg with overwrite enabled, live HLS checks for pending publication recovery and refuses to overwrite the preserved staging file. The error directs the user to restart UDM, whose catalog initialization already invokes publication/replacement recovery. Sealed recording inputs remain available because cache cleanup only follows successful publication.

Native version is 1.2.10; browser 0.63.82 is unchanged. All five native production targets compiled successfully and report 1.2.10. Source and binary hashes are recorded in [build evidence](validation/live-hls-publication-recovery-build-20261030.json). Tests are deferred. Per the latest user instruction, no installer, installation or release will be generated for this development increment. Installed UDM remains 1.2.8, and the latest historical installer is 1.2.9.
