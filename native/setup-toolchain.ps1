$ErrorActionPreference='Stop'
$udmRoot=Split-Path $PSScriptRoot -Parent
$udmCache=Join-Path $udmRoot '.media-cache\driver-toolchain'
$udmPackages=@(
 @('Microsoft.VC.14.44.17.14.ATL.Headers.base','f8d375a254b6cfdbe0e7dc4fe7342d6212052068eb6394070dd932c815adb500'),
 @('Microsoft.VC.14.44.17.14.ATL.X64.base','6dd0b7f8079c654db2b8fdaebc74900e8418b5204914a77bde93a5b01ac0998c'),
 @('Microsoft.VC.14.44.17.14.MFC.Headers.base','9f2fd973b26765c378aa2990a0d7b8296b8cb2b5f8f3ac203dbb9539cff0b8ec'),
 @('Microsoft.VC.14.44.17.14.MFC.X64.base','40275c2d5ddcf5e258c122b976e73d5724e0e0f1a3cbb0f602dc701b20c6eb94'),
 @('Microsoft.VC.14.44.17.14.CRT.x64.Store.base','9135b03c0df53c7a0aa9bef7230a1c2ff4263a0ee7baa7e419d034f484f6bb56','67cf767c-5e71-47c2-a54a-cd5631e28942')
)
Add-Type -AssemblyName System.IO.Compression.FileSystem
New-Item -ItemType Directory -Force -Path $udmCache | Out-Null
foreach($udmPackage in $udmPackages){
 $udmArchive=Join-Path $udmCache ($udmPackage[0]+'.vsix')
 if(!(Test-Path -LiteralPath $udmArchive)){
  $udmDownloadId=if($udmPackage.Count -gt 2){$udmPackage[2]}else{'d2fa077f-a814-4fb2-b903-1fca7658d17e'}
  Invoke-WebRequest ('https://download.visualstudio.microsoft.com/download/pr/'+$udmDownloadId+'/'+$udmPackage[1]+'/'+$udmPackage[0]+'.vsix') -OutFile $udmArchive
 }
 if((Get-FileHash -LiteralPath $udmArchive -Algorithm SHA256).Hash -ne $udmPackage[1]){throw ('Package hash mismatch: '+$udmPackage[0])}
 $udmExtract=Join-Path $udmCache $udmPackage[0]
 if(!(Test-Path -LiteralPath $udmExtract)){[IO.Compression.ZipFile]::ExtractToDirectory($udmArchive,$udmExtract)}
 Write-Output ('Verified '+$udmPackage[0])
}
$udmThirdParty=Join-Path $PSScriptRoot 'third_party'
New-Item -ItemType Directory -Force -Path $udmThirdParty | Out-Null
$udmHeader=Join-Path $udmThirdParty 'json.hpp'
if(!(Test-Path -LiteralPath $udmHeader)){Invoke-WebRequest 'https://raw.githubusercontent.com/nlohmann/json/v3.12.0/single_include/nlohmann/json.hpp' -OutFile $udmHeader}
if((Get-FileHash -LiteralPath $udmHeader -Algorithm SHA256).Hash -ne 'aaf127c04cb31c406e5b04a63f1ae89369fccde6d8fa7cdda1ed4f32dfc5de63'){throw 'JSON header hash mismatch'}
Invoke-WebRequest 'https://raw.githubusercontent.com/nlohmann/json/v3.12.0/LICENSE.MIT' -OutFile (Join-Path $udmThirdParty 'LICENSE-json.txt')
Write-Output 'Verified JSON for Modern C++ 3.12.0 against the official release SHA-256.'
