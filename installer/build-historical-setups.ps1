param([string[]]$Versions)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$compiler = @(
    (Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6\ISCC.exe'),
    (Join-Path $env:LOCALAPPDATA 'Programs\Inno Setup 6\ISCC.exe')
) | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if (-not $compiler) { throw 'Install Inno Setup 6 before building setup executables.' }
$driver = Join-Path $root 'drivers\out\x64-test-signed'
$signing = Get-Content (Join-Path $driver 'signing-evidence.json') -Raw | ConvertFrom-Json
if (-not $signing.cmsSignaturesVerified -or $signing.productionSigned) { throw 'Expected a verified development-signed driver package.' }
$credentialLines = "protocol=https`nhost=github.com`n`n" | git credential fill
$token = ($credentialLines | Where-Object { $_ -like 'password=*' } | ForEach-Object { $_.Substring(9) })
if (-not $token) { throw 'No GitHub credential is available.' }
$headers = @{ Authorization = "Bearer $token"; Accept = 'application/vnd.github+json'; 'User-Agent' = 'UDM-historical-setup' }
$archiveApi = 'https://api.github.com/repos/usamakkhan/UDM-release-archive'
if (-not $Versions) { $Versions = (Invoke-RestMethod -Method Get -Uri ($archiveApi + '/releases?per_page=100') -Headers $headers).tag_name | ForEach-Object { $_.TrimStart('v') } }
$output = Join-Path $root 'installer-out'
New-Item -ItemType Directory -Path $output -Force | Out-Null
foreach ($version in $Versions) {
    if ($version -notmatch '^\d+\.\d+(\.\d+)?$') { throw "Invalid version: $version" }
    $setup = Join-Path $output ('UDM-' + $version + '-Setup-x64.exe')
    if (Test-Path -LiteralPath $setup) { Write-Output "Already built: $setup"; continue }
    $release = Invoke-RestMethod -Method Get -Uri ($archiveApi + '/releases/tags/v' + $version) -Headers $headers
    $asset = $release.assets | Where-Object { $_.name -eq ('UDM-' + $version + '-source-and-portable.zip') } | Select-Object -First 1
    if (-not $asset) { throw "Missing private archive for $version" }
    $temp = Join-Path $env:TEMP ('udm-setup-' + $version + '-' + [Guid]::NewGuid().ToString('N'))
    $zip = Join-Path $temp 'release.zip'
    try {
        New-Item -ItemType Directory -Path $temp | Out-Null
        Invoke-WebRequest -Uri $asset.url -Headers @{ Authorization = "Bearer $token"; Accept = 'application/octet-stream'; 'User-Agent' = 'UDM-historical-setup' } -OutFile $zip
        if ((Get-Item $zip).Length -ne $asset.size) { throw "Downloaded archive size mismatch for $version" }
        Expand-Archive -LiteralPath $zip -DestinationPath $temp
        $packageRoot = Join-Path $temp ('UDM-' + $version)
        if (-not (Test-Path -LiteralPath (Join-Path $packageRoot 'release\UDM.exe'))) { throw "Portable package does not contain UDM.exe for $version" }
        Copy-Item -LiteralPath $PSScriptRoot -Destination (Join-Path $packageRoot 'installer') -Recurse -Force
        foreach ($file in @('UdmWfp.sys','UdmWfp.inf','udmwfp.cat','UDM-development-public.cer')) {
            $destination = Join-Path $packageRoot ('drivers\out\x64-test-signed\' + $file)
            New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force | Out-Null
            Copy-Item -LiteralPath (Join-Path $driver $file) -Destination $destination -Force
        }
        foreach ($file in @('ffmpeg.exe','ffprobe.exe','FFmpeg-LICENSE.txt')) {
            $destination = Join-Path $packageRoot ('release\tools\' + $file)
            New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force | Out-Null
            Copy-Item -LiteralPath (Join-Path $root ('release\tools\' + $file)) -Destination $destination -Force
        }
        & $compiler ('/DAppVersion=' + $version) ('/DDriverThumbprint=' + $signing.certificateThumbprint) ('/O' + $output) ('/FUDM-' + $version + '-Setup-x64') (Join-Path $packageRoot 'installer\UDM-Setup.iss')
        if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $setup)) { throw "Setup compilation failed for $version" }
        Write-Output "Built: $setup"
    } finally {
        if (Test-Path -LiteralPath $temp) { Remove-Item -LiteralPath $temp -Force -Recurse }
    }
}
