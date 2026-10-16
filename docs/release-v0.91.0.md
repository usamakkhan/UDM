# UDM 0.91.0

This cumulative release advances from UDM 0.90.0 with a refined dark desktop interface and more responsive recorded-media verification and recovery.

## Desktop interface

- Refine dark surfaces across the main bar, menus, status controls, download grid, scrollbars, shared dialogs, date controls, and combo boxes.
- Avoid redundant list rebuilding and cache clipboard snapshots while the desktop refreshes.
- Integrate recorded HLS-map display refinements.

## Recorded media and recovery

- Keep the interface responsive while hashing saved media parts and honor Pause during verification.
- Remove failed assembly output instead of leaving a misleading partial result.
- Handle recorded HLS subtitle-header changes and subtitle discontinuities.
- Reduce repeated capability calls and map scans, and improve restart receipt recovery for large catalogs.

## Verification

Focused UI, recorded-media, and recovery validation is recorded with this source. This package is built from the tagged revision; full IDM parity remains unfinished.
