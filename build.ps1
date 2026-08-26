param([switch]$Test,[switch]$UseInstalledToolchain)
$ErrorActionPreference='Stop'
& (Join-Path $PSScriptRoot 'native\build.ps1') -Test:$Test -UseInstalledToolchain:$UseInstalledToolchain
$udmRelease=Join-Path $PSScriptRoot 'release'
New-Item -ItemType Directory -Force -Path $udmRelease | Out-Null
foreach($udmName in @('UDM.exe','Udm.NativeHost.exe','Udm.Monitor.exe')){
 Copy-Item -LiteralPath (Join-Path $PSScriptRoot ('release-native\'+$udmName)) -Destination $udmRelease -Force
}
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'assets') -Destination $udmRelease -Recurse -Force
Write-Output ('Built native C++/MFC UDM: '+(Join-Path $udmRelease 'UDM.exe'))
