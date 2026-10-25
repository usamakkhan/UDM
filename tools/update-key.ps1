# The UDM update signing key: an ECDSA P-256 key pair in the user's CNG key store.
#
#   tools\update-key.ps1 -Create            # create "UDM Update Signing" (non-exportable) and write native\UpdateKey.hpp
#   tools\update-key.ps1 -Create -Exportable # same, but the private key may be exported for a backup
#   tools\update-key.ps1 -Show              # print the public key and key id of the existing key
#
# The private key never leaves the key store unless -Exportable was chosen; tools\publish-manifest.ps1
# signs with it by name. The public half is embedded in UDM through native\UpdateKey.hpp, so a build
# made after -Create verifies manifests signed by this key and no other. Keep the key: a new key means
# a new UDM build before any client accepts manifests signed by it.
param([switch]$Create,[switch]$Show,[switch]$Exportable,[string]$Name='UDM Update Signing',[string]$Header)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Security
$root=Split-Path $PSScriptRoot -Parent
if(-not $Header){$Header=Join-Path $root 'native\UpdateKey.hpp'}
if(-not ($Create -or $Show)){throw 'Pass -Create or -Show.'}
if($Create){
 if([System.Security.Cryptography.CngKey]::Exists($Name)){throw ('A key named "'+$Name+'" already exists; use -Show, or pick another -Name.')}
 $parameters=New-Object System.Security.Cryptography.CngKeyCreationParameters
 $parameters.Provider=[System.Security.Cryptography.CngProvider]::MicrosoftSoftwareKeyStorageProvider
 $parameters.KeyUsage=[System.Security.Cryptography.CngKeyUsages]::Signing
 $parameters.ExportPolicy=if($Exportable){[System.Security.Cryptography.CngExportPolicies]::AllowPlaintextExport}else{[System.Security.Cryptography.CngExportPolicies]::None}
 $key=[System.Security.Cryptography.CngKey]::Create([System.Security.Cryptography.CngAlgorithm]::ECDsaP256,$Name,$parameters)
}else{
 if(-not [System.Security.Cryptography.CngKey]::Exists($Name)){throw ('No key named "'+$Name+'" exists; create it with -Create.')}
 $key=[System.Security.Cryptography.CngKey]::Open($Name)
}
# BCRYPT_ECCKEY_BLOB: 4-byte magic, 4-byte key size, then X and Y of 32 bytes each.
$blob=$key.Export([System.Security.Cryptography.CngKeyBlobFormat]::EccPublicBlob)
if($blob.Length -lt 72){throw 'Unexpected public key blob.'}
[byte[]]$xy=$blob[8..71]
$hex=($xy | ForEach-Object { $_.ToString('x2') }) -join ''
$sha=[System.Security.Cryptography.SHA256]::Create()
$keyId=(($sha.ComputeHash($xy))[0..7] | ForEach-Object { $_.ToString('x2') }) -join ''
if($Create){
 $bytes=($xy | ForEach-Object { '0x'+$_.ToString('x2') }) -join ','
 $text=@(
  '#pragma once',
  ('// The public half of the update signing key "'+$Name+'", written by tools/update-key.ps1 on '+(Get-Date).ToString('yyyy-MM-dd')+'.'),
  '// UDM verifies update manifests against this key only; all zeros would mean no key is built in.',
  'namespace udm {',
  ('inline constexpr unsigned char updatePublicKey[64]={'+$bytes+'};'),
  ('inline constexpr const char* updateKeyId="'+$keyId+'";'),
  '}'
 ) -join "`r`n"
 [IO.File]::WriteAllText($Header,$text+"`r`n",(New-Object System.Text.UTF8Encoding($false)))
 Write-Output ('written: '+$Header)
}
Write-Output ('key name:   '+$Name)
Write-Output ('public key: '+$hex)
Write-Output ('key id:     '+$keyId)
Write-Output ('exportable: '+$(if($key.ExportPolicy -ne [System.Security.Cryptography.CngExportPolicies]::None){'yes'}else{'no'}))
