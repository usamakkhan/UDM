# Main-window keyboard navigation

UDM's category tree, download list and quick filter previously did not move focus when Tab or Shift+Tab was routed through the frame. Four direct focus transitions failed against the unchanged app implementation. The frame now passes plain Tab/Shift+Tab from its child controls through Windows dialog navigation. Ctrl/Alt combinations remain outside that new route.

Ten actual MFC checks pass: forward/reverse traversal, toolbar participation, visible filter navigation, skipping hidden or disabled controls, the sole-visible-control case and Ctrl+Tab. Navigation creates no downloads. The optimized app also passes 27 existing desktop menu/queue/restart checks. The fix is packaged in the paired 0.84/0.55 installer; only UDM.exe changed among 132 payload inputs. Installed executables and both personal catalogs remain byte-identical.

The retained IDM menu extraction matches the current executable hash and confirms the already implemented Ctrl+F and F3 captions. Accelerator resource enumeration returned Windows error 1813; no extra shortcut map was inferred. This is a tested standard keyboard-navigation correction, not proof of complete IDM keyboard/focus-order parity.

The fixture initially omitted the toolbar assets and exited before reporting. Its corrected asset setup produced the baseline. Initial wrap expectations also omitted the toolbar; the final fixture includes it and explicitly hides it when testing one remaining tab stop. Missing-assets startup robustness remains a separate investigation.

Elevated installer lifecycle, multi-user behavior, manifest concurrency, intermittent live-HLS timing and full IDM parity remain open. The candidate has not been installed or published.

[Acceptance and exact hashes](D:/UDM-Workspace/candidates/release-084-055/control/keyboard-navigation-acceptance.json) · [Windows control-navigation API](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-isdialogmessagew)
