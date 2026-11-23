# UDM 9.81: check the original response during prefix removal

UDM 9.81 removes a duplicate full-body disk read in the publication path for an approved text-prefix removal when the server supplies a checksum. Both extension source manifests advance to **0.73.45**; browser JavaScript is unchanged. All five native programs build as 9.81. This candidate is **uninstalled**. The earlier whole-app finishing-speed losses and overall IDM parity remain open.

## Cause and implementation

The frozen 9.80 worker copied the retained tail into staging and hashed that final output. It then opened and read the entire original response again to check `ServerSha256`. Those digests describe different byte domains, so both checks are necessary; the additional read is not.

[Transfer.cpp](../native/Transfer.cpp) lets the existing tail-copy loop calculate the raw server digest as well. When that digest is required, the loop reads from byte zero, hashes every received byte, omits the approved prefix from writes, and separately hashes exactly the bytes written. The source handle disallows writes and deletion throughout that copy. The server digest is checked before publication; a user-specified digest still checks the stripped output. Cancellation checks, staging ownership, output flush, recognizable-content validation and durable publication/recovery barriers remain.

The ordinary non-prefix path and numbered-part assembly retain their existing behavior. Receipt verification still reads saved bytes before trusting a reopened response. This removes one publication-phase read; it does not make the entire saved-response workflow a single read or remove either required digest calculation. The successful path records `ServerHashMode: prefix-copy` in the existing finishing diagnostics.

The [isolated six-file delta](validation/prefix-checksum-9.81/prefix-copy.patch) and [frozen source/core hashes](validation/prefix-checksum-9.81/baseline/inputs.json) distinguish this improvement from the inherited working tree.

## Real HTTP and recovery checks

All **1,559 assertions in 23 backend suites** pass. The maintained [native form fixture](../native/HttpPostIdentityTests.cpp) and [owned HTTP runner](../tests/http-post-identity.native-live.cjs) add four prefix cases:

- A matching raw server digest and final-output digest publish the exact stripped bytes.
- An incorrect server digest blocks publication and preserves the original response.
- A server digest calculated over only the stripped output is rejected as the wrong byte domain.
- A matching raw digest with an incorrect user output digest also blocks publication.

Each rejection is retried from retained, verified bytes. The independent origin sees exactly one original POST per case and no retry submission.

| Qualification | Assertions | Requests |
| --- | ---: | ---: |
| Frozen 9.80, multi-chunk payload on D: | 28 | 4 |
| 9.81, small payload on D: | 28 | 4 |
| 9.81, multi-chunk payload on D: | 28 | 4 |
| 9.81, multi-chunk payload on C: | 28 | 4 |
| Existing plain-form workflow | 53 | 14 |
| Existing gzip-form workflow | 49 | 12 |
| Existing 21 server-digest cases | 88 | 25 |

The current-build workflow count is **274**, separate from the **28 baseline control assertions** and backend count. The baseline uses frozen production objects/resource with the same added fixture, changing only its expectation that the new diagnostic mode is absent. It remains a passing semantic control; this is a redundant-read optimization, not another checksum bypass reproduction.

The small response has 455 raw bytes and a **395-byte** final file. The multi-chunk response has 4,560,075 raw bytes and a **4,560,015-byte** final file. All three current successful prefix outputs match the independently generated hash and the exact size announced before Start. The large case crosses multiple 1 MiB copy buffers. The old/current large-file output hashes also match each other. Existing controls retain empty/unknown-length responses, prefix decisions, Download Later, catalog reopening, explicit-only resubmission, collisions and journal recovery.

The maintained mode is `UDM_POST_DIGEST_CASE=prefix-all`; `UDM_POST_PREFIX_LARGE=1` selects the multi-chunk fixture. `UDM_POST_IDENTITY_TEST_EXE` selects the built native actor. These tests exercise native transfers through a real owned origin; they are not installed-browser acceptance or whole-app speed measurements.

## Build, evidence and cleanup

[Build and suite records](validation/prefix-checksum-9.81/build/build-result.json), [all workflows](validation/prefix-checksum-9.81/workflow/results.json), [terminal record](validation/prefix-checksum-9.81/workflow/terminal.json), [final audit](validation/prefix-checksum-9.81/final-audit.json) and [export hashes](validation/prefix-checksum-9.81/export-manifest.json) are tracked. Reproduction drivers use `[repo]`, `[local-diagnostics]` and `[user-profile]` in place of private paths.

Transfer and Bridge are recompiled; forty core objects are retained. All **699 captured inputs**, **42 referenced core objects**, five program hashes and resource are verified after qualification and cleanup. Every program's Windows version is 9.81. All actor processes and owned origins terminate normally.

After sanitized export, **1,059 individually path/type/hash-checked files**, totaling **384,977,894 logical bytes**, are removed. These include payloads, private fixture catalogs, supplementary source snapshots, test actors, compiler targets and six superseded 9.80 helper/object artifacts. The frozen 9.80 App is retained. Current products, resource and all referenced core objects remain. [Cleanup receipt](validation/prefix-checksum-9.81/cleanup-receipt.json), [intentional retirement](validation/prefix-checksum-9.81/retired-9.80-artifacts.json). Physical space reclaimed was not measured. Protected installed programs/manifests, personal catalogs and shared FFmpeg/FFprobe retain all eleven reference hashes.

## Remaining comparison work

The user reported closing IDM, but its existing session-2 `-Embedding` process remained. The other Windows session was left alone. A fresh desktop-control retry still failed with `failed to write kernel assets` / missing path. [Blocker record](validation/prefix-checksum-9.81/desktop-blocker.json). No process was forced closed and no further approval prompt was launched.

There is no new IDM timing result, installed UI acceptance, or proof of a millisecond saving in the three earlier finishing cases. Next, restore supported desktop control, obtain a genuinely clean IDM lifecycle, and compare normal products through actual completion, process exit and final background flush. Retain losing results. No installer, installation, commit, push or release occurred in this increment.
