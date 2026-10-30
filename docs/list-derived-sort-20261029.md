# Derived download-list sort keys

Size, status/progress and remaining-time sorting now prepare derived keys once per visible row while the manager lock is held. Comparator calls no longer repeat size lookup, progress calculation or ETA calculation. Date sorting retains its existing once-per-row parse and now uses pointer-keyed hash lookup. The comparator takes shared download pointers by reference, avoiding reference-count changes for every comparison.

Keys are local to each refresh, so current progress, speed and metadata are reflected without persistent-cache invalidation. Stable ordering, ascending/descending behavior, unknown-value ordering, text/referer handling and selection/scroll restoration retain their existing paths. Row rendering still calculates displayed values as needed.

Source review establishes fewer derived-key calculations; no runtime speedup or IDM performance parity is claimed. Runtime/performance testing remains deferred at user request.

Extension 0.63.70 is unchanged. All five production targets and the unsigned installer built successfully. See the [build record](validation/list-derived-sort-build-20261029.json) for source and artifact hashes. No installation or publication is performed.
