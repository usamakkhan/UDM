# IDM native static coverage

Generated from the local hash-indexed evidence. All function counts come from Ghidra recognition; successful pseudocode is not a correctness guarantee. Full SHA-256 values, source paths, per-function errors, assembly, and counts of executable bytes outside recognized functions remain in the local evidence ledger.

[Investigation report](idm-complete-static-analysis-2026-09-27.md) · [Full CSV](../benchmarks/idm-complete-20260927/native-coverage.csv)

| Binary | SHA-256 prefix | Architecture | Eligible functions | Pseudocode recovered | Residual failures | Assembly audit |
| --- | --- | --- | ---: | ---: | ---: | --- |
| downlWithIDM.dll | `38ac192d707f` | x86 | 891 | 891 | 0 | complete |
| downlWithIDM64.dll | `8a51ece1c4c8` | x64 | 422 | 422 | 0 | complete |
| IDMan.exe | `03cc62e9adb7` | x86 | 18835 | 18834 | 1 | complete |
| IDMan.exe | `e8b0459d59a3` | x86 | 18841 | 18840 | 1 | complete |
| idman643build11.exe | `87254f35dd11` | x86 | 271 | 271 | 0 | complete |
| idmbrbtn.dll | `19d5f712ba9f` | x86 | 131 | 131 | 0 | complete |
| idmbrbtn64.dll | `89c56c16047a` | x64 | 116 | 115 | 1 | complete |
| idmbrbtnAA.dll | `b8e959cf07bf` | ARM64 | 174 | 173 | 1 | complete |
| idmBroker.exe | `97810e0b3838` | x86 | 631 | 631 | 0 | complete |
| idmcchandler2.dll | `1450146b9049` | x86 | 1999 | 1997 | 2 | complete |
| idmcchandler2_64.dll | `59ac02f5a064` | x64 | 1764 | 1763 | 1 | complete |
| idmcchandler7.dll | `e1a4dea06f18` | x86 | 2295 | 2294 | 1 | complete |
| idmcchandler7_64.dll | `d464e8e7c84c` | x64 | 3720 | 3719 | 1 | complete |
| idmfsa.dll | `801d3a802a64` | x86 | 193 | 193 | 0 | complete |
| idmftype.dll | `3a47dbb1f86f` | x86 | 116 | 116 | 0 | complete |
| IDMFType64.dll | `0479dda9f821` | x64 | 110 | 110 | 0 | complete |
| IDMGetAll.dll | `33a8a6b9413d` | x86 | 409 | 409 | 0 | complete |
| IDMGetAll64.dll | `117abaeb2745` | x64 | 303 | 303 | 0 | complete |
| IDMGrHlp.exe | `b3b6281ea820` | x86 | 2632 | 2632 | 0 | complete |
| IDMIECC.dll | `9a9989644213` | x86 | 2846 | 2846 | 0 | complete |
| IDMIECC64.dll | `d0c6d455d067` | x64 | 2374 | 2373 | 1 | complete |
| idmindex.dll | `1fdb0d5b31e0` | x86 | 1708 | 1708 | 0 | complete |
| IDMIntegrator64.exe | `caffd6fedeb8` | x64 | 28 | 28 | 0 | complete |
| IDMIntegratorAA.exe | `5f2695d34fdf` | ARM64 | 69 | 69 | 0 | complete |
| idmmkb.dll | `8a10c135de47` | x86 | 51 | 51 | 0 | complete |
| IDMMsgHost.exe | `dba25a49adb8` | x86 | 73 | 73 | 0 | complete |
| idmmzcc.dll | `4b924f07115a` | x86 | 55 | 55 | 0 | complete |
| idmmzcc.dll | `92b753d1e482` | x86 | 66 | 66 | 0 | complete |
| idmmzcc.dll | `c9b83ce41312` | x86 | 35 | 35 | 0 | complete |
| idmmzcc64.dll | `132591d6563f` | x64 | 76 | 76 | 0 | complete |
| idmmzcc64.dll | `721aa5f82a1b` | x64 | 93 | 93 | 0 | complete |
| idmmzcc7.dll | `1621fd14dd72` | x86 | 64 | 64 | 0 | complete |
| idmmzcc7_64.dll | `c44fd11a6973` | x64 | 350 | 350 | 0 | complete |
| IDMNetMon.dll | `51fa2219d9be` | x86 | 864 | 863 | 1 | complete |
| IDMNetMon.dll | `6829aea180f1` | x86 | 864 | 864 | 0 | complete |
| IDMNetMon64.dll | `9840d78e94f9` | x64 | 833 | 822 | 11 | complete |
| IDMNetMon64.dll | `c8093f7824eb` | x64 | 831 | 819 | 12 | complete |
| IDMNetMonAA.dll | `45fabeee1f7b` | ARM64 | 913 | 910 | 3 | complete |
| idmnmcl.dll | `6baf9aa997be` | x86 | 113 | 113 | 0 | complete |
| IDMShellExt.dll | `211930e13a12` | x86 | 60 | 60 | 0 | complete |
| IDMShellExt64.dll | `48e5c5916f10` | x64 | 52 | 52 | 0 | complete |
| IDMShellExtAA.dll | `e21340a8f15a` | ARM64 | 101 | 101 | 0 | complete |
| idmtdi32.sys | `3eb60c4d9ac3` | x86 | 370 | 370 | 0 | complete |
| idmtdi64.sys | `84382bf4c10b` | x64 | 328 | 328 | 0 | complete |
| idmvconv.dll | `96694c5184b8` | x86 | 4481 | 4481 | 0 | complete |
| idmvconv.dll | `9ce7213f6583` | x86 | 4600 | 4600 | 0 | complete |
| IDMVMPrs.dll | `f9fe31b640b7` | x86 | 957 | 957 | 0 | complete |
| IDMVMPrs64.dll | `6ff10a3025e0` | x64 | 944 | 943 | 1 | complete |
| IDMVMPrsAA.dll | `7405fdba1b44` | ARM64 | 984 | 984 | 0 | complete |
| idmvs.dll | `a2e28177b51a` | x86 | 36 | 36 | 0 | complete |
| idmwfp32.sys | `a9915728125b` | x86 | 434 | 434 | 0 | complete |
| idmwfp64.sys | `8acffb018114` | x64 | 356 | 356 | 0 | complete |
| idmwfpAA.sys | `fd646cfc32db` | ARM64 | 390 | 390 | 0 | complete |
| IEMonitor.exe | `c8fd8860e9a0` | x86 | 2200 | 2200 | 0 | complete |
| libcrypto.dll | `487a7532f22a` | x86 | 4860 | 4860 | 0 | complete |
| libssl.dll | `3e652321c101` | x86 | 1024 | 1024 | 0 | complete |
| MediumILStart.exe | `a4fda3af1c38` | x86 | 170 | 170 | 0 | complete |
| oldjsproxy.dll | `8fcca62c1d53` | x86 | 193 | 193 | 0 | complete |
| Uninstall.exe | `6f6201b354db` | x86 | 314 | 314 | 0 | complete |
