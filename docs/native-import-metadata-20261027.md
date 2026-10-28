# Native import file information

Native batch/text import now starts background metadata lookups when opened.
The list shows server-reported Size in bytes and MIME Type, and uses a safe
Content-Disposition filename suggestion when the user has not specified a
per-file name or batch output pattern. Suggestions are display information;
the actual transfer still resolves and validates its own response.

File name, Address, Size and Type headers sort the displayed rows. Unknown sizes
and empty types remain last in either direction. Equal values keep original
input order. Newly arriving metadata updates cells without continually moving
rows; clicking a header sorts the current values again.

The retained IDM dialog-228 import list and analysis at 004c14b0 show per-link
filename/type information; the size comparator at 004bbd30 reads each record's
64-bit size. This change fills that native import gap with UDM's existing
metadata parser. It does not establish pixel or timing equivalence.

## Lookup lifetime and request policy

`BatchDownloadPreview` runs at most four workers for one dialog, with an
eight-second deadline per link. Workers hold copied preferences, protected
headers and shared result state, never UI or Manager pointers. Closing or
restarting the preview cancels its shared parent token; the dialog does not join
network threads. Results from an old generation cannot reach the replacement.

HTTP uses the existing HEAD-only `probeDownload` path, including response status,
content-encoding, size and Content-Disposition validation. It does not fall back
to GET. FTP uses the existing FTP metadata path. Servers that refuse metadata,
timeouts and missing lengths display Unknown; import remains available.

Changing a URL or accepting shared authorization starts a new lookup generation.
Already-added rows are omitted. Results are applied by original row ID so sorting
cannot attach one link's details to another. Per-file names and output patterns
take precedence over server suggestions. Metadata does not become a resume
validator or overwrite download catalog data.

## Delivery status

Runtime, browser and regression tests are deferred at the user's request. Build
and source-review evidence is recorded in
[the candidate record](validation/native-import-metadata-build-20261027.json).
The candidate is not installed. Automatic shared-extension adjustment from the
first non-HTML batch response remains a separate open parity item.
