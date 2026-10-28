# Metadata inside native import File Properties

The retained IDM File Properties dialog (resource 135, reached by the import Edit command) includes Type, Status and Size labels, with value controls 1093, 1098 and 1095. The reference executable SHA-256 is 03CC62E9ADB77A380F9DC12F67CCAAEE5106F12844AA73CE32C914DDD16D607C. These controls are absent from UDM's earlier import-only editor even though the parent list already receives metadata.

UDM's import editor is now titled File Properties and shows MIME type, metadata status and the exact server-reported byte count. It reads the existing row metadata rather than starting another request. Zero-byte files are displayed as zero bytes; missing or failed lookups remain Unknown. A ready lookup shows Not started because these rows have not been admitted as downloads.

The summary follows asynchronous metadata updates while the editor remains open. Unsaved address, authorization-mode, login, password or Referer edits hide the old type/size and request saving to refresh details. Reverting the request edits restores the current metadata. Filename, destination and description edits do not invalidate response metadata. The byte count is informational response metadata, not a new promise about assembled media output size.

All existing editor controls were shifted below the summary; native import request and destination behavior remain intact. Runtime, DPI and keyboard testing are deferred at the user's request. Compilation and packaging are recorded separately, and the candidate is not installed or published.
