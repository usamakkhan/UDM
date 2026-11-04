# Imported download request context — 1.6.7

Catalog import previously preserved missing form/media context only for unfinished records. Attaching a hash-verified completed file discarded those flags. Download Again could consequently create an ordinary URL request from a form response or a captured media item. Export also did not retain RequiresMediaCapture on records lacking their original protected media plan.

Import now preserves form/media recapture flags independently of file completion. Verified attached files retain Complete status, their bytes and hash, and remain outside pending queue membership. Unfinished imports still show Needs browser capture. Export carries the media flag through further catalog round trips.

Download Again checks required form/media capture before any new admission and validates the browser session on all eligible paths, including ordinary direct files. Existing complete output stays available; obtaining fresh bytes requires the missing request context. Available protected POST requests retain the existing replay path; browser proxy validation remains in that path.

This change cannot reconstruct flags already discarded by an older import. Restoring those records requires their original request context; this update does not migrate them. Tests are deferred by user instruction; compile success is not proof of browser or import round-trip acceptance.

Native version: 1.6.7. Browser remains 0.63.82. Installed UDM remains 1.6.0. No installer, installation or publication is requested.

Build record: [native receipt](validation/catalog-recapture-context-build-20261101.json).

All five production targets built successfully at 1.6.7. Build-start source hashes remained unchanged; executable versions, sizes and SHA-256 hashes are recorded in the receipt.
