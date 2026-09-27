# UDM signed network backend 0.3

UDM 0.36.0 includes desktop integration through an authenticated, short-lived elevated helper.
Use the desktop Network window's **Start signed monitor** / **Stop monitor** controls.
The GUI displays process-tree TCP/UDP connection metadata; unavailable per-flow byte counters are shown as --.

The upstream WinDivert 2.2.2-A DLL and driver are byte-for-byte unchanged and verified before loading.
No Test Mode, root-certificate installation, HTTPS decryption, or boot-policy changes are required by this package.
Administrative permission is requested only when starting a monitor or explicit gateway.

Udm.Network.exe --status verifies the runtime without loading the kernel driver.
Udm.Network.exe --watch-tree PID[,PID...] SECONDS observes selected process trees.
Udm.Network.exe --redirect-watch PID[,PID...] PORT[,PORT...] SECONDS runs the bounded TCP gateway.
The latter changes selected TCP routes only, never automatically starts downloads, and requires administrator rights.
TCP gateway connections can end when the helper stops; new direct connections recover when its filters close.

HTTP/1 metadata inspection covers fixed/chunked/close-delimited bodies, redirects and multipart byte ranges.
Bodies, cookies and authorization headers are not retained. HTTPS remains encrypted.
Legacy RTMP/RTMPT/RTSP protocol interpretation and broad VPN/HVCI compatibility remain unqualified.

Licenses are in runtime/WinDivert-LICENSE.txt; matching upstream source is WinDivert-2.2.2-Source.zip.
UDM implementation and build instructions are in the project's drivers/signed-network directory.


## Build
The top-level build.ps1 builds the helper and desktop together. It requires the cached MSVC/SDK toolchain and the verified WinDivert 2.2.2-A x64 runtime already under release/network/runtime, or supplied with -NetworkRuntimeDirectory.
The runtime directory must contain WinDivert.dll, WinDivert64.sys and WinDivert-LICENSE.txt. Binaries must match the pinned upstream hashes. Preserve the accompanying WinDivert-2.2.2-Source.zip.
For only the helper, run drivers/signed-network/build.ps1 with absolute -ToolchainRoot, -NativeRoot and -OutputRoot arguments.
Use package.ps1 -CompilerPath <ISCC.exe> to compile the installer. These build commands do not enable Test Mode or load the kernel driver.
