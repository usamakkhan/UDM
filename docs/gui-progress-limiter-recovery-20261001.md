# Progress limiter control recovery

A blocked catalog write left Use Speed Limiter unchecked even though the download retained its saved 128 KB/s cap. The engine rolled back correctly; the visible controls did not.

The progress dialog now restores limiter controls from the effective job settings after persistence failures. It also distinguishes an active temporary override from a remembered limit when reopened. The existing resume path uses the same control synchronization routine.

17 focused actual-dialog checks passed, including failure reporting, checkbox and numeric-field restoration, unchanged saved catalog, a temporary 64 KB/s override over a saved 128 KB/s cap, reopening, failed remembering, successful retry and process restart. All 27 desktop regressions also passed. Fixtures selected the visible Speed Limiter tab using control-local messages; no global keyboard or mouse input was used.

The fixture initially required corrections to tab selection, instance-tag length and multiline caption matching. These failures are retained. The passing baseline reproduces a real UI/backend mismatch. These tests do not measure network throughput or qualify live transfer pause/resume, DPI or accessibility.

The paired installer contains the tested executable. Only UDM.exe changed among 132 payload inputs. Installed executables and both personal catalogs remain byte-identical. The candidate was not installed or published. Full IDM parity remains unestablished.

[Acceptance and exact artifact hashes](D:/UDM-Workspace/candidates/release-084-055/control/progress-limit-acceptance.json)
