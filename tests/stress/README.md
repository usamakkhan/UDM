# Native engine stress tests

These fixtures link the production C++ engine; Node only hosts loopback servers, launches isolated clients, and independently calculates expected bytes/hashes. Use an x64 Visual Studio environment or the development compiler cache. PowerShell 7 is required for the optional TLS fixture generator.

```powershell
.\native\build.ps1 -Test
.\tests\stress\build-stress.ps1
node .\tests\stress\engine-stress.cjs
.\tests\stress\prepare-tls-fixture.ps1
node .\tests\stress\protocol-stress.cjs
```

The engine matrix covers 40 size/worker combinations, HTTP retries and truncation, changed resources, sequential fallbacks, invalid responses, queue concurrency, aggregate throttling, actual process termination/resume, repeated pause/reload, two batches of 120 sequential files, and a file larger than 4 GiB. Allow about 8 minutes and at least 12 GiB free disk space. Assembly temporarily requires both the part files and the destination staging file. Each run writes a new directory beneath `benchmarks/stress-2026-09-20`; previous evidence is retained.

Use `--quick` for the two stalled-server cancellation checks or `--handles-only` for the resource test. WinHTTP resources on the development PC can remain alive for roughly one minute after session closure. The resource test deliberately waits 65 seconds after each batch and compares both settled counts, with a separate cap relative to the process's initial count. Its five-minute process deadline includes transfer and cleanup time. It does not assert stability from a cold-start count or five-second cooldown.

The protocol matrix checks POST buffer lifetime, stalled POST cancellation, rejection of an untrusted TLS certificate, 16 simultaneous stalled downloads, missing/overlapping/oversized/corrupted partial data, invalid saved state, and destination errors. Corrupted bytes can only be detected against an independently supplied expected hash; a matching server validator alone does not establish local byte integrity.

`StressClient.cpp` accepts loopback hosts only for new jobs and never executes downloads. Its process termination test kills only its own fixture child. The TLS generator writes a disposable certificate and key as files; it does not change certificate trust. Do not package generated keys, state, output files, or binaries.

`HandleProbe.cpp` is an optional diagnostic, not part of production or the pass count. It inspects its own handle types using Windows native diagnostic APIs and compares HTTP session use, hashing and atomic writes. Build with `build-handle-probe.ps1`; pass a loopback one-byte range URL and an existing evidence directory. Those diagnostic API details may differ on another Windows version.

These tests are bounded development validation. They do not establish performance on the public Internet, all-day reliability, live streaming compatibility, power-loss recovery, disk-full recovery, or kernel-driver safety.