param([string]$Version = '0.31.0')
$ErrorActionPreference = 'Stop'
if ($Version -notmatch '^\d+\.\d+(\.\d+)?$') { throw 'Use a numeric release version.' }
$root = Split-Path $PSScriptRoot -Parent
$compiler = @(
    (Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6\ISCC.exe'),
    (Join-Path $env:LOCALAPPDATA 'Programs\Inno Setup 6\ISCC.exe')
) | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if (-not $compiler) { throw 'Install Inno Setup 6 before building the setup executable.' }
$driver = Join-Path $root 'drivers\out\x64-test-signed'
foreach ($file in @('UdmWfp.sys','UdmWfp.inf','udmwfp.cat','UDM-development-public.cer','signing-evidence.json')) {
    if (-not (Test-Path -LiteralPath (Join-Path $driver $file))) { throw 'Generate the development-signed driver package before building setup.' }
}
$signing = Get-Content (Join-Path $driver 'signing-evidence.json') -Raw | ConvertFrom-Json
if (-not $signing.cmsSignaturesVerified -or $signing.productionSigned) { throw 'Expected a verified development-signed driver package.' }
& $compiler ('/DAppVersion=' + $Version) ('/DDriverThumbprint=' + $signing.certificateThumbprint) (Join-Path $PSScriptRoot 'UDM-Setup.iss')
if ($LASTEXITCODE -ne 0) { throw 'Inno Setup compilation failed.' }
Get-ChildItem (Join-Path $root 'installer-out') -Filter ('UDM-' + $Version + '-Setup-x64.exe') | Select-Object -ExpandProperty FullName
