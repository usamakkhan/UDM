# Native DASH recording assembly

The native DASH module can now assemble a locally captured single-Period audio/video recording into a staging MP4 or M4A. It first reconciles receipts, requires every indexed media segment to be committed, rejects pending/unverified initialization, and orders segments numerically. Track clocks, identity and declared kind are validated. At most one video and one audio track are accepted; live subtitle assembly and Period transitions remain unfinished.

Each local track input contains initialization followed by its media segments. Source files are opened with write/delete sharing denied, checked against their saved size/hash under that lock, and copied into uniquely created local inputs. Cancellation is checked during verification and copying. Output names use a fresh identifier and FFmpeg is instructed not to overwrite existing output. The existing local-only protocol whitelist, process runner, stream probe and coarse duration guard are reused.

Assembly subtracts each track's presentation offset and the selected common capture start. The returned result exposes that start, duration, staging path, size, hash and temporary inputs. Capture indexes now preserve the known Period duration separately from the stable track identity so assembly can exclude samples beyond the declared Period end. Unknown/older Period-duration metadata remains unknown. Track coverage differences greater than half a second fail rather than silently dropping a materially longer track; smaller differences use the common interval and packet-granular stream-copy boundaries.

Inputs and output are probed for the expected audio/video stream counts, declared video height, codec configuration and readable time base. Output duration must be within the existing half-second tolerance. These are coarse structural checks, not decoded-frame completeness or sample-accurate synchronization proof. Runtime media verification is still deferred.

## Remaining integration

The assembly function calls reconciliation and the existing media process helpers. The running job loop still needs to drive capture/resume/stop, call assembly, publish the staging file through Manager, and clean temporary files after successful publication. The function deliberately does not publish or erase the recording. Browser live admission remains disabled pending that integration.

Production compilation and packaging are recorded in `validation/native-dash-live-assembly-build-20261028.json`. No FFmpeg process or live transfer was exercised during implementation. Browser 0.63.61 is unchanged; the candidate remains unsigned, uninstalled and unpublished. Full IDM parity is not established.
