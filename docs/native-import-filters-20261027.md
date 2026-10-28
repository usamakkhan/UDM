# Native import visibility filters

Native batch/text import now exposes Hide HTML files and Hide repeated files,
corresponding to reference IDM dialog 228 controls 1055 and 1060/1849.
HTML recognition uses .htm/.html names and the text/html or
application/xhtml+xml response MIME types. A server filename takes precedence
over the URL filename for this classification. User output patterns do not hide
the original type. Repeated links are exact address matches; the first original
row is retained, independently of display sorting.

These controls filter visibility, retaining each row's checked state. The counter
includes all selected rows and identifies the hidden selected count. Check all
and Uncheck all affect the visible list. On submission, visible selected rows
follow display order, then hidden selected rows follow original input order.
Names for every selected row are validated before any admission. Existing
duplicate policy remains responsible for resolving repeated download requests.

Metadata can change HTML visibility while the dialog is open. Rebuilding the
visible set preserves row IDs, checkboxes, the selected row when still visible,
and the current sort direction. Per-file editing resolves the original row ID
again on return, rather than relying on a position that may have changed while
the editor was open. Added rows remain guarded against accidental resubmission.

This is behavior implementation from the retained reference controls, not proof
of pixel-identical geometry or complete import parity. Native text/batch imports
have no source-page image context, so Hide images located on this web page
remains part of the browser import workflow.

Runtime and regression testing are deferred at the user's request. Source review
and the native build are recorded in
[the candidate record](validation/native-import-filters-build-20261027.json).
The candidate is staged, not installed.
