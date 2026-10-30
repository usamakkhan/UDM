# Native live SegmentBase capture and recovery

Live startup now resolves indexed tracks before choosing a capture window or saving its bootstrap. The index reader's exact common clock, media ranges, media/index versions and reference ID are stored in an atomic protected checkpoint. The checkpoint identity includes the unresolved track identity and a digest of the source locators, preventing an abandoned pre-capture checkpoint from colliding with a later source selection.

Restart restores the checkpoint without fetching or guessing a new index mapping. It validates source identity, clock conversion, numbering, timeline continuity, byte-range order and version bounds before reconstructing the planned references. Missing or inconsistent state stops recovery and retains existing media. Recording identities additionally bind the index reference ID, version and source digest.

Manifest refresh preserves the resolved clock and ranges only when the indexed source remains identical. Changes to source locators, initialization, index ranges, manifest clock or availability metadata are rejected within the active Period. Period duration can become known through the existing refresh path. Adjacent compatible Period transitions resolve and checkpoint their own indexed tracks before capture, using the existing rate/quota accounting callback.

The native selected-track index gate has been removed now that startup and recovery supply the resolved addressing. Capture still rejects unresolved indexes directly. The browser catalog continues to exclude SegmentBase until its selection/capability integration is added. The strict strong-validator requirement remains: this path does not permit a shared media file to mutate underneath a recording.

Runtime startup, network, restart, cancellation, transition and playback tests remain deferred at user request. Extension 0.63.65 is unchanged. All five production targets and the installer compiled successfully; compiler warnings remain. See the [build record](validation/native-dash-base-capture-build-20261029.json) for source and payload hashes. No installation or publication is performed.
