# Reproducible IDM static research

These scripts inspect local reference material and save reproducible static evidence. Target binaries remain unexecuted; the COM API is used only to read type-library metadata. The findings report is [here](../../docs/idm-complete-static-analysis-2026-09-27.md).

Raw files, extracted packages, decoded configuration, and decompiled pseudocode belong in the ignored `benchmarks/` directory. Tool dependencies belong in ignored `.media-cache/`. The scripts were developed for this particular specimen and Windows environment; do not treat them as hardened parsers for arbitrary untrusted collections.

## Environment used

- Python: `D:\IDM\python.exe` (3.12)
- Python modules: `.media-cache\reverse-tools\python-libs`, pefile 2024.8.26 and Capstone 5.0.9
- Ghidra: `.media-cache\reverse-tools\ghidra_12.1.3_PUBLIC`
- Java: `D:\Program Files\Java\jdk-25\bin\java.exe`
- 7-Zip: `D:\Program Files\7-Zip\7z.exe` (26.03)
- Sigcheck: `.media-cache\reverse-tools\sigcheck\sigcheck64.exe`
- DUMPBIN: path configured in `metadata.ps1`

All commands below are PowerShell. Adjust explicit paths for another machine. Never use the original installed files as an output directory. Ghidra batches sharing a project must run sequentially; separate projects can run concurrently. A summary checkpoint skips previously exported programs; remove only that generated checkpoint when deliberately re-exporting a program. Existing output directories should not be reused for different source sets without reviewing stale outputs.

## Inventory and installer extraction

```powershell
$researchPython = 'D:\IDM\python.exe'
$researchLibs = 'D:\UDM\.media-cache\reverse-tools\python-libs'
$researchEvidence = 'D:\UDM\benchmarks\idm-complete-20260927'
$researchSetup = 'D:\UDM\benchmarks\idm-setup-build11'

& $researchPython D:\UDM\tools\idm-research\unpack-setup.py `
  'C:\Users\Abuzar\Downloads\Programs\idman643build11.exe' `
  --output $researchSetup --python-libs $researchLibs

& $researchPython D:\UDM\tools\idm-research\inventory.py `
  --root 'D:\UDM\IDM FIles' `
  --root 'C:\Program Files (x86)\Internet Download Manager' `
  --root 'C:\Users\Abuzar\Downloads\Programs\idman643build11.exe' `
  --root "$researchSetup\payload" `
  --root 'C:\Users\Abuzar\AppData\Roaming\IDM\idmmzcc5' `
  --root 'C:\Windows\System32\drivers\idmwfp.sys' `
  --output $researchEvidence --python-libs $researchLibs `
  --sevenzip 'D:\Program Files\7-Zip\7z.exe'
```

Expected specimen inventory: 988 occurrences, 465 unique contents, 59 PEs (36 x86, 17 x64, six ARM64), 13 containers, and 1,321 PE resources. Counts intentionally include duplicates by path separately from unique content. Installed-state files are recorded by `footprint.py` separately from this corpus.

`unpack-setup.py` verifies footer bounds, record lengths, sequential manifest entries, path safety, complete zlib streams/checksums, and the trailer. Record zero uses an output capacity rather than an exact uncompressed length. It never invokes the executable.

## Metadata and decoded formats

```powershell
& D:\UDM\tools\idm-research\metadata.ps1 -Evidence $researchEvidence
& $researchPython D:\UDM\tools\idm-research\enrich.py $researchEvidence
& $researchPython D:\UDM\tools\idm-research\decode-filetypes.py `
  'D:\UDM\IDM FIles\IDMFType.dat' --output "$researchEvidence\filetype-database"
& $researchPython D:\UDM\tools\idm-research\decode-capture-config.py `
  'D:\UDM\IDM FIles\idmfc.dat' --output "$researchEvidence\capture-config"
& $researchPython D:\UDM\tools\idm-research\trace-browser-rules.py $researchEvidence
& $researchPython D:\UDM\tools\idm-research\decode-xpt.py $researchEvidence
& $researchPython D:\UDM\tools\idm-research\decode-languages.py $researchEvidence
& $researchPython D:\UDM\tools\idm-research\auxiliary.py $researchEvidence
& $researchPython D:\UDM\tools\idm-research\compare-installations.py $researchEvidence --setup $researchSetup

$researchTypeResults = @()
New-Item -ItemType Directory -Force -Path "$researchEvidence\type-libraries" | Out-Null
foreach ($researchType in (Get-Content -LiteralPath "$researchEvidence\type-library-inputs.json" -Raw | ConvertFrom-Json)) {
  & D:\UDM\tools\idm-research\typelib.ps1 -Path $researchType.path `
    -Output (Join-Path "$researchEvidence\type-libraries" ($researchType.sha256+'.json')) | Out-Null
  $researchTypeResults += [pscustomobject]@{sha256=$researchType.sha256;owners=$researchType.owners;status='parsed'}
}
$researchTypeResults | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath "$researchEvidence\type-libraries\status.json" -Encoding utf8

& 'D:\UDM\.media-cache\reverse-tools\sigcheck\sigcheck64.exe' `
  -accepteula -nobanner -a -h -s -c "$researchEvidence\native-inputs" `
  > "$researchEvidence\sigcheck.csv"
