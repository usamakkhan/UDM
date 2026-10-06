# Building the native desktop

The three application executables are x64 C++17. The UI uses static Unicode MFC; HTTP uses WinHTTP, FTP uses WinINet, secrets use DPAPI, and IPC uses a current-user named pipe. FFmpeg/FFprobe are optional local media tools. The JavaScript browser extension and C kernel source keep their appropriate languages.

## Supported build path

Run `native/test-progress-dialog.ps1 -BuildRoot <absolute build directory> -RunRoot <fresh absolute output directory>` to compare actual status, Speed Limiter and completion controls with retained IDM resources at 96, 144 and 192 DPI. It also checks detail toggling, saved limiter/completion settings and checkbox accessibility. This uses isolated fixture windows and data; simulated DPI changes do not establish physical mixed-monitor behavior, and completion power actions are not executed.

Command-line downloads accept `/d URL`, `/p folder`, `/f filename`, and `/n` (silent). Use `/a` (or `--paused`) to add the download to Main queue without starting it. It retains queue membership and can be started later from the queue. These options work on first launch and when forwarding to an open UDM instance.

Run `UDM.exe /s` (or `--start-queue`) separately to start Main queue through the scheduler. This works on first launch and when UDM is already running, using the same queue limits and completion settings as the GUI's Start queue action. Queue-start cannot be combined with adding a URL or recovery mode.

Install Visual Studio 2022 or Build Tools with Desktop development with C++, the matching MFC/ATL components and a Windows 10/11 SDK. Open an x64 developer PowerShell, then run `./build.ps1 -UseInstalledToolchain` from the project root. This compiles with `/std:c++17 /MT /O2 /W4`, builds the resources and links the application, host, monitor and tests. The build script resolves paths independently of the caller's working directory.

On this PC the script can also use `.media-cache/driver-toolchain`: MSVC 14.44.35207, MFC/ATL and Windows SDK 10.0.26100.0. Microsoft payload hashes and compiler signatures were checked during setup. `setup-toolchain.ps1` supplements an existing cache with MFC/ATL; it is not a complete Visual Studio installer. Compiler caches are excluded from releases. The cached optional vctip telemetry helper was disabled after repeated crashes; cl/link remain unchanged.

`native/build.ps1` writes to `release-native`. The root `build.ps1` then copies the three application binaries and original assets to `release`. Close the target executable first. PDB files stay in development output; portable packaging excludes them.

## Completion dialog validation

Run `native/test-completion-dialog.ps1 -BuildRoot <absolute build directory> -RunRoot <fresh absolute output directory>` for isolated completion-dialog layout, icon-only drag area, and suppression persistence checks. Its default reference JSON is the versioned `docs/reference/completion-dialog.json`; use `-ReferencePath` to override it. No reference executable is needed or included.

The missing-file modal-warning diagnostic is opt-in with `UDM_TEST_COMPLETION_MISSING=1` because it stalled on this host. A normal passing run excludes that diagnostic and does not establish real shell actions, OLE drag/drop, or physical mixed-monitor behavior.

## Validation

For the consolidated backend checks, run this from the repository root in PowerShell 7:

```powershell
.\native\test-backend.ps1 -OutputRoot "$env:TEMP\UDM-backend-build"
```

Use `-UseInstalledToolchain` in a Visual Studio developer shell, or pass `-ToolchainRoot` for the cached toolchain. The runner builds and executes the native suite and the focused queue, synchronization, recovery, backup, and restore checks listed in `backend-tests.json`. Full media validation requires `release/tools/ffmpeg.exe`, `ffprobe.exe`, and their license. Each run uses a unique data directory and writes logs and `backend-results/<run-id>/summary.json` beneath the output root; failures return an error.

Use `-FocusedOnly` to skip the full native suite, `-FocusedOnly -Case IdleCheckpointTests` for the idle catalog regression, or `-FocusedOnly -Case SyncProbeTests` for synchronization response and cancellation checks. Test data stays separate from the installed application's catalog. See [backend consolidation](../docs/backend-completion-20261002.md) for validation and remaining scope.

`-FocusedOnly -Case ServerRetryTests` exercises real HTTP `Retry-After` timing for downloads and synchronization, including catalog restart, cancellation, and waits requiring an explicit retry.

`-FocusedOnly -Case PauseTransactionTests` locks the fixture catalog to check failed Pause rollback, continued HTTP transfer, successful retry, completed-file no-op behavior, and queue completion. Run `TerminalActionTests` alongside it for active scanner-wait cancellation.

