param([string]$Evidence='D:\UDM\benchmarks\idm-complete-20260927',[Parameter(Mandatory=$true)][string]$ProjectDirectory,[Parameter(Mandatory=$true)][string]$ProjectName,[Parameter(Mandatory=$true)][string]$ProgramName,[Parameter(Mandatory=$true)][string]$ExpectedSha256)
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$ghidra=Join-Path $repo '.media-cache\reverse-tools\ghidra_12.1.3_PUBLIC'
$java='D:\Program Files\Java\jdk-25\bin\java.exe'
$outputName=$ExpectedSha256.Substring(0,12)+'__'+$ProgramName
if(Test-Path -LiteralPath "$Evidence\ghidra\$outputName\summary.json"){throw 'Export already exists; do not overwrite concurrent analysis'}
& $java '-Xmx2G' '-XX:ParallelGCThreads=2' '-XX:CICompilerCount=2' '-Djava.system.class.loader=ghidra.GhidraClassLoader' '-Dfile.encoding=UTF8' "-Dapplication.settingsdir=$Evidence\settings-reuse-$ProjectName" "-Dapplication.cachedir=$Evidence\cache-reuse-$ProjectName" '-cp' "$ghidra\Ghidra\Framework\Utility\lib\Utility.jar" 'ghidra.Ghidra' 'ghidra.app.util.headless.AnalyzeHeadless' $ProjectDirectory $ProjectName -process $ProgramName -readOnly -noanalysis -max-cpu 2 -scriptPath $PSScriptRoot -postScript ExportAll.java "$Evidence\ghidra" $ExpectedSha256 -postScript AuditCoverage.java "$Evidence\ghidra" *> "$Evidence\logs\$outputName-reuse.log"
if($LASTEXITCODE -ne 0){throw 'Existing project export failed'}
if(!(Test-Path -LiteralPath "$Evidence\ghidra\$outputName\audit.json")){throw 'Required audit output missing'}
@{expectedSha256=$ExpectedSha256;project="$ProjectDirectory\$ProjectName.gpr";program=$ProgramName;analysisReused=$true;newFunctionExport='all recognized non-thunk functions';originalProjectReadOnly=$true} | ConvertTo-Json | Set-Content -LiteralPath "$Evidence\$outputName-reuse.json" -Encoding utf8
