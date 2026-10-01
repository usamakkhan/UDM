$ErrorActionPreference='Stop'
$udmBenchRoot=$PSScriptRoot
$udmRoot=(Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$udmBuild=[IO.File]::ReadAllText((Join-Path $udmRoot 'native\build.ps1'))
# Reuse the build's compiler environment selection without compiling/editing production code.
$udmSetup=$udmBuild.Substring($udmBuild.IndexOf('$udmCache='),$udmBuild.IndexOf('Push-Location')-$udmBuild.IndexOf('$udmCache='))
& ([scriptblock]::Create($udmSetup + @'

try {
 $udmObjects=@('Core','Transfer','Streaming','Adaptive','Bridge','Network')|ForEach-Object {Join-Path $udmRoot ('native\out\'+$_.ToString()+'.obj')}
 & $udmCompiler /nologo /std:c++17 /EHsc /MT /O2 /W4 /utf-8 /permissive- /DUNICODE /D_UNICODE /DNOMINMAX /D_WIN32_WINNT=0x0A00 /D_CRT_SECURE_NO_WARNINGS (Join-Path $udmBenchRoot 'ProtocolBench.cpp') @udmObjects ('/Fo'+(Join-Path $udmBenchRoot 'ProtocolBench.obj')) ('/Fe'+(Join-Path $udmBenchRoot 'ProtocolBench.exe')) /link winhttp.lib wininet.lib crypt32.lib bcrypt.lib shell32.lib shlwapi.lib ole32.lib oleaut32.lib advapi32.lib user32.lib gdi32.lib comctl32.lib ws2_32.lib iphlpapi.lib uuid.lib psapi.lib /SUBSYSTEM:CONSOLE /DYNAMICBASE /NXCOMPAT /HIGHENTROPYVA
 if($LASTEXITCODE){throw 'Benchmark harness build failed.'}
} finally {$env:INCLUDE=$udmSavedInclude;$env:LIB=$udmSavedLib}
'@))

