# UDM versioning policy

User direction, 2026-10-29: every functional improvement must advance the application version. Do not accumulate successive improvement batches under the same version.

- Advance the desktop and both browser manifests before building each separately delivered improvement, including native-only and browser-only changes. Multiple outputs from the same build count once.
- Use at most two digits per component. Carry after 99. Desktop 2.99 advances to 3.0 and browser 0.66.99 advances to 0.67.0.
- Keep all three definitions in native/Version.hpp synchronized. All five production binaries must use that version; any later release installer must match.
- Add a matching CHANGELOG.md entry explaining the changed behavior.
- Preserve historical build records and artifact names. Never relabel an older executable as the new version.
- Record source, build, test, installation and publication status separately; a source bump does not establish a release.

User direction, 2026-10-30: continue native development and complete native builds, but do not create setup packages for each improvement. Keep tests deferred and maintain changelog/build records for a later major release. Do not infer a release number, publish, install, or generate a new installer from a successful development build alone. Existing setup artifacts remain historical.

Version 1.2.3 records synchronization recovery publication-state and review-required retry fixes. Version 1.2.4 adds browser-recapture retry exclusions and includes those fixes. A 1.2.4 installer has been built; testing, installation and publication remain separate steps. Historical 1.2.2 installers remain unchanged.

## Historical increments

Use the recorded numeric distance between original versions so jumps preserve their increments. Count a distinct build that reused a number once, using a build receipt, release tag or documented development delivery to establish it. An already-accounted browser-only version can be listed in the ledger at its existing position. Repeated builds of the same source and commits without evidence of a separate delivery add no increments.

| Historical evidence | Accounting correction |
| --- | --- |
| [Native 0.28.0](parity-0.28.0.md) | Add one paired increment for the verified offline-site/SOCKS5 build. Its browser manifest version is unspecified in the report. |
| [Native 0.75.0](candidate-native-0.75.0.md) | Add one paired increment for the compiled automation candidate. Its browser manifest version is unspecified in the report. |
| [Recorded 1.105.0](validation/captured-response-durable-pause-20261105-build.json) and [first recorded 2.05](validation/native-2.05-20261105-build.json) | Count the durable Pause and local-save progress builds separately as 2.05 and 2.06. |
| [Later recorded 2.05](validation/native-2.05-current-build.json) | Count the context-menu build after the dark-menu build and before the drop-target build. It fills the missing slot in the desktop sequence. |

The [changelog](../CHANGELOG.md) shows the revised historical sequence. These four corrections move the prior accounted browser version from 0.66.41 to 0.66.45 and the desktop version from 2.80 to 2.81. The reconciliation build advances to 2.82 and browser 0.66.46.

## Additional historical deliveries

The third audit recovered 42 increments from installer delivery notes and the [0.84 candidate history](current-staged-candidate.md). The [historical ledger](../CHANGELOG.md#additional-historical-builds) lists the individual changes and gives repeated 0.84 desktop builds separate numbers. Browser-only work with an existing numeric slot keeps that slot. A rebuilt installer, a clean rebuild of the same functional source, and test-only milestones add no increment.

Apply the 42 recovered slots to the historical browser sequence from the point where each was missed. Carry the total into the active desktop sequence without changing the versions stamped into earlier binaries. The previous build was recorded as desktop 3.2 and browser 0.66.66. Restoring the missed increments brings the running count to desktop 3.44 and browser 0.67.8. The reconciliation build advances to desktop 3.45 and browser 0.67.9.

## Fourth historical audit

The [GUI delivery ledger](../CHANGELOG.md#earlier-gui-delivery-accounting) restores 39 separately documented functional candidates. Five audit/verification-only notes add nothing; the completed-file Properties build is already counted. This brings the recovered historical count from 135 to 174.

Allocate the missing block before the prior Properties accounting position. Historical headings and ledger browser positions carry those 39 increments; older executable versions and factual receipts stay unchanged. Assigned desktop patch labels distinguish the GUI candidates without claiming an undocumented delivery chronology.

The prior development build was stamped 3.65 / 0.67.29. Adding 39 recovered increments brings the running count to 4.4 / 0.67.68; this reconciliation advances it to **4.5 / 0.67.69**. Carry after 99 continues to apply. Historical arithmetic descriptions above record their earlier audits, rather than current versions.
