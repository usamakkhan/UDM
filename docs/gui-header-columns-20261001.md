# Column-header customization menu

UDM previously had no popup when a column header received a context-menu request. The verified baseline returned no menu. IDM documents a Columns command from this location in its [main-window guide](https://www.internetdownloadmanager.com/support/using_idm/using_idm.html).

The header now opens Columns and uses the existing customization dialog. Explicit header message forwarding preserves the originating control, including keyboard-position requests; the first implementation incorrectly opened the download menu for those requests. Header customization works without selecting a download and with an empty filtered list.

16 actual-app checks passed for menu dispatch, dialog opening, Cancel, selection preservation, width/order saving and persistence after restart, plus empty-list behavior and unchanged download records. All 27 existing desktop regressions also passed. Tests send messages only to privately launched application windows; they do not use global keyboard or mouse input.

The final paired installer contains the tested executable. Only UDM.exe changed among 132 payload inputs. Installed executables and both personal catalogs remain byte-identical. Previous builds and failed fixture results are retained. The candidate was not installed or published.

This establishes the documented header route, not complete live IDM, visual, accessibility or DPI parity. Elevated installer lifecycle, multi-user behavior, manifest concurrency and intermittent live-HLS timing remain open.

[Acceptance and exact hashes](D:/UDM-Workspace/candidates/release-084-055/control/header-menu-acceptance.json)
