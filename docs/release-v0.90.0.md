# UDM 0.90.0

This cumulative release advances from UDM 0.89.0 with live HLS delta-update support, safe full-playlist recovery, and discontinuity-aware output assembly.

## HLS delta updates and recovery

- Read `EXT-X-SKIP` delta updates during live recording and browser handoff.
- Preserve discontinuity boundaries while applying delta updates.
- Retry the complete live playlist when delta history has expired, keeping signed query bytes intact.

## Recorded media assembly

- Assemble captured HLS segments across timestamp discontinuities without flattening their boundaries.
- Record validation for delta boundaries and complete-playlist fallback cases.

## Verification

Focused native and browser HLS checks are recorded with the source. The installer is built from this tagged source; encrypted or DRM-protected media and full IDM parity remain outside the supported scope.
