# Deferred hidden history columns, 2026-10-13

The download list formerly formatted and stored Added and Save Path text for every row even when those columns had zero width. The existing Referer and Parent Web Page columns already skipped their hidden values. The same rule now applies to Added and Save Path; a width change invalidates the row cache so showing either column fills its current value.

An isolated MFC menu/state fixture passed 380 checks, including new assertions that hidden column text is empty and that revealing the columns fills the saved date and path. The updated native app compiled with `native/build.ps1 -AppOnly`, and all seven isolated `tests/launch.native-live.cjs` checks passed.

With 5,000 synthetic completed downloads, default column widths, isolated data directories, and startup tips disabled, a Win32 probe observed all list rows after 1,277, 1,164, and 1,155 ms in three updated launches. A single previous-build observation was 1,395 ms. These local runs suggest reduced first-list work but are too few to claim a stable speedup or an IDM comparison. They do not test a user's real history or first paint.
