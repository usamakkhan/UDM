param([switch]$Test)
$ErrorActionPreference = 'Stop'
$udmRoot = $PSScriptRoot
$compiler = Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
if (-not (Test-Path -LiteralPath $compiler)) { $compiler = Join-Path $env:WINDIR 'Microsoft.NET\Framework\v4.0.30319\csc.exe' }
if (-not (Test-Path -LiteralPath $compiler)) { throw '.NET Framework 4.x compiler was not found.' }
$output = Join-Path $udmRoot 'release-legacy-built'
New-Item -ItemType Directory -Force -Path $output | Out-Null
& $compiler /nologo /optimize+ /target:exe /r:System.Drawing.dll "/out:$output\IconBuilder.exe" (Join-Path $udmRoot 'src\Icons.cs') (Join-Path $udmRoot 'tests\IconBuilder.cs')
if ($LASTEXITCODE -ne 0) { throw 'Icon builder failed.' }
& "$output\IconBuilder.exe" (Join-Path $udmRoot 'assets')
$refs = @('/r:System.dll','/r:System.Core.dll','/r:System.Net.Http.dll','/r:System.Runtime.Serialization.dll','/r:System.Security.dll','/r:System.Drawing.dll','/r:System.Windows.Forms.dll','/r:System.Web.dll','/r:System.Web.Extensions.dll')
$sources = @(Get-ChildItem -LiteralPath (Join-Path $udmRoot 'src') -Filter '*.cs' | ForEach-Object { $_.FullName })
& $compiler /nologo /optimize+ /target:winexe /platform:anycpu /main:Udm.Program "/win32icon:$udmRoot\assets\udm.ico" "/out:$output\UDM.exe" $refs $sources
if ($LASTEXITCODE -ne 0) { throw 'UDM build failed.' }
& $compiler /nologo /optimize+ /target:exe /platform:anycpu /main:Udm.NativeHost "/out:$output\Udm.NativeHost.exe" $refs $sources
if ($LASTEXITCODE -ne 0) { throw 'Native host build failed.' }
& $compiler /nologo /optimize+ /target:exe /platform:anycpu /main:Udm.MonitorProgram "/out:$output\Udm.Monitor.exe" $refs $sources
if ($LASTEXITCODE -ne 0) { throw 'Network monitor build failed.' }
if ($Test) {
    $testSources = @(Get-ChildItem -LiteralPath (Join-Path $udmRoot 'tests') -Filter '*.cs' | ForEach-Object { $_.FullName })
    & $compiler /nologo /debug /target:exe /main:Udm.Tests "/out:$output\Udm.Tests.exe" $refs $sources $testSources
    if ($LASTEXITCODE -ne 0) { throw 'Test build failed.' }
    & "$output\Udm.Tests.exe"
    if ($LASTEXITCODE -ne 0) { throw 'Tests failed.' }
}
Write-Output "Built $output\UDM.exe"
