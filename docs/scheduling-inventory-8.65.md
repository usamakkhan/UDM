# Bounded missing-schema inventory, 8.65

The first scheduling capture failed qualification with 86,672 undecodable events and no usable context switch. Its old binary did not retain the failing global event headers, so attributing that count to a particular schema would be a guess. The earlier fixed-tool worker cannot silently adopt a changed collector.

## Read-only lookup evidence

Local Windows WMI defines context-switch schemas through version 4. A new normal-token `probe_cswitch_tdh.cpp` issued fifteen synthetic TDH metadata queries: versions 1–5 under classic, classic/32-bit and classic/64-bit headers. Versions 1–4 resolve successfully in all twelve controls; all three version-5 queries return 1168 (not found). No logger or administrator process is involved. These synthetic results do **not** identify the missing live events or validate a decoder.

[Lookup results](validation/scheduling-inventory-8.65/synthetic-tdh-results.json), [local WMI definitions](validation/scheduling-inventory-8.65/local-cswitch-schemas.json), [probe build identity](validation/scheduling-inventory-8.65/synthetic-tdh-build.json).

Microsoft's [PerfView context-switch parser](https://github.com/microsoft/perfview/blob/main/src/TraceEvent/Parsers/KernelTraceEventParser.cs) reads the thread IDs and wait fields from the payload and checks a minimum length for versions above 2. That provides a primary-source lead for subsequent **offline** analysis, not permission to assume the version, provider or payload layout of events we have not retained.

## Updated collector

`TraceSchemaInventory.hpp` records the provider, event ID, opcode, version, header flags, payload length, fresh TDH lookup status and count for missing schemas. It caps the inventory at 64 distinct keys. It also retains undecoded bytes only for Thread-provider/opcode-36 events in an observed target lifetime, with limits of 16,384 records and 128 bytes per payload. Other unknown event payloads are not retained. Reaching a cap sets the collector's limit flag; no incomplete inventory is silently accepted.

The new profile is `disk-scheduling-no-fileio-v2-inventory`. It retains the scheduling-only provider flags, deadlines, owned-root/controller restrictions, logger cleanup and original rejection semantics. It does not implement a guessed decoder, register schemas, change WMI, enable FileIo or establish a flush cause. Raw context-switch bytes remain private; public reports retain header/count inventories and a private-trace hash, with kernel IRP addresses replaced by opaque tokens.

Twelve native controls verify window/provider/opcode filtering, exact byte retention, repeated-key counting, record/schema/payload caps and missing-payload rejection. The updated collector builds and refuses an unelevated argumentless invocation. [Build and control identities](validation/scheduling-inventory-8.65/build-and-controls.json). The product remains 8.65 / extension 0.72.29; no production source, installer or installed binary is changed.

## Live inventory acquired

The old worker stopped normally after its one completed request. The updated fixed-tool worker started successfully and serviced one 8 KiB normal-token fixture capture. It retains its own eight-hour, twenty-request cap; it accepts only the newly pinned collector, not arbitrary commands. No additional prompt was required for the fixture launch itself.

The inventory contains exactly one missing global schema: Thread GUID `3d6fa8d1-fe05-11d0-9dda-00c04fd7ba7c`, event ID 0, opcode 36, **version 5**, header flags 848, **28-byte payload**. Fresh TDH lookup returns 1168. Its count accounts for all 89,763 unknown events. The collector retained all 6,883 qualifying raw records during the observed fixture lifetime; neither inventory cap was reached. This now identifies an actual missing schema rather than inferring it from synthetic lookup failures.

Inventory transport/structure checks pass: one matching fresh normal-token process, exact native fixture output, successful collector exit and logger absence, zero reported event/buffer losses, exact unknown-header count accounting, in-lifetime raw timestamps and payload lengths matching their schema keys. This is **not** a scheduling qualification pass or a decoder/cause claim. The original strict scheduling validator still rejects the new inventory profile and its undecoded events, as expected.

[Live outcome and complete header inventory](validation/scheduling-inventory-8.65/live-inventory.json), [redacted trace](validation/scheduling-inventory-8.65/fixture-82e541b9-trace.json), [native calls](validation/scheduling-inventory-8.65/fixture-82e541b9-native.json), [inventory checks](validation/scheduling-inventory-8.65/fixture-82e541b9-inventory-validation.json), [unchanged strict rejection](validation/scheduling-inventory-8.65/fixture-82e541b9-validation.json), [cleanup](validation/scheduling-inventory-8.65/fixture-82e541b9-cleanup.json). All 8,192 payload bytes, the generated executable copy and the marked fixture work root are removed. Raw records and a copy of the native call report remain private for offline analysis.

The finished metadata-probe and inventory-control executable/object pairs are also [removed](validation/scheduling-inventory-8.65/finished-controls-cleanup.json) after hash verification. Their source and receipts remain. The active fixed collector and normal fixture are preserved for the approved diagnostic session; product build directories are untouched.

## Next action

The subsequent [selected-prefix analysis](cswitch-prefix-analysis-8.65.md) passes 48 independent known-schema TDH field checks and 44 policy checks, then correlates the retained native fixture calls with observed off-CPU intervals. It explicitly preserves the failed full-schema qualification and version-5 inference limit. Actual slow-App attribution remains pending; a target-thread collection filter is needed to respect the current record cap on longer captures.

Analyse the actual version-5/28-byte records against authoritative prefix definitions and independent native/TDH controls offline, without another download or administrator prompt. Do not interpret its extra bytes by guesswork, call inventory acquisition full scheduling qualification or erase either previous failed capture. A larger App trace must wait for a clearly defined, verified observation model; full completion/background-work and IDM parity requirements remain open.
