# Research-only evaluator. This does not build or install the desktop application.
param(
 [Parameter(Mandatory=$true)][string]$QuickJsRoot,
 [Parameter(Mandatory=$true)][string]$OutputRoot,
 [string]$ToolchainRoot,
 [switch]$UseInstalledToolchain
)
$ErrorActionPreference='Stop'
$udmQ=(Resolve-Path -LiteralPath $QuickJsRoot).Path
$udmRevision = & git -C $udmQ rev-parse HEAD
if($LASTEXITCODE -ne 0 -or $udmRevision -ne '6d46d07d04041b40f4f49eaa7fdebe44c314c699'){throw 'Requires QuickJS-NG v0.17.0 at the documented commit.'}
$udmDirty = & git -C $udmQ status --porcelain --untracked-files=no
if($LASTEXITCODE -ne 0 -or $udmDirty){throw 'Requires an unchanged QuickJS-NG checkout.'}
$udmQOut=[IO.Path]::GetFullPath($OutputRoot)
New-Item -ItemType Directory -Force -Path $udmQOut | Out-Null
$udmRoot=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$udmCache=if($ToolchainRoot){$ToolchainRoot}else{Join-Path $udmRoot '.media-cache\driver-toolchain'}
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
$udmSavedInclude=$env:INCLUDE
$udmSavedLib=$env:LIB
$udmCompiler=Join-Path $udmBin 'cl.exe'
$udmResourceCompiler=Join-Path $udmSdk 'bin\10.0.26100.0\x64\rc.exe'
if($UseInstalledToolchain -or !(Test-Path -LiteralPath $udmCompiler)){
 $udmCl=Get-Command cl.exe -ErrorAction SilentlyContinue
 $udmRc=Get-Command rc.exe -ErrorAction SilentlyContinue
 if(!$udmCl -or !$udmRc -or !$env:INCLUDE -or !$env:LIB){throw 'Open an x64 Native Tools PowerShell for Visual Studio 2022 with Desktop C++, MFC/ATL and a Windows SDK installed, then run build.ps1 again.'}
 $udmCompiler=$udmCl.Source
 $udmResourceCompiler=$udmRc.Source
 if($env:VSCMD_ARG_TGT_ARCH -and $env:VSCMD_ARG_TGT_ARCH -ne 'x64'){throw 'Select the x64 Visual Studio developer environment.'}
}else{
 foreach($udmPath in @($udmIncludes)+@($udmLibraries)){if(!(Test-Path -LiteralPath $udmPath)){throw ('Missing cached compiler dependency: '+$udmPath+'. Use -UseInstalledToolchain in a Visual Studio x64 developer environment.')}}
 $env:INCLUDE=$udmIncludes -join ';'
 $env:LIB=$udmLibraries -join ';'
}
try {
 $udmQObjects=@()
 foreach($udmName in @('dtoa','libregexp','libunicode','quickjs')){
  $udmObject=Join-Path $udmQOut ($udmName+'.obj')
  & $udmCompiler /nologo /TC /std:c11 /MT /O2 /W2 /D_GNU_SOURCE /DWIN32_LEAN_AND_MEAN /D_WIN32_WINNT=0x0A00 /D_CRT_SECURE_NO_WARNINGS /c (Join-Path $udmQ ($udmName+'.c')) ('/Fo'+$udmObject)
  if($LASTEXITCODE){throw ('Engine compile failed: '+$udmName)}
  $udmQObjects+=$udmObject
 }
 & $udmCompiler /nologo /std:c++17 /EHsc /MT /O2 /W4 /DUNICODE /D_UNICODE ('/I'+$udmQ) (Join-Path $PSScriptRoot 'pac-runtime-probe.cpp') @udmQObjects ('/Fo'+(Join-Path $udmQOut 'probe.obj')) ('/Fe'+(Join-Path $udmQOut 'probe.exe')) /link /STACK:8388608
 if($LASTEXITCODE){throw 'Runtime prototype link failed'}
} finally {$env:INCLUDE=$udmSavedInclude;$env:LIB=$udmSavedLib}
