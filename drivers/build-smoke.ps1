$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$compiler=Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
$out=Join-Path $PSScriptRoot 'out\tests'
New-Item -ItemType Directory -Path $out -Force | Out-Null
$refs=@('/r:System.dll','/r:System.Core.dll','/r:System.Net.Http.dll','/r:System.Runtime.Serialization.dll','/r:System.Security.dll','/r:System.Drawing.dll','/r:System.Windows.Forms.dll','/r:System.Web.dll','/r:System.Web.Extensions.dll')
$sources=@(Get-ChildItem (Join-Path $root 'src') -Filter '*.cs' | ForEach-Object FullName)
& $compiler /nologo /optimize+ /target:exe /main:Udm.DriverSmoke "/out:$out\Udm.DriverSmoke.exe" $refs $sources (Join-Path $PSScriptRoot 'LiveSmoke.cs')
if($LASTEXITCODE -ne 0){throw 'Driver smoke-test build failed.'}
Write-Output "Built $out\Udm.DriverSmoke.exe. No kernel tests ran."