```

PowerShell's native redirection encoding can differ by host version; detect the BOM when reading Sigcheck CSV. The observed session produced UTF-16 LE. `-i` and `-c` cannot be combined in the tested Sigcheck build.

The COM parser uses `LoadTypeLibEx` with `REGKIND_NONE`; embedded type-library blobs are supplied as standalone files. No COM class is instantiated. XPT offsets require the historical Mozilla directory-offset correction; all seven observed libraries parse with every byte accounted for.

`decode-capture-config.py` independently implements the observed fixed table/stream transform. It validates the recognizable header and a complete round trip. It also inventories rule prefixes and records selected static consumer evidence. `decode-filetypes.py` validates all six blocks, string/blob tables, mapping indices, record widths, and round trips. Semantic gaps remain explicit in their output.

`trace-browser-rules.py` supplements the configuration evidence with source excerpts, byte offsets, and the mechanical effects of all seven active flags in the hash-pinned browser evaluator. It does not execute the JavaScript. Its field-level descriptions take precedence over the earlier decoder's incomplete consumer notes; broader native/driver meanings remain unverified.

## Native-code export and audit

```powershell
& D:\UDM\tools\idm-research\run-ghidra.ps1 -Evidence $researchEvidence
& D:\UDM\tools\idm-research\audit-ghidra.ps1 -Evidence $researchEvidence -ProjectName IDM-all
& D:\UDM\tools\idm-research\disassemble.ps1 -Evidence $researchEvidence
& $researchPython D:\UDM\tools\idm-research\verify-http2-change.py $researchEvidence --python-libs $researchLibs
& $researchPython D:\UDM\tools\idm-research\trace-driver-interface.py $researchEvidence
```

The main runner imports one file at a time and gives automatic analysis 600 seconds per file. `ExportAll.java` attempts every recognized nonexternal, non-thunk function, with 15 seconds per function and no count cap. Thunks are separately listed. Automatic-analysis timeout status, SHA-256, counts, and individual failures are recorded.

`AuditCoverage.java` exports every recognized instruction, measures function/instruction coverage per executable memory block, retries failed functions with 120 seconds each, and retains assembly for residual failures. Audit results are checkpoints too. Success of a Java process alone is not enough: the final validator requires actual per-binary export and audit artifacts.

`disassemble.ps1` runs Microsoft's DUMPBIN `/DISASM` over every unique PE snapshot and records exit codes, output sizes, and hashes. This provides an independent section disassembly, including bytes beyond Ghidra's recognized functions. It does not establish runtime reachability or prove that every decoded instruction represents code. The finalizer checks the complete 59-input set, output hashes, and the presence of instruction listings.

`trace-driver-interface.py` preserves the pinned x64 WFP driver's 11 device-control cases, observed minimum-input gates, device/security strings, four relevant function excerpts, and device-creation assembly. It never opens or invokes the driver. Its map is evidence for further protocol analysis, not a full message specification or an exploitability finding.

The observed investigation initially started before the installer was added, so it used `IDM-all`, `IDM-build11`, `IDM-missed`, and `IDM-setup` projects. The three `IDM-missed` inputs repair an early exporter API error. Reproduction from scratch can use one project for all 59 inputs. Do not run a second importer against a project that is already open for writing.

The shared crypto library was additionally processed in the separate `IDM-crypto` project. The installed 32-bit monitor reused the earlier `IDM-components` analysis via `reuse-analysis.ps1`, with an exact SHA-256 guard and a fresh full export/audit. All five new project directories and the two reused programs are reconciled by hash in the same final ledger.

`reuse-idman-analysis.ps1` is an optional shortcut specific to the existing September 20 `IDM-static.gpr` project. It checks the exact expected executable SHA-256, opens the earlier analysis read-only, and performs a new uncapped export and assembly audit. It reuses prior analysis, not the earlier limited decompilation output. If that project is unavailable, the normal importer handles the same specimen instead.

Pseudocode is not original source. Incorrect prototypes, missing indirect edges, exception handling, and erroneous no-return assumptions can produce misleading output even for a successful decompilation. Consult assembly before relying on such details. Executable bytes outside recognized functions are retained as a coverage measure, not automatically labeled undiscovered instructions.

## Local state and final verification

```powershell
& $researchPython D:\UDM\tools\idm-research\footprint.py `
  --root 'C:\Users\Abuzar\AppData\Roaming\IDM' `
  --root 'C:\ProgramData\IDM' `
  --output "$researchEvidence\state-footprint.private.json"

& $researchPython D:\UDM\tools\idm-research\finalize.py $researchEvidence `
  --setup $researchSetup --strict `
  --report D:\UDM\docs\idm-complete-static-analysis-2026-09-27.md `
  --native-report D:\UDM\docs\idm-native-coverage-2026-09-27.md
```

The state pass hashes files and records registry schemas without copying downloads, history values, or credentials. It records files changing during reads. Its counts are separate from the static program corpus.

The finalizer verifies all snapshots and original source-file hashes, rechecks every setup record, rejects a corrupted zlib stream, reconciles per-function counts, and writes `file-coverage-final.csv`, `native-coverage.csv`, and `coverage-summary.json`. `--strict` fails if an export/audit is missing, source bytes changed, or evidence accounting fails. Residual decompiler failures remain visible; they do not masquerade as successful pseudocode. Do not interpret an initial inventory status such as "format unresolved" as the final state: the final ledger incorporates supplemental decoder results.
