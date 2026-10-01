# Local production-engine benchmark

Build the app first (`native/build.ps1`), then run `build-bench.ps1` in this folder. The harness links the same production engine object files. It requires the native compiler environment/cache; it does not contain a separate downloader.

Start `node reference-server.cjs 50827` in a separate terminal. It listens only on loopback and stops after 30 minutes. Read the resulting server.json, create an input JSON containing its URL (for example http://127.0.0.1:50827/slow/new-test.bin) and SHA-256, and call `ProtocolBench.exe input.json new-test` with a fresh label for every run. Use the steady or slow URL path. Output includes transfer/assembly/verification/publication timing; the server records HTTP requests separately.

Generated results, state folders, executables and downloaded test files are disposable local artifacts. Preserve baseline engine objects/executable before changing production source if comparing versions. The 0.9 baseline binary is retained only in the development workspace, not redistributed in this source package.

For a valid comparison, run one file at a time, verify hashes, use the same payload/settings, alternate build order, and distinguish server timing from full application completion. This fixture is not an Internet-speed test. See docs/reverse-engineering-0.10.md for this release's results and limitations.

## Range-plan comparison

`node range-bench.cjs baseline.exe candidate.exe output-directory` runs three alternating pairs for request latency, steady throughput and one deliberately slow range. Both binaries must be built from `ProtocolBench.cpp` against their respective production engine objects. It uses identical 32 MiB bytes, eight workers, a per-run URL, expected SHA-256 and an independent hash of each final file. Results and request counts are written after every run. This is a local fixture comparison, not an Internet or IDM speed claim.
