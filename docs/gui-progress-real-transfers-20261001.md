# Progress controls during actual HTTP transfers

The current packaged candidate passed 29 checks across three real localhost transfers. Each downloaded file is 8 MiB; all three outputs independently match the known source bytes and SHA-256 hash.

- Start resets a temporary limiter override to the remembered rate. Pause leaves the progress window open and saves a resumable partial file. Saved bytes survive application restart, and resume issues range requests before producing the correct completed file.
- Cancel and the progress window close button stop active transfers, close the progress window and retain newly received data and resume metadata. Reopening and resuming after both interruptions still produces the exact file.
- Hide closes only the progress window. Received bytes continue increasing, and reopening shows the transfer still active. It completes with the correct bytes and hash.
- Completion changes the progress actions to Open and Close; closing that window retains the downloaded output.

The first two Hide test attempts did not activate the intended menu item. The passing test observes the actual highlighted command before activation and confirms that the original progress window disappears. The initial transfer log retains expected server-side connection resets after cancellation; later fixtures suppress only these expected reset/abort exceptions.

No production changes or installer rebuild were needed. The tested app matches the current installer payload, all 132 payload inputs remain unchanged, and installed executables and both personal catalogs remain byte-identical. The candidate has not been installed or published.

These are functional HTTP/1.1 tests with a stable localhost server and ETag, not an IDM speed comparison. Timings include UI waits, pauses and restart; they must not be presented as throughput measurements. Public-server, live-media, visual/DPI/accessibility, elevated lifecycle and full IDM parity remain open.

[Acceptance, output hashes and current installer](D:/UDM-Workspace/candidates/release-084-055/control/progress-real-transfer-acceptance.json)
