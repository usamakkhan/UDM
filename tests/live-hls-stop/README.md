# Live HLS stop/save latency reproducer

Build native/build.ps1 -TestsOnly with the normal toolchain and media helpers. Run Udm.NativeTests.exe --live-hls-stop-checks.

The dedicated route runs eight repetitions of stalled playlist headers and stalled playlist bodies. Each trial creates synthetic video/audio locally and a private loopback HTTP fixture. It waits for a verified server stall, requests Stop and save, and records requestMs, mergeMs and elapsedMs. The existing elapsedMs < 1500 assertion is unchanged. mergeMs covers the application's merge/probe/hash/publication interval, not only encoding; requestMs includes persisting stop intent. The remaining elapsed time must not be described as pure network cancellation.

Each trial uses a separate output folder under test-output/<unique-id>/<trial>. The route avoids unrelated native test prerequisites. The ordinary --live-hls-checks route remains available and exercises the unchanged cases.

2026-10-01 acceptance: 16 ordinary and 16 bounded mixed-load cases passed; the existing live-HLS route passed 132 checks. This did not reproduce or fix the earlier intermittent timing failure, and does not qualify the separate browser audio-only timeout. The retained run-loaded.py is an investigation script tied to its recorded candidate layout, not a portable test launcher.
