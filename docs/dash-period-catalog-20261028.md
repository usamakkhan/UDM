# Stored DASH catalog matching

Source review found that the new cross-Period matcher used a JavaScript Set of track objects. Audio and subtitle tracks appear in several quality choices. Once a catalog is materialized as separate object copies, object-reference deduplication does not merge those repeated entries and can incorrectly report an ambiguous track match.

The matcher now keys candidates by a canonical serialization of the complete track value. Object keys are sorted, array order remains significant and every track field remains included. Different IDs, resource URLs, byte ranges, clocks or descriptors remain distinct; only equal track values collapse. Genuine equally ranked alternatives still produce the ambiguity error. Candidates of unrelated media kinds are skipped before serialization.

Both browser families are 0.63.61. JavaScript syntax checks passed and media.js mirrors match byte for byte. The installer reuses the previously compiled native 1.0.4 payload; no native source changed in this increment. See `validation/dash-period-catalog-build-20261028.json` for packaging evidence.

Runtime, browser storage round-trip and download tests remain deferred at the user's request. The candidate is unsigned, uninstalled and unpublished. This correction does not prove the complete multi-Period workflow or finish live DASH support.
