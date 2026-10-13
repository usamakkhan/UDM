# Same-file DASH sibling indexes

The Chromium and Firefox media parsers now expand multiple consecutive SIDX boxes returned in a single same-file child index range. Every box must have the same reference ID, exact rational presentation-time continuity, and the full child range must match its parent's duration. Media ranges must remain ordered and must not overlap any index range. Existing depth, request, byte, reference, and segment limits still apply.

Validation on 2026-10-13:

- `node tests/dash-hierarchy.test.cjs`: 41 checks passed, including two sibling boxes in one child range, differing timescales, identity/timing/offset rejects, and a media-only native handoff plan.
- All 57 `tests/*.test.cjs` files passed.
- The Chromium and Firefox `media.js` files are byte-identical.

This validates parsing and the browser-to-native plan in the test harness. An isolated live browser/native transfer was not run, and no installed app or release binary was changed.
