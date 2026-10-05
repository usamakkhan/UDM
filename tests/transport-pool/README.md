# HTTP connection-pool acceptance

From an x64 Visual Studio developer shell, build the actual native backend and fixture probe:

```powershell
./native/build.ps1 -TransportPoolTestsOnly -UseInstalledToolchain -OutputRoot D:/UDM-TestBuild
python ./tests/transport-pool/run.py --probe D:/UDM-TestBuild/release-native/Udm.TransportPoolProbe.exe --output D:/UDM-TestResults/pool-001
```

Alternatively pass `-ToolchainRoot` for the existing cached MSVC/SDK layout. Choose a new output directory for every run; results are never overwritten. Python 3.11+ and network access are required. The executable uses isolated request objects and does not instantiate the personal download manager or read its catalog.

The runner starts loopback HTTP and SOCKS fixtures and allows only their local origin and enumerated public HTTPS fixture addresses. It counts actual connections and checks exact bytes, range/HEAD reuse, independent sessions, separate POST connections, one-use POST submission, cookie/authentication isolation, truncated/closed responses, 16 concurrent cancellations, overlapping HTTP/2 streams, a slow reader and TLS-failure isolation. HTTPS verification remains enabled; no certificates are installed. Public endpoint availability can make a run fail. Timing assertions distinguish overlapping delayed streams and bounded cancellation; they are not download-speed benchmarks.

`checks.json` retains assertions, per-request status/protocol/timing, probe hash and fixture observations. A zero process exit and all 18 checks passing are required. The fixture uses synthetic credentials only. Per-scenario output files retain exact response bytes for independent verification.
