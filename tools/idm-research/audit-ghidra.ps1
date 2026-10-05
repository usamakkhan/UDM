param([string]$Evidence='D:\UDM\benchmarks\idm-complete-20260927',[Parameter(Mandatory=$true)][string]$ProjectName)
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$ghidra=Join-Path $repo '.media-cache\reverse-tools\ghidra_12.1.3_PUBLIC'
$java='D:\Program Files\Java\jdk-25\bin\java.exe'
$log=Join-Path "$Evidence\logs" ($ProjectName+'-audit.log')
& $java '-Xmx2G' '-XX:ParallelGCThreads=2' '-XX:CICompilerCount=2' '-Djava.system.class.loader=ghidra.GhidraClassLoader' '-Dfile.encoding=UTF8' "-Dapplication.settingsdir=$Evidence\settings-$ProjectName" "-Dapplication.cachedir=$Evidence\cache-$ProjectName" '-cp' "$ghidra\Ghidra\Framework\Utility\lib\Utility.jar" 'ghidra.Ghidra' 'ghidra.app.util.headless.AnalyzeHeadless' "$Evidence\projects" $ProjectName -process -readOnly -noanalysis -max-cpu 2 -scriptPath $PSScriptRoot -postScript AuditCoverage.java "$Evidence\ghidra" *> $log
if($LASTEXITCODE -ne 0){throw "Audit failed: $log"}
Write-Output "Audit process finished: $log"
