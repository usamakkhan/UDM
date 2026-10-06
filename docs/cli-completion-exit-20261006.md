# Command-line completion exit

UDM accepts `/q` (alias `--quit-after-download`) with `/d URL` to exit after that download completes successfully. The flag applies only when this invocation starts the application. Commands forwarded to an existing instance do not change its exit behavior. A bare `/q` does not schedule an exit.

Example:

```powershell
UDM.exe /d "https://example.com/file.zip" /n /q
```

The first instance tracks the admitted download in memory. It waits for `Complete`, an inactive worker, and a scanner result that permits completion, then uses the normal application exit command. Failed or paused transfers do not trigger exit; removing the record disarms the pending exit. Reusing an already-complete record does not count as a newly successful download. The exit intent is neither persisted nor included in browser/native-host requests. With `/a /q`, the file remains queued until explicitly started.

Reference: [IDM command-line documentation](https://support.internetdownloadmanager.com/support/command_line.html), checked 2026-10-06. This establishes the documented `/q` contract, not measured behavior of every IDM edge case. The subsequent [CLI hang-up candidate](cli-hangup-20261006.md) adds `/h` separately.

Validation passed: 10 real-app completion-exit checks, 16 focused backend checks, and 12 existing real-app queue checks. The candidate built successfully; it has not replaced the installed app. See [validation receipt](validation/cli-quit-20261006.json). Tests: `native/LaunchQueueTests.cpp` and `tests/cli-quit.native-live.cjs`. The live harness uses separate catalogs, instance tags, and a loopback HTTP server; it does not alter the personal catalog or installed executable.