# Independent external DASH index roots

The Chromium and Firefox media parsers now accept consecutive, independent SIDX roots in one recorded external index response. Each root must use the same reference ID, start exactly when the preceding root ends, and describe media bytes immediately following the preceding root's media. Timing is compared as an exact rational value, so different timescales can be used. The existing index depth, size, request, and segment limits still apply; a response may contain at most 64 top-level SIDX boxes.

Validation on 2026-10-13:

- `node tests/dash-external-tree.test.cjs`: 51 checks passed, including whole and ranged external indexes, different timescales, identity/timing discontinuities, byte gaps and overlaps, and the native media-only handoff plan.
- All 57 `tests/*.test.cjs` files passed. Two stale test assertions were updated to follow the current browser manifest version and the current 4 MiB form snapshot limit.
- The Chromium and Firefox `media.js` files are byte-identical.

This is parser and handoff-plan validation. The isolated browser/native transfer fixture was not run because Playwright is unavailable in this workspace. No installed app or release binary was changed by this work.
