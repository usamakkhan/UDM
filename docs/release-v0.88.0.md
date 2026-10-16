# UDM 0.88.0

This cumulative release advances from UDM 0.87.0 with desktop refresh and recovery performance work, stronger Site Grabber filename handling, and expanded recorded DASH index parsing.

## Desktop responsiveness

- Populate the download list as the main window enters its message loop rather than waiting for the first periodic refresh.
- Avoid rebuilding unchanged download rows and the navigation tree while Grabber progress changes.
- Skip work for hidden history columns and snapshot quota-wait state once per refresh.
- Reduce completed-history queue work and avoid copying jobs while comparing idle checkpoints.

## Site Grabber and media discovery

- Apply the same bounded Content-Disposition filename parsing to Grabber downloads and accept server-provided matching filenames for opaque URLs.
- Support consecutive external DASH SIDX roots and sibling same-file child indexes with bounded, contiguous range and timing validation.

## Verification

Focused desktop, Grabber, DASH and backend checks are recorded under `docs/validation/`. This package is built from this tagged source; broader public-site and complete IDM-parity claims remain outside this release.