`release-native/Udm.NativeTests.exe` creates its own temporary history and local HTTP server. DPAPI and local sockets require the user's normal process context. Place FFmpeg and FFprobe in `release-native/tools` for the media checks. These tests cover actual HTTP transfers, pause/resume, verification and publication, original stream protocol parsing, local muxing, persistence and named-pipe framing.

JavaScript checks live in `tests/*.test.cjs`. `tests/native-host.cjs` accepts `UDM_HOST_EXE` to test a staged executable and `UDM_INSTANCE_TAG` to use an isolated desktop instance. `ui-fixture.cjs --handoff` serves a bounded local file for a five-minute MFC dialog test; it targets the `native-validation` instance and does not execute the downloaded file.

For isolated UI validation:

```powershell
.\release-native\UDM.exe --instance-tag native-validation --data-dir .\native\validation-state
$env:UDM_INSTANCE_TAG='native-validation'
$env:UDM_HOST_EXE=(Resolve-Path .\release-native\Udm.NativeHost.exe).Path
node .\tests\native-host.cjs
```

Do not share the private validation state; it may contain copies of user download URLs. Use an empty data directory for distributable test fixtures.

## Source and dependencies

- `Core.cpp`: models, compatibility, persistence, queue scheduling and transfer management.
- `Transfer.cpp`: HTTP/FTP, workers, integrity checks, media assembly, grabber.
- `Streaming.cpp`: original SABR/protobuf/UMP transport.
- `Bridge.cpp`: native messaging and local pipe.
- `Network.cpp`: endpoint table and original WFP client.
- `App.cpp`, `Ui.hpp`, `WorkflowsUi.hpp`: MFC UI and workflows.
- `Tests.cpp`, `TransferChecks.hpp`, `TransferFixture.hpp`, `MediaChecks.hpp`, `BridgeChecks.hpp`: native verification, including persistent HTTP and dynamic-range resume fixtures.
- `third_party/json.hpp`: nlohmann JSON 3.12.0, MIT license in `LICENSE-json.txt`.

Microsoft MFC/CRT components remain subject to Microsoft's development and redistribution terms. UDM has no activation or trial-expiry code. The source-and-portable package contains no Microsoft compiler, private certificate key or IDM code/assets.

## Main-window menu regression

Build and run the isolated menu/command-state fixture from the repository root:

```powershell
.\native\test-menus.ps1 -BuildRoot D:\UDM\menu-test-build -RunRoot D:\UDM\menu-test-run1
```

Use a new `RunRoot` each time. The fixture creates its own catalog and local transfer data; it does not use the personal download catalog. The runner waits for the GUI process and requires a successful `results.json`. `-SkipBuild` explicitly reuses the fixture in the selected build root; use it only when that executable represents the source you intend to test. `-UseInstalledToolchain` and `-ToolchainRoot` are forwarded to the native build.

To compile only, use `native/build.ps1 -MenuStateTestsOnly -OutputRoot <directory>`. This cannot be combined with another build mode. The checks cover retained reference menu positions, dynamic queue actions, selection enablement, context routing, limiter state, and isolated transfer actions. They do not establish complete visual parity or real monitor/DPI behavior.
## Find dialog regression

Run `native/test-find.ps1 -BuildRoot <absolute-build-directory> -RunRoot <new-absolute-run-directory>` to build and exercise the actual Find dialog with an isolated three-record catalog. Use `-SkipBuild` only to reuse an explicitly chosen fixture executable. The checks cover default and retained search options, Cancel, next-match/wrap navigation, category and list filtering, and control bounds at simulated 96/144/192 DPI. Rendered dialog images are written beside results.json. These checks do not measure physical monitor transitions or prove live IDM pixel parity. Compile-only mode: `native/build.ps1 -FindDialogUiTestsOnly -OutputRoot <directory>`.
Properties dialog regression: run native/test-properties.ps1 -BuildRoot <absolute-build-folder> -RunRoot <fresh-absolute-run-folder>. Use -SkipBuild only to reuse an explicitly built fixture. The runner validates results.json; it uses an isolated catalog and download folder.

Deployment readiness: native/check-deployment.ps1 returns a read-only Ready/Checks object for the backed-up deployment plan. Any false check prevents deployment. Rerun immediately before replacement; the checker does not hold locks or mutate installed files.
