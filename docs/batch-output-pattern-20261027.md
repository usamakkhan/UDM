# Batch output filename pattern

Generated numeric and letter batches now carry their original formatted sequence
tokens into native import. The import dialog exposes a File name pattern field
only for those batches. Every `*` is replaced with that row's original token;
sorting or deselecting rows never renumbers the output.

For example, a numeric batch starting at 3 with width 3 and pattern
`photo-*-copy-*.jpg` previews `photo-003-copy-003.jpg`. Descending and letter
ranges use the same path. Blank patterns retain normal server naming. Explicit
per-file names override the shared pattern; clearing an override uses the pattern
again. Changing the pattern previews all not-yet-added rows, while rows already
admitted after a partial failure retain their submitted names.

Names are bounded and checked with UDM's existing safe filename rules. A nonblank
pattern must contain an asterisk. Invalid patterns show an inline message and
retain the last successful preview. All selected generated names are validated
before admission or queue creation, so an invalid later row cannot cause a
partially submitted batch. Ordinary text-list imports remain server-named unless
edited individually.

## Evidence and scope

The token association and replace-all behavior follow the retained reference
analysis described in [batch sequence behavior](batch-sequence-reference-20261027.md).
UDM's code is independently written. This completes explicit output-pattern
substitution, not all of IDM's filename workflow: its metadata-driven shared
extension adjustment remains unimplemented, and live visual/behavior equivalence
has not been established.

The source was reviewed and built. Runtime, browser and regression testing remain
deferred at the user's request. The candidate is staged, not installed; see the
[build record](validation/batch-output-pattern-build-20261027.json).

## Extension follow-up

[Server-extension adjustment](batch-server-extension-20261027.md) now covers shared and per-row extensions supplied by metadata. MIME-only and other response-derived filename cases remain open.
