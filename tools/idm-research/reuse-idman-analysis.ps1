param([string]$Evidence='D:\UDM\benchmarks\idm-complete-20260927')
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$ghidra=Join-Path $repo '.media-cache\reverse-tools\ghidra_12.1.3_PUBLIC'
$java='D:\Program Files\Java\jdk-25\bin\java.exe'
$prior=Join-Path $repo 'benchmarks\reverse-2026-09-20\projects'
$expected='03cc62e9adb77a380f9dc12f67ccaaee5106f12844aa73ce32c914ddd16d607c'
& $java '-Xmx2G' '-XX:ParallelGCThreads=2' '-XX:CICompilerCount=2' '-Djava.system.class.loader=ghidra.GhidraClassLoader' '-Dfile.encoding=UTF8' "-Dapplication.settingsdir=$Evidence\settings-reuse" "-Dapplication.cachedir=$Evidence\cache-reuse" '-cp' "$ghidra\Ghidra\Framework\Utility\lib\Utility.jar" 'ghidra.Ghidra' 'ghidra.app.util.headless.AnalyzeHeadless' $prior IDM-static -process IDMan.exe -readOnly -noanalysis -max-cpu 2 -scriptPath $PSScriptRoot -postScript ExportAll.java "$Evidence\ghidra" $expected -postScript AuditCoverage.java "$Evidence\ghidra" *> "$Evidence\logs\IDMan-reused-analysis.log"
if($LASTEXITCODE -ne 0){throw 'Existing project export failed'}
if(!(Test-Path -LiteralPath "$Evidence\ghidra\03cc62e9adb7__IDMan.exe\audit.json")){throw 'Required audit output missing'}
@{expectedSha256=$expected;project=$prior+'\IDM-static.gpr';program='IDMan.exe';analysisReused=$true;newFunctionExport='all recognized non-thunk functions';originalProjectReadOnly=$true} | ConvertTo-Json | Set-Content -LiteralPath "$Evidence\reused-analysis.json" -Encoding utf8
