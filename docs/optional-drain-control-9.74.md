# Split writeback: optional-drain causal control

Disabling optional split drainage makes both finishing and total time worse in this matched control. Keep the current policy. This is a private experiment on frozen **9.74 / browser 0.73.38**, not a new app improvement, version increment, installation or successful IDM comparison.

## Controlled difference

Two uninstrumented executables use the same freshly compiled App object, normal resource and 41 identical core objects. Only Transfer differs: the experimental copy skips optional prefinish work for split downloads. Sequential eligibility is unchanged. Both retain the mandatory final full body flush, publication intent, source identity checks, range-credit rules and canonical catalog acceptance. There is no timing threshold or adjusted pacing.

Three paired 64 MiB downloads alternate candidate/baseline order. Each owns a fresh catalog, destination and loopback origin. All six files match their independent size/hash oracle and saved Complete record. Actual process exit includes app-owned workers; a symmetric post-exit output flush is also measured. These endpoints do not prove that every system service or hardware cache is idle.

## Every endpoint loses

Median milliseconds; “tail” starts at the last origin body send, “total” starts at launch:

| Endpoint | Current drainage | No optional split drainage |
| --- | ---: | ---: |
| Exclusive file availability tail | 327.163 | 690.607 |
| Actual process exit tail | 373.287 | 740.088 |
| Total through process exit | 2,453.589 | 2,568.239 |
| Tail through post-exit output flush | 396.513 | 769.785 |
| Total through post-exit output flush | 2,475.451 | 2,600.276 |

The no-drain candidate loses all five endpoints in all three paired trials. Its final mandatory flush takes **650.055, 628.991 and 524.358 ms**, versus **216.196, 218.485 and 167.961 ms** with drainage. Baseline saves seven successful data-only operations per run; candidate saves zero optional operations. Its unchanged diagnostic schema labels the zero-operation mode “full”; that label does not mean an optional full flush occurred.

This rejects removing split drainage as a fix for the traced stalls. It supports retaining reception-time drainage for these conditions; it does not establish an optimal cadence, identify a disk/filter/scheduler cause or erase the earlier normal-product regression gate. The prior [operation trace](storage-finishing-timeline-9.74.md) remains separate from this uninstrumented comparison.

## IDM inspection limitations

The local REA launcher responds and lists tools. Its attempted binary-layout operation rejects the Windows PE target because its configured artifact provider supports Linux x64 only. Deep analysis providers are unavailable in this session. The original transport receipt's `passed` field means RPC completion, not successful PE analysis; [corrected operation status](validation/optional-drain-control-9.74/control/rea-status.json) records the distinction.

A separate read-only MSVC import inventory finds `FlushFileBuffers`, `WriteFile`, `CreateFileA/W`, `MoveFileA/W`, `MoveFileExW` and `GetProcAddress` in the installed IDM executable. Imports establish neither call order nor handle flags, durability semantics or actual runtime use. No target was attached to or executed by static analysis, and its hash is unchanged. No IDM implementation claim follows.

The requested fresh IDM process remains running after the reported window close. Desktop control fails to initialize, so a clean full-lifetime IDM comparison still requires its normal tray-menu Exit. The process in the other Windows session is left alone.

## Next attribution step

Keep drainage and the mandatory final flush intact. Attribute the remaining final-flush and metadata namespace waits with a qualified bounded IO recording; the current measurements cannot distinguish disk queueing, filesystem filters and scheduling. Separately complete the fresh IDM comparison once its current-session process exits. Do not promote the uninstalled 9.74 candidate on this control: its earlier full no-regression gate remains failed.

## Evidence and cleanup

[All six results](validation/optional-drain-control-9.74/comparison/results.json), [paired analysis](validation/optional-drain-control-9.74/control/analysis.json), [private source delta](validation/optional-drain-control-9.74/control/optional-drain-control.patch), [build identity](validation/optional-drain-control-9.74/control/build.json), [policy checks](validation/optional-drain-control-9.74/control/policy-verification.json), [final audit](validation/optional-drain-control-9.74/final-audit.json).

All six payloads, totaling **384 MiB**, were removed after verification. Four private executable/object files were individually checked and removed after sanitized export, another **112,690,189 logical bytes**. Combined logical length is **515,343,373 bytes**, about 491 MiB; physical space reclaimed is not measured. Current production build inputs, products, referenced core objects, installed files, personal catalogs and shared media helpers remain unchanged.
