param([string]$Evidence='D:\UDM\benchmarks\idm-complete-20260927',[string]$OnlyName='*',[string]$ProjectName='IDM-all',[string]$InputDirectory='')
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$ghidra=Join-Path $repo '.media-cache\reverse-tools\ghidra_12.1.3_PUBLIC'
$java='D:\Program Files\Java\jdk-25\bin\java.exe'
$env:JAVA_HOME='D:\Program Files\Java\jdk-25'
$env:GHIDRA_HEADLESS_MAXMEM='2G'
$env:GHIDRA_HEADLESS_JAVA_OPTIONS="-Dapplication.settingsdir=$Evidence\settings -Dapplication.cachedir=$Evidence\cache"
New-Item -ItemType Directory -Force -Path "$Evidence\projects","$Evidence\ghidra","$Evidence\logs" | Out-Null
if(!$InputDirectory){$InputDirectory="$Evidence\native-inputs"}
$files=Get-ChildItem -LiteralPath $InputDirectory -File | Where-Object Name -Like $OnlyName | Sort-Object Length,Name
foreach($file in $files){
    $summary=Join-Path "$Evidence\ghidra" ($file.Name+'\summary.json')
    if(Test-Path -LiteralPath $summary){continue}
    $log=Join-Path "$Evidence\logs" ($file.Name+'.log')
    Write-Output ('Analyzing '+$file.Name)
    & $java '-Xmx2G' '-XX:ParallelGCThreads=2' '-XX:CICompilerCount=2' '-Djava.system.class.loader=ghidra.GhidraClassLoader' '-Dfile.encoding=UTF8' '-Duser.country=US' '-Duser.language=en' "-Dapplication.settingsdir=$Evidence\settings-$ProjectName" "-Dapplication.cachedir=$Evidence\cache-$ProjectName" '-cp' "$ghidra\Ghidra\Framework\Utility\lib\Utility.jar" 'ghidra.Ghidra' 'ghidra.app.util.headless.AnalyzeHeadless' "$Evidence\projects" $ProjectName -import $file.FullName -overwrite -max-cpu 2 -analysisTimeoutPerFile 600 -scriptPath $PSScriptRoot -postScript ExportAll.java "$Evidence\ghidra" *> $log
    $code=$LASTEXITCODE
    if(Test-Path -LiteralPath $summary){Get-Content -LiteralPath $summary -Raw | Write-Output}else{throw ('NO SUMMARY exit='+$code+' log='+$log)}
}
