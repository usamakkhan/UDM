# Writes the signed update manifest (latest.json) for a built setup.
#
#   tools\publish-manifest.ps1 -Setup installer-out\UDM-1.0.5-Browser-0.63.21-Setup-x64.exe -Version 1.0.5 `
#       -SetupUrl https://github.com/usamakkhan/UDM/releases/download/UDM-1.0.5/UDM-1.0.5-Browser-0.63.21-Setup-x64.exe `
#       -Notes https://github.com/usamakkhan/UDM/releases/tag/UDM-1.0.5 -Out installer-out\latest.json
#
# Upload latest.json as a release asset of the same release: UDM fetches
# https://github.com/usamakkhan/UDM/releases/latest/download/latest.json. The manifest's bytes are
# signed as written (ECDSA P-256 over SHA-256, IEEE P1363 r||s) with the key from tools\update-key.ps1;
# the setup's SHA-256 inside it is what UDM checks the download against before the publisher
# signature is verified and the setup is run.
param(
 [Parameter(Mandatory=$true)][string]$Setup,
 [Parameter(Mandatory=$true)][string]$Version,
 [Parameter(Mandatory=$true)][string]$SetupUrl,
 [string]$BrowserVersion,
 [string]$Notes,
 [string]$Out='latest.json',
 [string]$KeyName='UDM Update Signing'
)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Security
if($Version -notmatch '^\d+\.\d+(\.\d+)?$'){throw 'Use a numeric release version.'}
if($SetupUrl -notmatch '^https://github\.com/usamakkhan/UDM/releases/download/[^?#\s]+\.exe$'){throw 'The setup URL must be a release asset of the UDM repository.'}
if($Notes -and $Notes -notmatch '^https://github\.com/'){throw 'Release notes must be a GitHub address.'}
if(-not (Test-Path -LiteralPath $Setup -PathType Leaf)){throw ('Setup not found: '+$Setup)}
if(-not [System.Security.Cryptography.CngKey]::Exists($KeyName)){throw ('No update signing key named "'+$KeyName+'"; create it with tools\update-key.ps1 -Create.')}
$item=Get-Item -LiteralPath $Setup
$manifest=[ordered]@{
 version=$Version
 browserVersion=$BrowserVersion
 published=(Get-Date).ToString('yyyy-MM-dd')
 notes=$Notes
 setup=[ordered]@{url=$SetupUrl;sha256=(Get-FileHash -LiteralPath $Setup -Algorithm SHA256).Hash.ToLowerInvariant();size=$item.Length}
}
$json=$manifest | ConvertTo-Json -Compress -Depth 4
$bytes=[Text.Encoding]::UTF8.GetBytes($json)
$key=[System.Security.Cryptography.CngKey]::Open($KeyName)
$ecdsa=New-Object System.Security.Cryptography.ECDsaCng($key)
$ecdsa.HashAlgorithm=[System.Security.Cryptography.CngAlgorithm]::Sha256
$signature=$ecdsa.SignData($bytes)
if($signature.Length -ne 64){throw 'Unexpected signature length.'}
$blob=$key.Export([System.Security.Cryptography.CngKeyBlobFormat]::EccPublicBlob);[byte[]]$xy=$blob[8..71]
$keyId=(([System.Security.Cryptography.SHA256]::Create().ComputeHash($xy))[0..7] | ForEach-Object { $_.ToString('x2') }) -join ''
$envelope=[ordered]@{algorithm='ecdsa-p256-sha256';keyId=$keyId;manifest=[Convert]::ToBase64String($bytes);signature=[Convert]::ToBase64String($signature)}
$Out=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Out)
[IO.File]::WriteAllText($Out,($envelope | ConvertTo-Json -Compress),(New-Object System.Text.UTF8Encoding($false)))
Write-Output ('written: '+$Out)
Write-Output ('version: '+$Version+'  setup: '+$item.Name+'  sha256: '+$manifest.setup.sha256+'  size: '+$item.Length)
Write-Output ('key id:  '+$keyId)
