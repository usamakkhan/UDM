# Category icons refresh after editing

The main list previously updated category icons only when its set or order of downloads changed. Editing a file from Video to Archives or Music retained its previous icon until the list was rebuilt, for example by changing filters.

The existing-row refresh now compares the displayed image with the saved category and updates it in place. It does not recreate the row or change selection or keyboard focus.

The actual MFC control fixture reproduced three failed assertions before the fix. All nine checks now pass: initial rendering, repeat edits, saved-category validation failures, selection/focus, unaffected rows and filtered rebuilding. These checks call the same Manager configuration operation used by Properties; they do not claim to click through Properties itself. The rebuilt app also passes 27 existing desktop workflow checks.

The paired 0.84/0.55 installer was rebuilt. Only UDM.exe changed among 132 payload inputs; the previous app and installer are retained. Personal catalogs and installed executables remain byte-identical. No installation or publication occurred.

This is a verified presentation defect fix, not proof of complete IDM visual or functional parity. Elevated installation, multi-user behavior, broad public-video support, intermittent live-HLS timing and matched-route speed comparisons remain open.

[Exact evidence and hashes](D:/UDM-Workspace/candidates/release-084-055/control/category-row-acceptance.json)
