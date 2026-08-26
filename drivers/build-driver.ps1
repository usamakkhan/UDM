param([switch]$Analyze)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$cache=Join-Path $root '.media-cache\driver-toolchain'
$compiler=Get-ChildItem (Join-Path $cache 'Microsoft.VC.14.44.17.14.Tools.HostX64.TargetX64.base') -Filter cl.exe -Recurse | Select-Object -First 1
if(!$compiler){throw 'Microsoft C++ compiler cache missing. See drivers/README.md for prerequisites.'}
$bin=$compiler.DirectoryName
$linker=Join-Path $bin 'link.exe'
foreach($tool in @($compiler.FullName,$linker)){$signature=Get-AuthenticodeSignature $tool;if($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'Microsoft Corporation'){throw "Tool signature verification failed: $tool"}}
$crt=(Get-ChildItem (Join-Path $cache 'Microsoft.VC.14.44.17.14.CRT.Headers.base') -Filter vcruntime.h -Recurse | Select-Object -First 1).DirectoryName
$wdk=Join-Path $cache 'wdk\c'
$sdk=Join-Path $cache 'microsoft.windows.sdk.cpp\c'
$version='10.0.26100.0'
$out=Join-Path $PSScriptRoot 'out\x64'
New-Item -ItemType Directory -Path $out -Force | Out-Null
$resourceRoot=Join-Path $cache 'Microsoft.VC.14.44.17.14.Tools.HostX64.TargetX64.Res.base.enu'
$resources=Get-ChildItem $resourceRoot -Recurse -Directory -Filter 1033 | Select-Object -First 1
if($resources -and !(Test-Path (Join-Path $bin '1033'))){Copy-Item -LiteralPath $resources.FullName -Destination (Join-Path $bin '1033') -Recurse}
$flags=@('/nologo','/c','/TC','/kernel','/Zl','/O2','/GS','/guard:cf','/Qspectre','/W4','/external:W0','/WX','/Zi','/D_AMD64_','/DAMD64','/DNDIS_SUPPORT_NDIS6=1','/DWINVER=0x0A00','/D_WIN32_WINNT=0x0A00','/DNTDDI_VERSION=0x0A000000',"/external:I$wdk\Include\$version\km","/external:I$wdk\Include\$version\km\crt","/external:I$sdk\Include\$version\shared","/external:I$sdk\Include\$version\ucrt","/external:I$crt","/Fo$out\UdmMonitor.obj","/Fd$out\UdmMonitor-compile.pdb")
if($Analyze){$flags+=@('/analyze','/analyze:external-')}
& $compiler.FullName $flags (Join-Path $PSScriptRoot 'UdmMonitor.c')
if($LASTEXITCODE -ne 0){throw 'Driver compilation failed.'}
$libs=Join-Path $wdk "Lib\$version\km\x64"
& $linker /nologo /driver /subsystem:native /entry:GsDriverEntry /nodefaultlib /machine:x64 /guard:cf /dynamicbase /nxcompat /integritycheck /debug /opt:ref /opt:icf "/libpath:$libs" "/out:$out\UdmWfp.sys" "/pdb:$out\UdmWfp.pdb" "$out\UdmMonitor.obj" ntoskrnl.lib hal.lib fwpkclnt.lib wdmsec.lib BufferOverflowK.lib
if($LASTEXITCODE -ne 0){throw 'Driver link failed.'}
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'UdmWfp.inf') -Destination (Join-Path $out 'UdmWfp.inf')
& (Join-Path $wdk "tools\$version\x64\infverif.exe") /w (Join-Path $out 'UdmWfp.inf')
if($LASTEXITCODE -ne 0){throw 'INF verification failed.'}
$record=[ordered]@{builtUtc=[DateTime]::UtcNow.ToString('o');architecture='x64';compiler=$compiler.VersionInfo.FileVersion;kit='10.0.26100.6584';analyzed=[bool]$Analyze;infVerified=$true;driverSha256=(Get-FileHash "$out\UdmWfp.sys").Hash;sourceSha256=(Get-FileHash (Join-Path $PSScriptRoot 'UdmMonitor.c')).Hash;protocolSha256=(Get-FileHash (Join-Path $PSScriptRoot 'UdmWfpProtocol.h')).Hash;signature=(Get-AuthenticodeSignature "$out\UdmWfp.sys").Status.ToString();installedByBuild=$false;loadedByBuild=$false}
$record | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $out 'build-evidence.json')
Write-Output "Built unsigned driver: $out\UdmWfp.sys. This script does not install or load it."
