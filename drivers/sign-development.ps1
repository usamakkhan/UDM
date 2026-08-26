param()
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$cache=Join-Path $root '.media-cache\driver-toolchain'
$signTool=Join-Path $cache 'microsoft.windows.sdk.cpp\c\bin\10.0.26100.0\x64\signtool.exe'
$inf2cat=Join-Path $cache 'wdk\c\bin\10.0.26100.0\x86\Inf2Cat.exe'
$nuget=Join-Path $cache 'nuget.exe'
foreach($tool in @($signTool,$nuget)){$signature=Get-AuthenticodeSignature $tool;if($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'Microsoft Corporation'){throw "Microsoft tool verification failed: $tool"}}
# Inf2Cat has a Microsoft internal tool signature that the public Windows trust
# store does not accept. Verify its signed distribution and exact payload instead.
$wdkPackage=Join-Path $cache 'wdk.nupkg'
& $nuget verify -All $wdkPackage | Out-Null
if($LASTEXITCODE -ne 0){throw 'WDK package signature verification failed.'}
$archive=[IO.Compression.ZipFile]::OpenRead($wdkPackage)
try{
    $entry=$archive.GetEntry('c/bin/10.0.26100.0/x86/Inf2Cat.exe')
    if(!$entry){throw 'Inf2Cat missing from verified package.'}
    $stream=$entry.Open();$sha=[Security.Cryptography.SHA256]::Create()
    try{$expected=[BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','')}finally{$stream.Dispose();$sha.Dispose()}
    if((Get-FileHash $inf2cat).Hash -ne $expected){throw 'Inf2Cat does not match its signed package.'}
}finally{$archive.Dispose()}
$built=Join-Path $PSScriptRoot 'out\x64'
$evidence=Get-Content (Join-Path $built 'build-evidence.json') -Raw | ConvertFrom-Json
if(!$evidence.analyzed -or !$evidence.infVerified){throw 'Build and analyze the driver first.'}
foreach($entry in @(@('UdmMonitor.c','sourceSha256'),@('UdmWfpProtocol.h','protocolSha256'))){if((Get-FileHash (Join-Path $PSScriptRoot $entry[0])).Hash -ne $evidence.($entry[1])){throw 'Source changed; rebuild before signing.'}}
if((Get-FileHash (Join-Path $built 'UdmWfp.sys')).Hash -ne $evidence.driverSha256){throw 'Binary differs from build evidence.'}
$out=Join-Path $PSScriptRoot 'out\x64-test-signed'
New-Item -ItemType Directory -Path $out -Force | Out-Null
foreach($file in @('UdmWfp.sys','UdmWfp.inf','UdmWfp.pdb','build-evidence.json')){Copy-Item -LiteralPath (Join-Path $built $file) -Destination $out -Force}
# A non-exportable development key in Personal only. Never add it to a trust store.
$friendly='UDM Development Test Signing'
$certificate=Get-ChildItem Cert:\CurrentUser\My -CodeSigningCert | Where-Object {$_.FriendlyName -eq $friendly -and $_.Subject -eq 'CN=UDM Development Test Only' -and $_.HasPrivateKey -and $_.NotAfter -gt (Get-Date).AddDays(30)} | Sort-Object NotAfter -Descending | Select-Object -First 1
if(!$certificate){$certificate=New-SelfSignedCertificate -Type CodeSigningCert -Subject 'CN=UDM Development Test Only' -FriendlyName $friendly -CertStoreLocation Cert:\CurrentUser\My -KeyAlgorithm RSA -KeyLength 3072 -HashAlgorithm SHA256 -KeyExportPolicy NonExportable -NotAfter (Get-Date).AddYears(1)}
Export-Certificate -Cert $certificate -FilePath (Join-Path $out 'UDM-development-public.cer') -Force | Out-Null
& $signTool sign /fd SHA256 /s My /sha1 $certificate.Thumbprint (Join-Path $out 'UdmWfp.sys')
if($LASTEXITCODE -ne 0){throw 'Driver test signing failed.'}
& $inf2cat "/driver:$out" /os:10_GE_X64
if($LASTEXITCODE -ne 0){throw 'Catalog creation failed.'}
& $signTool sign /fd SHA256 /s My /sha1 $certificate.Thumbprint (Join-Path $out 'UdmWfp.cat')
if($LASTEXITCODE -ne 0){throw 'Catalog test signing failed.'}
# Check the CMS signatures without installing a trusted root. Windows chain trust
# is recorded separately and is expected to fail for this untrusted test identity.
Add-Type -AssemblyName System.Security
function Test-CmsSignature([byte[]]$bytes){$cms=New-Object System.Security.Cryptography.Pkcs.SignedCms;$cms.Decode($bytes);$cms.CheckSignature($true);if($cms.SignerInfos.Count -ne 1 -or $cms.SignerInfos[0].Certificate.Thumbprint -ne $certificate.Thumbprint){throw 'Unexpected signer.'}}
$binary=[IO.File]::ReadAllBytes((Join-Path $out 'UdmWfp.sys'))
$pe=[BitConverter]::ToInt32($binary,60);$optional=$pe+24
if([BitConverter]::ToUInt16($binary,$optional) -ne 0x20b){throw 'Expected an x64 PE image.'}
$table=[BitConverter]::ToInt32($binary,$optional+112+32);$length=[BitConverter]::ToInt32($binary,$table)
if($table -le 0 -or $length -lt 8 -or [long]$table+$length -gt $binary.Length){throw 'Invalid PE signature table.'}
$cmsBytes=New-Object byte[] ($length-8);[Buffer]::BlockCopy($binary,$table+8,$cmsBytes,0,$cmsBytes.Length)
Test-CmsSignature $cmsBytes
Test-CmsSignature ([IO.File]::ReadAllBytes((Join-Path $out 'UdmWfp.cat')))
$sysSignature=Get-AuthenticodeSignature (Join-Path $out 'UdmWfp.sys')
$catSignature=Get-AuthenticodeSignature (Join-Path $out 'UdmWfp.cat')
if($sysSignature.Status -eq 'HashMismatch' -or $catSignature.Status -eq 'HashMismatch'){throw 'Signed content digest mismatch.'}
$record=[ordered]@{signedUtc=[DateTime]::UtcNow.ToString('o');kind='Development test signature only';certificateThumbprint=$certificate.Thumbprint;certificateExpires=$certificate.NotAfter.ToUniversalTime().ToString('o');privateKeyExported=$false;trustedRootInstalled=$false;cmsSignaturesVerified=$true;driverSha256=(Get-FileHash (Join-Path $out 'UdmWfp.sys')).Hash;catalogSha256=(Get-FileHash (Join-Path $out 'UdmWfp.cat')).Hash;windowsDriverTrust=$sysSignature.Status.ToString();windowsDriverTrustDetail=$sysSignature.StatusMessage;windowsCatalogTrust=$catSignature.Status.ToString();productionSigned=$false;installed=$false}
$record | ConvertTo-Json | Set-Content (Join-Path $out 'signing-evidence.json')
Write-Output "Test-signed driver and catalog: $out. No trust, boot settings or driver installation were changed."
