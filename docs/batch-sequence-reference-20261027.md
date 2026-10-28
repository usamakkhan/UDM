# Batch sequence behavior

## Evidence

The retained analysis of reference IDMan.exe SHA-256
03cc62e9adb77a380f9dc12f67ccaaee5106f12844aa73ce32c914ddd16d607c
contains the batch-generation path at 004885d0. Its letter and numeric loops
choose increment or decrement from the relationship of the endpoints. The
numeric formatter uses a zero-padded decimal field with the selected minimum
width. Both paths add the final endpoint separately after walking the range.

The same path associates each generated URL with its formatted sequence token.
The import constructor at 004c2c20 retrieves that token for batch mode through
004c13a0; the filename replacement path at 004bdb80 consumes it. The replacement
helper at 0043b380 replaces each occurrence, not only the first. This narrows the
remaining filename-pattern implementation: it must retain the original generated
token through selection/sorting rather than assign a new number from display order.
Automatic extension changes in 004c14b0 still require further review before
claiming full rename parity.

These are static-analysis findings, not newly observed live IDM behavior. No
proprietary code or assets are incorporated into UDM.

## UDM change

`batchPattern` now supports descending numeric and same-case letter ranges.
For example, `img*.jpg`, From 3, To 1, width 3 generates `img003.jpg`,
`img002.jpg`, `img001.jpg`. From c to a generates c, b, a. Equal endpoints still
produce one URL. Numeric batches still accept only 0 through 999999 and at most
1,000 entries; the count guard now checks distance in either direction.

Existing live previews and import selection use the same generator. The change
was followed by [explicit wildcard output renaming](batch-output-pattern-20261027.md).

Interactive and regression testing are deferred at the user's request. See the
[build record](validation/batch-descending-build-20261027.json) for build status.
