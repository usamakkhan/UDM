# Stable browser import choices

The review previously used a global selection mode. Changing one checkbox stopped metadata-driven file selection for every row, but changing authorization or Referer did not protect that row's current selection. These differed from the native import's per-row explicit-choice handling.

Each browser row now records whether the user has made an explicit choice. A checkbox change or property edit protects that row's current selection from late metadata. Other untouched rows can still be classified as their metadata arrives. Check all visible marks visible rows explicit; Clear all marks every row explicit. Select file links intentionally resets those markers and reapplies automatic classification. Preset selections from the in-page picker remain explicit from the start.

Submission now takes the actual displayed DOM order. Previously it recomputed a sort from possibly newer metadata at click time, which could submit files in a different order from the visible list while lookups were running. Hidden checked rows remain included in their retained list positions.

Both successful and failed lookup completions now stop updating the review after submission begins. A late failed HEAD request can no longer replace the batch progress status with a lookup count.

Chromium and Firefox version 0.63.55 share the change. JavaScript syntax, mirror and manifest checks are recorded; runtime, keyboard and asynchronous interaction tests remain deferred at the user's request. No newly measured IDM interaction claim is made.
