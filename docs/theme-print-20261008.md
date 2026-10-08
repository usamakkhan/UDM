# Consistent themed-control rendering — 8 October 2026

The previous appearance report described light tab surfaces in a dark progress-dialog image. Investigation showed ThemeTabs and ThemeHeader already paint dark surfaces for WM_PAINT, but did not handle WM_PRINTCLIENT. Our native image fixture uses WM_PRINT, so its captures overstated the live appearance gap. This is a correction to that interpretation, not evidence of a newly implemented dark-tab design.

Both controls now share their existing dark painter between normal painting and print-client rendering. Print rendering preserves the supplied device-context state. Light mode still uses the native control implementation. No control layout, caption, selection or navigation behavior changed.

A new pixel regression failed on the unchanged renderer, then passed with the correction. Final acceptance: 69 appearance checks and 174 progress-dialog checks passed. The latter compares retained IDM control metadata and exercises existing progress actions, tabs and preference persistence. The installed IDM executable hash matches the reference: 03CC62E9ADB77A380F9DC12F67CCAAEE5106F12844AA73CE32C914DDD16D607C. Both dark and light progress images were inspected; the corrected dark capture includes dark tabs, body and connection headers.

Evidence and build scripts: `C:\Users\Abuzar\AppData\Local\Temp\udm-theme-print-20261008`. Candidate and SHA-256: [receipt](validation/theme-print-20261008.json). App and GUI tests rebuilt; unchanged backend objects reused. Compiler warnings remain. Not packaged, installed or published. Personal UDM process 16436 remained on D:\UDM\release\UDM.exe.

These checks do not establish physical desktop appearance across Windows themes, full high-contrast accessibility, public-site download behavior or complete IDM parity.
