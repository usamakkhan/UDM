# Startup list refresh, 2026-10-13

The main window previously waited for its first 500 ms timer tick before filling the download list. Startup now posts an initial refresh after showing the window, so the list is filled when the UI message loop starts. The periodic timer remains responsible for later state updates.

Validation used isolated `--background --data-dir --instance-tag` processes with 100 synthetic completed downloads and startup tips disabled. A Win32 probe polled the UDM window's `SysListView32` item count until all 100 rows appeared. Three launches of the prior `0e61fe5` executable took 723, 709, and 708 ms. Three launches of the updated executable took 582, 272, and 275 ms. These are local wall-clock observations, including process creation, and the first updated launch was slower than subsequent warm launches.

`native/build.ps1 -AppOnly` succeeded. `tests/launch.native-live.cjs` passed all seven isolated desktop and loopback checks against the updated executable, including forwarded CLI commands, download completion, native messaging, and File Info behavior. This measurement establishes earlier list population for the fixture; it does not measure first paint, large catalogs, or parity with IDM on the same machine.
