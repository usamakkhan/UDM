param([switch]$Test,[switch]$UseInstalledToolchain,[string]$NetworkRuntimeDirectory,[string]$ToolchainRoot)
$ErrorActionPreference='Stop'
$udmToolchain=if($ToolchainRoot){$ToolchainRoot}else{Join-Path $PSScriptRoot '.media-cache\driver-toolchain'}
$udmNetwork=Join-Path $PSScriptRoot 'release\network'
$udmRuntime=Join-Path $udmNetwork 'runtime'
$udmExpected=@{'WinDivert.dll'='C1E060EE19444A259B2162F8AF0F3FE8C4428A1C6F694DCE20DE194AC8D7D9A2';'WinDivert64.sys'='8DA085332782708D8767BCACE5327A6EC7283C17CFB85E40B03CD2323A90DDC2'}
foreach($udmFile in $udmExpected.Keys){
 $udmSourceRuntime=if($NetworkRuntimeDirectory){$NetworkRuntimeDirectory}else{$udmRuntime}
 $udmPath=Join-Path $udmSourceRuntime $udmFile
 if(!(Test-Path -LiteralPath $udmPath) -or (Get-FileHash -LiteralPath $udmPath -Algorithm SHA256).Hash -ne $udmExpected[$udmFile]){throw ('Provide the unchanged WinDivert 2.2.2-A x64 runtime using -NetworkRuntimeDirectory. Missing or unverified file: '+$udmFile)}
}
if(!(Test-Path -LiteralPath (Join-Path $udmSourceRuntime 'WinDivert-LICENSE.txt'))){throw 'The WinDivert license must accompany the runtime.'}
if($NetworkRuntimeDirectory -and [IO.Path]::GetFullPath($NetworkRuntimeDirectory) -ne [IO.Path]::GetFullPath($udmRuntime)){
 New-Item -ItemType Directory -Force -Path $udmRuntime | Out-Null
 foreach($udmFile in @('WinDivert.dll','WinDivert64.sys','WinDivert-LICENSE.txt')){
  Copy-Item -LiteralPath (Join-Path $NetworkRuntimeDirectory $udmFile) -Destination (Join-Path $udmRuntime $udmFile) -Force
 }
}
& (Join-Path $PSScriptRoot 'drivers\signed-network\build.ps1') -ToolchainRoot $udmToolchain -NativeRoot (Join-Path $PSScriptRoot 'native') -OutputRoot $udmNetwork -UseInstalledToolchain:$UseInstalledToolchain
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'drivers\signed-network\WinDivert-2.2.2-Source.zip') -Destination $udmNetwork -Force
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'drivers\signed-network\README.md') -Destination $udmNetwork -Force
& (Join-Path $PSScriptRoot 'native\build.ps1') -Test:$Test -UseInstalledToolchain:$UseInstalledToolchain -ToolchainRoot $udmToolchain
$udmRelease=Join-Path $PSScriptRoot 'release'
foreach($udmName in @('UDM.exe','Udm.NativeHost.exe','Udm.Monitor.exe','Udm.SetupHelper.exe')){
 Copy-Item -LiteralPath (Join-Path $PSScriptRoot ('release-native\'+$udmName)) -Destination $udmRelease -Force
}
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'assets') -Destination $udmRelease -Recurse -Force
& (Join-Path $udmNetwork 'Udm.Network.exe') --status
if($LASTEXITCODE){throw 'Signed network runtime verification failed.'}
Write-Output ('Built native C++/MFC UDM: '+(Join-Path $udmRelease 'UDM.exe'))
