# Reuse unchanged download-list ordering

Development version 1.22.0; browser source remains 0.63.85.

The main window previously rebuilt sort keys and ran stable_sort on every regular refresh, including idle lists and active transfers whose selected sort key had not changed. It now reuses the previous sorted order when the column, direction, exact filtered candidate sequence and relevant computed keys all match.

The cache retains the original catalog candidate sequence separately from its sorted output. Adding/removing a job, replacing a job object or changing the candidates' relative order forces a new stable sort. This preserves the existing equal-key tie order after queue/catalog moves; changing ascending/descending direction also sorts from the current catalog sequence rather than reversing old ties.

Key matching follows the existing comparator: numeric size/ETA/date values, status label plus progress for percentage-sorted downloads, transfer speed, or normalized text for name/queue/description/path/referer/page. Unrelated display changes do not force a new sort. The cache owns its saved keys and job references; it holds no views into temporary key maps. New keys are retained only after a sort miss, and ordinary text/status buffers are moved after the comparator finishes. The prior cache stays intact if the allocating copy needed to commit a new order fails.

Filtering/search and key derivation still run on current immutable row snapshots. Progress, rate, quota indicators, selection/focus, scroll anchoring and asynchronous file icons continue through the existing render path even when sorting is skipped. Disabling sorting clears cached order state. This change does not claim that all refresh work is eliminated or that current runtime speed matches IDM.

Build status: passed for all five production targets. Embedded versions and SHA-256 hashes are recorded; feature source hashes matched the pre-build snapshot. Large-catalog timing, live reordering and interaction tests, installer creation, installation and publication remain deferred. Installed UDM remains 1.6.0 by the last direct check. See [build receipt](validation/download-list-order-reuse-build-20261104.json).
