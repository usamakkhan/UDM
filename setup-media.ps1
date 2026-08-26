$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$mediaTools = Join-Path $PSScriptRoot 'release\tools'
$mediaCache = Join-Path $PSScriptRoot '.media-cache'
New-Item -ItemType Directory -Force -Path $mediaTools,$mediaCache | Out-Null
function Fetch-Verified($Uri, $Destination, $Expected) {
    if ((Test-Path -LiteralPath $Destination) -and (Get-FileHash -LiteralPath $Destination -Algorithm SHA256).Hash -eq $Expected) { return }
    Invoke-WebRequest -Uri $Uri -OutFile ($Destination + '.download')
    if ((Get-FileHash -LiteralPath ($Destination + '.download') -Algorithm SHA256).Hash -ne $Expected) { throw "Checksum failed for $Uri" }
    Move-Item -LiteralPath ($Destination + '.download') -Destination $Destination -Force
}
$ffUri = 'https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-essentials.zip'
$ffSums = (Invoke-WebRequest ($ffUri + '.sha256')).Content
if ($ffSums -is [byte[]]) { $ffSums = [Text.Encoding]::UTF8.GetString($ffSums) }
$ffHash = [regex]::Match($ffSums, '[a-fA-F0-9]{64}').Value
if (-not $ffHash) { throw 'FFmpeg checksum unavailable.' }
$ffZip = Join-Path $mediaCache 'ffmpeg.zip'
Fetch-Verified $ffUri $ffZip $ffHash
Expand-Archive -LiteralPath $ffZip -DestinationPath (Join-Path $mediaCache 'ffmpeg') -Force
$ffFolder = Get-ChildItem -LiteralPath (Join-Path $mediaCache 'ffmpeg') -Directory | Select-Object -First 1
foreach ($exe in @('ffmpeg.exe','ffprobe.exe')) { Copy-Item -LiteralPath (Join-Path $ffFolder.FullName ('bin\' + $exe)) -Destination $mediaTools -Force }
Copy-Item -LiteralPath (Join-Path $ffFolder.FullName 'LICENSE') -Destination (Join-Path $mediaTools 'FFmpeg-LICENSE.txt') -Force
@{ffmpegUrl=$ffUri;ffmpegZipSha256=$ffHash;retrievedUtc=[DateTime]::UtcNow.ToString('o')} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $mediaTools 'provenance.json') -Encoding utf8
Write-Output 'FFmpeg and FFprobe installed and checksums verified. UDM captures and downloads streams; FFmpeg combines local tracks.'
