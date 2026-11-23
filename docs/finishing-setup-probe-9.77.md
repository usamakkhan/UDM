# UDM 9.77 finishing setup: volume traversal is measurable, async submission is small

The actual Manager download worker was qualified in three fresh console probe processes: sequential 4 MiB, two-connection-capped 32 MiB with eight requested connections, and healthy 32 MiB with eight requested connections. Every file completed and matched its independent hash and exact zone marker. The probe calls the real Resume/dispatch/worker/stop flow; it does not create the normal desktop window. Its process lifetime includes writing a fixture receipt and must not be ranked against normal App or IDM timing.

| Operation after the hash phase | Sequential 4 MiB | Capped 32 MiB | Healthy 32 MiB |
| --- | ---: | ---: | ---: |
| Post-hash validation/tag handling | 0.0033 ms | 0.0031 ms | 0.0029 ms |
| Stage output | 0.2611 ms | 0.2968 ms | 0.2421 ms |
| Finalization eligibility | 2.4665 ms | 2.5491 ms | 2.4842 ms |
| Volume-path query inside eligibility | 2.4584 ms | 2.5411 ms | 2.4754 ms |
| Drive-type query after path resolution | 0.0013 ms | 0.0012 ms | 0.0013 ms |
| Advisory setup | 0.0022 ms, no deferred task | 0.0201 ms | 0.0180 ms |
| Existing cache ownership transfer | Not used | 0.0009 ms | 0.0005 ms |
| `std::async` submission | Not used | 0.0140 ms | 0.0127 ms |
| Finish-phase snapshot critical section | 0.0071 ms | 0.0079 ms | 0.0128 ms |
| Zone-marker function | 1.5390 ms | 1.4394 ms | 1.4801 ms |

The volume-path walk accounts for nearly all of the final eligibility time in each observed flow. It also occurs earlier during reception setup: once in the sequential case and twice in each split case. These observations do not reproduce or explain the earlier 157.828 ms broad metadata interval, and do not establish a slow asynchronous task launch. Do not replace its task policy on that speculation.

A subsequent read-only check against an existing file found an opened-object volume query substantially cheaper than walking the file path. That motivated the guarded [9.78 fixed-volume change](fixed-volume-9.78.md). It queries the actual opened object without caching a drive-letter result and retains the old policy for unsupported results. The larger final body/intent/catalog waits and IDM parity remain separate work.

## Evidence and correction

[All three records and lossless trace rings](validation/finishing-setup-probe-9.77/runs.json), [exact private instrumentation delta](validation/finishing-setup-probe-9.77/Transfer.cpp.patch), [actual worker actor](validation/finishing-setup-probe-9.77/Actor.cpp), [frozen inputs and fixture correction](validation/finishing-setup-probe-9.77/inputs.json), [driver](validation/finishing-setup-probe-9.77/run-finishing-setup-probe-9.77.py), [terminal result](validation/finishing-setup-probe-9.77/terminal.json), [export hashes](validation/finishing-setup-probe-9.77/export-manifest.json).

The initial actor omitted its direct `<fstream>` include. Compilation failed before any actor launch. Its initial source/log are retained; the corrected actor was captured before compilation, linked against the already built diagnostic Transfer and 41 frozen normal core objects, and used for all three completed probes. Production inputs were unchanged during compilation and probe execution. The four authorized native/version/manifest changes for 9.78 happened only after all probe actors were terminal.

All 68 MiB of generated output payloads were individually hash-checked and removed; trace rings were exported losslessly and removed with zero dropped events. Eleven disposable source/object/executable artifacts were path/type/hash checked and deleted, totaling 43,677,244 logical bytes, including multiple hard links to the diagnostic executable. Physical space reclaimed was not measured. No owned probe process remains. Baseline normal programs, installed/catalog/helper files and compact evidence were preserved.
