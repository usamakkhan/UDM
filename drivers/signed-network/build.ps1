param([string]$ToolchainRoot,[string]$NativeRoot,[string]$OutputRoot,[switch]$UseInstalledToolchain)
$ErrorActionPreference='Stop'
if(!$ToolchainRoot -or !$NativeRoot -or !$OutputRoot){throw 'Supply ToolchainRoot, NativeRoot and OutputRoot.'}
$udmCache=$ToolchainRoot
$udmVc='Contents\VC\Tools\MSVC\14.44.35207'
$udmBin=Join-Path $udmCache ('Microsoft.VC.14.44.17.14.Tools.HostX64.TargetX64.base\'+$udmVc+'\bin\Hostx64\x64')
$udmSdk=Join-Path $udmCache 'microsoft.windows.sdk.cpp\c'
$udmSdkLib=Join-Path $udmCache 'microsoft.windows.sdk.cpp.x64\c'
$udmIncludes=@(
 (Join-Path $udmCache ('Microsoft.VC.14.44.17.14.CRT.Headers.base\'+$udmVc+'\include')),
 (Join-Path $udmCache ('Microsoft.VC.14.44.17.14.ATL.Headers.base\'+$udmVc+'\atlmfc\include')),
 (Join-Path $udmCache ('Microsoft.VC.14.44.17.14.MFC.Headers.base\'+$udmVc+'\atlmfc\include')),
 (Join-Path $udmSdk 'Include\10.0.26100.0\ucrt'),(Join-Path $udmSdk 'Include\10.0.26100.0\um'),(Join-Path $udmSdk 'Include\10.0.26100.0\shared')
)
$udmLibraries=@(
 (Join-Path $udmCache ('Microsoft.VC.14.44.17.14.CRT.x64.Desktop.base\'+$udmVc+'\lib\x64')),
 (Join-Path $udmCache ('Microsoft.VC.14.44.17.14.CRT.x64.Store.base\'+$udmVc+'\lib\x64')),
 (Join-Path $udmCache ('Microsoft.VC.14.44.17.14.ATL.X64.base\'+$udmVc+'\atlmfc\lib\x64')),
 (Join-Path $udmCache ('Microsoft.VC.14.44.17.14.MFC.X64.base\'+$udmVc+'\atlmfc\lib\x64')),
 (Join-Path $udmSdkLib 'ucrt\x64'),(Join-Path $udmSdkLib 'um\x64')
)

$udmCompiler=Join-Path $udmBin 'cl.exe'
$udmSavedInclude=$env:INCLUDE;$udmSavedLib=$env:LIB
try {
  if($UseInstalledToolchain -or !(Test-Path -LiteralPath $udmCompiler)){
  $udmCl=Get-Command cl.exe -ErrorAction SilentlyContinue
  if(!$udmCl -or !$env:INCLUDE -or !$env:LIB){throw 'Use a Visual Studio x64 developer environment or supply the cached toolchain.'}
  $udmCompiler=$udmCl.Source
 }else{$env:INCLUDE=$udmIncludes -join ';';$env:LIB=$udmLibraries -join ';'}
 $env:VCTIP_NOOPTIN='1';$env:VSCMD_SKIP_SENDTELEMETRY='1'
 New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null
 & $udmCompiler /nologo /std:c++17 /EHsc /MT /O2 /W4 /WX /utf-8 /permissive- /DUNICODE /D_UNICODE /DNOMINMAX /D_WIN32_WINNT=0x0A00 /DWINVER=0x0A00 /D_CRT_SECURE_NO_WARNINGS ('/I'+$NativeRoot) (Join-Path $PSScriptRoot 'NetworkMain.cpp') ('/Fo'+(Join-Path $OutputRoot 'NetworkMain.obj')) ('/Fe'+(Join-Path $OutputRoot 'Udm.Network.exe')) /link ws2_32.lib bcrypt.lib advapi32.lib iphlpapi.lib winhttp.lib /DYNAMICBASE /NXCOMPAT /HIGHENTROPYVA /INCREMENTAL:NO
 if($LASTEXITCODE){throw 'Signed-network helper build failed.'}
} finally {$env:INCLUDE=$udmSavedInclude;$env:LIB=$udmSavedLib}
