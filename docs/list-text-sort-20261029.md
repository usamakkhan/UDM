# Download-list text sort preparation

The main list previously rebuilt text keys inside its stable-sort comparator on every refresh. Filename and description sorting repeatedly allocated/lowercased strings; path sorting also repeatedly constructed filesystem paths and converted them to UTF-8. Queue and recovery-page sorting did similar repeated work while the manager lock was held.

The refresh now prepares a key once per visible row for those text columns. Comparisons use local pointer-keyed hash-map lookups and string comparison. Keys are rebuilt each refresh, so renames, queue changes, path edits and recovery-page edits do not require cache invalidation. Existing stable sorting, direction, filtering, equal-key order, scroll/selection restoration and numeric/date/referer paths remain unchanged.

This removes repeated conversion from comparator calls at the cost of one local key per row. Source inspection establishes the reduction in key-generation calls, not a measured responsiveness improvement or IDM speed parity. Runtime/performance tests remain deferred at user request.

Extension 0.63.70 is unchanged. All five production targets and the unsigned installer built successfully. See the [build record](validation/list-text-sort-build-20261029.json) for source and artifact hashes. No installation or publication is performed.
