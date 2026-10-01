param([string]$Version = '0.78.0',[string]$CompilerPath)
$ErrorActionPreference = 'Stop'
if ($Version -notmatch '^\d+\.\d+(\.\d+)?$') { throw 'Use a numeric release version.' }
$root = Split-Path $PSScriptRoot -Parent
$compiler = @(
    $CompilerPath,
    (Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6\ISCC.exe'),
    (Join-Path $env:LOCALAPPDATA 'Programs\Inno Setup 6\ISCC.exe')
) | Where-Object { $_ -and (Test-Path -LiteralPath $_) } | Select-Object -First 1
if (-not $compiler) { throw 'Install Inno Setup 6 before building the setup executable.' }
$application = Join-Path $root 'release\UDM.exe'
if (!(Test-Path -LiteralPath $application) -or (Get-Item -LiteralPath $application).VersionInfo.ProductVersion -ne $Version) { throw 'Rebuild UDM with the requested version before packaging it.' }
$network = Join-Path $root 'release\network'
foreach ($file in @('Udm.Network.exe','runtime\WinDivert.dll','runtime\WinDivert64.sys','runtime\WinDivert-LICENSE.txt','WinDivert-2.2.2-Source.zip')) {
    if (-not (Test-Path -LiteralPath (Join-Path $network $file))) { throw ('Missing signed network package: '+$file) }
}
$status = & (Join-Path $network 'Udm.Network.exe') --status | ConvertFrom-Json
if ($LASTEXITCODE -ne 0 -or -not $status.RuntimeVerified -or $status.DesktopBrokerProtocol -ne 1) { throw 'Signed network runtime or desktop protocol verification failed.' }
$browserVersion=(Get-Content -LiteralPath (Join-Path $root 'browser\chromium\manifest.json') -Raw | ConvertFrom-Json).version
if ($browserVersion -notmatch '^\d+\.\d+\.\d+$') { throw 'The browser manifest must declare a numeric release version.' }
$outputName='UDM-' + $Version + '-Browser-' + $browserVersion + '-Setup-x64'
$firefoxVersion=(Get-Content -LiteralPath (Join-Path $root 'browser\firefox\manifest.json') -Raw | ConvertFrom-Json).version
if ($firefoxVersion -ne $browserVersion) { throw 'Chromium and Firefox packages must use the same release version.' }
& $compiler ('/DAppVersion=' + $Version) ('/DBrowserVersion=' + $browserVersion) ('/F' + $outputName) (Join-Path $PSScriptRoot 'UDM-Setup.iss')
if ($LASTEXITCODE -ne 0) { throw 'Inno Setup compilation failed.' }
Get-Item -LiteralPath (Join-Path $root ('installer-out\' + $outputName + '.exe')) | Select-Object -ExpandProperty FullName
