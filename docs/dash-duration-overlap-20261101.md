# DASH duration overlaps — 1.7.0 / browser 0.63.83

## Reference and scope

ISO/IEC 23009-1:2022 describes pdDelta as the representation-duration difference from the Period duration: a negative value indicates an ending gap, while a positive value indicates overlap. Absence means unknown, not a declared zero. See the attribute table on printed page 74 of the [standard text](https://previewnorm.com/iec/ISO%20IEC%2023009-1-2022%20PDF.pdf). [DASH-IF IOP Part 5](https://dashif.org/docs/IOP-Guidelines/DASH-IF-IOP-Part5-v5.0.0.pdf), Table 4, explicitly permits non-negative SegmentBase pdDelta for non-video ad tracks. These are format references, not evidence of actual IDM handling for these streams.

## Implementation

The native parser no longer rejects every pdDelta. It accepts signed decimal spellings whose value is non-negative and at most 2^53-1, including signed zero. Negative gaps and malformed or excessive values remain rejected. Existing segment references, timescales, presentation offsets and URI substitutions stay authoritative. The declaration does not extend the requested output interval or rewrite segment URLs.

Chromium and Firefox use a shared-equivalent validation rule across inherited template, list, base and representation-index addressing. Recorded selections require a finite Period and the existing Period-assembly contract, which clips output to the Period duration. Legacy flat assembly is not used for this metadata. Live catalog selections carry a durationBoundaryOverlap marker; sites.js requires live-dash-duration-overlap from the desktop before handoff. The native refresh parser independently validates the MPD.

The live recording path retains its existing availability-window selection, Period-end filtering and assembly bounds. Changing an overlap declaration does not change an already-bound segment identity; its actual clock/resource identity still controls reuse. The metadata is not treated as proof that the server supplied complete media.

## Delivery and evidence limits

Native version is 1.7.0 and both browser manifests are 0.63.83. Runtime tests, public-site comparisons and installer creation remain deferred by user instruction. No installation or publication is requested; installed UDM remains 1.6.0 with browser 0.63.82. Negative duration gaps, positive initial gaps and other documented DASH limitations remain open.

Build record: [native and browser syntax receipt](validation/dash-duration-overlap-build-20261101.json).

All five production targets built successfully at 1.7.0. Build-start native/browser source hashes remained unchanged; executable versions, sizes and SHA-256 hashes are recorded in the receipt. Both browser manifests parsed as 0.63.83 and all four changed JavaScript files passed node --check. No runtime tests ran.
