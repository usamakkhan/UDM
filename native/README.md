# Building the native desktop

The three application executables are x64 C++17. The UI uses static Unicode MFC; HTTP uses WinHTTP, FTP uses WinINet, secrets use DPAPI, and IPC uses a current-user named pipe. FFmpeg/FFprobe are optional local media tools. The JavaScript browser extension and C kernel source keep their appropriate languages.

## Supported build path

Install Visual Studio 2022 or Build Tools with Desktop development with C++, the matching MFC/ATL components and a Windows 10/11 SDK. Open an x64 developer PowerShell, then run `./build.ps1 -UseInstalledToolchain` from the project root. This compiles with `/std:c++17 /MT /O2 /W4`, builds the resources and links the application, host, monitor and tests. The build script resolves paths independently of the caller's working directory.

On this PC the script can also use `.media-cache/driver-toolchain`: MSVC 14.44.35207, MFC/ATL and Windows SDK 10.0.26100.0. Microsoft payload hashes and compiler signatures were checked during setup. `setup-toolchain.ps1` supplements an existing cache with MFC/ATL; it is not a complete Visual Studio installer. Compiler caches are excluded from releases. The cached optional vctip telemetry helper was disabled after repeated crashes; cl/link remain unchanged.

`native/build.ps1` writes to `release-native`. The root `build.ps1` then copies the three application binaries and original assets to `release`. Close the target executable first. PDB files stay in development output; portable packaging excludes them.

## Validation

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
