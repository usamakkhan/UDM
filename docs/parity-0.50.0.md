# UDM 0.50.0 / browser 0.37.0 verification

This update closes several connected media-recovery gaps. Previously, pausing captured YouTube streaming discarded completed segments, retrying a failed assembly could receive the tracks again, and a new browser session normally created another download. Full IDM parity is not established.

## What changed

- Completed SABR video/audio segments are checkpointed atomically with SHA-256 hashes. Their identity binds the video, duration, format revision and language while excluding expiring transport credentials. Pause, failure and restart preserve completed segments. Reopening validates hashes and fetches missing or corrupt ranges. Parallel windows can reuse saved media after the connection count changes.
- **Refresh download address…** now opens **Refresh media session** for an eligible paused SABR job. Open the original video, play it, choose the same quality/audio track in UDM's browser panel, then choose **Save session** or **Save and resume**. The matching offer updates the existing record after review. Changed language/revision/output is rejected, expired candidates are disabled, and persistence failures roll back both parent and child metadata. The captured request stays encrypted at rest.
- Complete internal tracks are hashed and saved. **Retry assembly** reuses verified tracks even after the session expires. Output hash mismatch, cancellation and destination conflict preserve source data. Cleanup occurs only after successful publication. This retry does not contact YouTube when the verified tracks are already complete.
- Progress distinguishes receiving, assembly and verification; shows retained/reused bytes; and excludes pre-existing timeline coverage from ETA. Browser panels display a fresh-session review acknowledgment instead of claiming another download was added.

## Fresh verification

- 920 native checks passed, including the new restart, connection reshaping, corruption, audio-only recovery, language identity, expiry, encrypted capture, native pipe acknowledgment, rollback and offline assembly cases. Real FFmpeg-generated video/audio validates expected-hash failures and publication conflicts.
- 8 native-host framing checks passed against 0.50.0.
- 559 browser checks passed across 23 suites. A prior boundary test used a future timestamp only 1 ms beyond tolerance; its assertion raced execution time. The fixture now uses a clear invalid margin, and all 25 cases in that suite pass. Production timestamp policy is unchanged.
- Chrome and Edge each passed 13 isolated browser/UI checks with real native parallel downloads, selected-language output hashes, full AAC decoding and the new review banner. The page and generated audio are fixtures with an explicit test-only loopback substitution. The banner's native response is controlled; actual native refresh routing is covered separately in the C++ pipe test.

## Limits that remain

Both desktop and browser control tools failed before initialization with `failed to write kernel assets: The system cannot find the path specified`, including a desktop-tool reset. The new MFC dialog has compiled but has not had fresh visual/button acceptance. Existing browser-profile extension reload is not claimed. Isolated Chrome/Edge fixture coverage is separate from current-session activation.

The 0.43 public YouTube test was rejected with a browser-attestation requirement for both audio and video. This release does not establish that compatibility gap is resolved and does not claim a live YouTube speed increase. A fresh accepted browser session is still required when missing segments need downloading. Changed stream revisions require a new download; local conflicting/corrupt checkpoint indexes fail explicitly instead of silently mixing content. Existing direct media/HLS/DASH session refresh is outside this update.

Firefox shared code is synchronized, but no new Firefox UI or persistent-install run was performed. Driver behavior, Windows Test Mode and IDM files are unchanged. The finite comparison remains 88 implemented, 12 partial, 2 unverified and 1 English-only item deferred by the user; implementation is not proof of IDM equivalence. No publisher-signing or clean-machine acceptance is claimed.

## Package and evidence

The 0.50.0 / browser 0.37.0 setup package, hash, current-PC file verification and activation results are recorded in [deployment evidence](evidence-0.50.0/deployment.json). Source/binary backups and prior installers are retained. See [test summary](evidence-0.50.0/summary.json) and the [current comparison](idm-parity-0.50.0.md).

## Current-PC deployment result

[UDM-0.50.0-Browser-0.37.0-Setup-x64.exe](../installer-out/UDM-0.50.0-Browser-0.37.0-Setup-x64.exe) was built, and 45 updated files were deployed with SHA-256 verification and original-file backups. Installer SHA-256: `7EE9AC8297B33430A17B2AEDF667615EEE182B14EA1547139C1993E8CBF252D1`.

Native 0.50.0 is running on `D:/UDM/user-data`; all 23 download records remain byte-identical, and the signed network runtime is unchanged. Chrome/Edge isolated fixtures pass, but existing browser profiles need a UDM extension reload to activate 0.37.0. Current-profile activation and the new native dialog's visual acceptance remain unverified because the control tools failed before initialization.
