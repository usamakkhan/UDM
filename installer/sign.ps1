# Authenticode signing for UDM's release binaries and setup.
#
# The certificate is the user's: a thumbprint of a code-signing certificate with a private key in
# the CurrentUser\My or LocalMachine\My store (a hardware token's key shows up there through its
# vendor's KSP), a PFX file with its password (parameter or the UDM_SIGN_PFX_PASSWORD environment
# variable), or an Azure Trusted Signing account, where signtool signs through Microsoft's dlib with
# a short-lived certificate issued at signing time. Nothing here creates, imports or trusts a certificate.
#
#   installer\sign.ps1 -Thumbprint <sha1> -DryRun          # report what would be signed and with what
#   installer\sign.ps1 -Thumbprint <sha1>                  # sign the release binaries, then verify them
#   installer\sign.ps1 -PfxPath cert.pfx -Paths a.exe b.exe
#   installer\sign.ps1 -TrustedSigningMetadata metadata.json   # Azure Trusted Signing (dlib found in the NuGet cache)
#
# Dot-source the file to use Get-UdmSignToolPath, Get-UdmSigningCertificate, Get-UdmSignSet,
# Invoke-UdmSign and Test-UdmSignature from build-setup.ps1 or a test.
param(
 [string]$Thumbprint,
 [string]$PfxPath,
 [securestring]$PfxPassword,
 [string]$TrustedSigningMetadata,
 [string]$TrustedSigningDlib,
 [string]$TimestampUrl='http://timestamp.digicert.com',
 [string]$SignToolPath,
 [string[]]$Paths,
 [switch]$DryRun,
 [switch]$NoTimestamp,
 [switch]$Force,
 [switch]$AsModule
)
$ErrorActionPreference='Stop'
$script:udmRoot=Split-Path $PSScriptRoot -Parent

function Get-UdmSignToolPath([string]$Preferred){
 # The cached toolchain's SDK, then installed Windows Kits (newest first), then PATH.
 $candidates=@()
 if($Preferred){$candidates+=$Preferred}
 $cached=Join-Path $script:udmRoot '.media-cache\driver-toolchain\microsoft.windows.sdk.cpp\c\bin'
 if(Test-Path -LiteralPath $cached){$candidates+=Get-ChildItem -LiteralPath $cached -Directory | Sort-Object Name -Descending | ForEach-Object { Join-Path $_.FullName 'x64\signtool.exe' }}
 $kits=Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\bin'
 if(Test-Path -LiteralPath $kits){$candidates+=Get-ChildItem -LiteralPath $kits -Directory | Where-Object Name -match '^10\.' | Sort-Object Name -Descending | ForEach-Object { Join-Path $_.FullName 'x64\signtool.exe' }}
 $onPath=Get-Command signtool.exe -ErrorAction SilentlyContinue
 if($onPath){$candidates+=$onPath.Source}
 $found=$candidates | Where-Object { $_ -and (Test-Path -LiteralPath $_ -PathType Leaf) } | Select-Object -First 1
 if(-not $found){throw 'signtool.exe was not found: install a Windows SDK or pass -SignToolPath.'}
 return $found
}

function Get-UdmTrustedSigningDlib([string]$Preferred){
 # Microsoft's Azure.CodeSigning.Dlib.dll from the Microsoft.Trusted.Signing.Client NuGet package: a path given
 # by the user, then the newest package in the user's NuGet cache, then .media-cache\trusted-signing.
 $candidates=@()
 if($Preferred){$candidates+=$Preferred}
 $nuget=Join-Path $env:USERPROFILE '.nuget\packages\microsoft.trusted.signing.client'
 if(Test-Path -LiteralPath $nuget){$candidates+=Get-ChildItem -LiteralPath $nuget -Directory | Sort-Object { [version]($_.Name -replace '[^0-9.].*$','') } -Descending | ForEach-Object { Join-Path $_.FullName 'bin\x64\Azure.CodeSigning.Dlib.dll' }}
 $cached=Join-Path $script:udmRoot '.media-cache\trusted-signing'
 if(Test-Path -LiteralPath $cached){$candidates+=Get-ChildItem -LiteralPath $cached -Recurse -Filter 'Azure.CodeSigning.Dlib.dll' | Where-Object FullName -like '*\x64\*' | ForEach-Object FullName}
 $found=$candidates | Where-Object { $_ -and (Test-Path -LiteralPath $_ -PathType Leaf) } | Select-Object -First 1
 if(-not $found){throw 'Azure.CodeSigning.Dlib.dll was not found: install the Microsoft.Trusted.Signing.Client NuGet package (nuget install Microsoft.Trusted.Signing.Client) or pass -TrustedSigningDlib.'}
 return $found
}

function Assert-UdmSignToolSupportsDlib([string]$SignTool){
 # /dlib arrived with the Windows SDK 10.0.22621 signtool; older ones fail with an unknown-option error.
 $version=(Get-Item -LiteralPath $SignTool).VersionInfo.FileVersion
 if($version -match '^(\d+)\.(\d+)\.(\d+)' -and ([version]($Matches[1]+'.'+$Matches[2]+'.'+$Matches[3]) -lt [version]'10.0.22621')){throw ('Azure Trusted Signing needs signtool from Windows SDK 10.0.22621 or later; '+$SignTool+' is '+$version+'.')}
}

function Get-UdmSigningCertificate([string]$Thumbprint,[string]$PfxPath,[securestring]$PfxPassword,[string]$TrustedSigningMetadata,[string]$TrustedSigningDlib){
 # Returns @{ Kind='store'|'pfx'|'trusted-signing'; Certificate=<X509Certificate2 or $null>; Thumbprint; Issuer;
 #            TimestampUrl=<the kind's usual authority>; Arguments=<signtool args> }.
 # A Trusted Signing credential has no certificate up front (Microsoft issues one per signing, valid for days),
 # so its signatures are recognized by a valid chain from Microsoft's "Microsoft ID Verified" issuing CAs.
 if(@($Thumbprint,$PfxPath,$TrustedSigningMetadata | Where-Object { $_ }).Count -gt 1){throw 'Pass one of -Thumbprint, -PfxPath or -TrustedSigningMetadata.'}
 if($TrustedSigningMetadata){
  if(-not (Test-Path -LiteralPath $TrustedSigningMetadata -PathType Leaf)){throw ('Trusted Signing metadata file not found: '+$TrustedSigningMetadata)}
  $metadataPath=(Resolve-Path -LiteralPath $TrustedSigningMetadata).Path
  try{$metadata=Get-Content -LiteralPath $metadataPath -Raw | ConvertFrom-Json}catch{throw ('The Trusted Signing metadata is not JSON: '+$_.Exception.Message)}
  foreach($field in 'Endpoint','CodeSigningAccountName','CertificateProfileName'){if(-not $metadata.$field){throw ('Trusted Signing metadata needs "'+$field+'" (Endpoint, CodeSigningAccountName, CertificateProfileName).')}}
  if($metadata.Endpoint -notmatch '^https://[a-z0-9-]+\.codesigning\.azure\.net/?$'){throw ('The Trusted Signing endpoint should look like https://<region>.codesigning.azure.net, not '+$metadata.Endpoint)}
  $dlib=Get-UdmTrustedSigningDlib $TrustedSigningDlib
  return @{Kind='trusted-signing';Certificate=$null;Thumbprint='';Issuer='Microsoft ID Verified';Account=$metadata.CodeSigningAccountName;Profile=$metadata.CertificateProfileName;Endpoint=$metadata.Endpoint;Metadata=$metadataPath;Dlib=$dlib;TimestampUrl='http://timestamp.acs.microsoft.com';Arguments=@('/dlib',$dlib,'/dmdf',$metadataPath)}
 }
 if($Thumbprint){
  $clean=($Thumbprint -replace '[^0-9A-Fa-f]','').ToUpperInvariant()
  if($clean.Length -ne 40){throw 'A certificate thumbprint is 40 hexadecimal characters.'}
  $certificate=@('Cert:\CurrentUser\My','Cert:\LocalMachine\My') | ForEach-Object { Get-ChildItem -LiteralPath $_ -ErrorAction SilentlyContinue | Where-Object Thumbprint -eq $clean } | Select-Object -First 1
  if(-not $certificate){throw ('No certificate with thumbprint '+$clean+' in CurrentUser\My or LocalMachine\My.')}
  if(-not $certificate.HasPrivateKey){throw 'The certificate has no private key here; signing needs the key.'}
  if(-not ($certificate.EnhancedKeyUsageList | Where-Object ObjectId -eq '1.3.6.1.5.5.7.3.3')){throw 'The certificate is not enabled for code signing (EKU 1.3.6.1.5.5.7.3.3).'}
  return @{Kind='store';Certificate=$certificate;Thumbprint=$clean;Issuer='';TimestampUrl='http://timestamp.digicert.com';Arguments=@('/sha1',$clean)}
 }
 if($PfxPath){
  if(-not (Test-Path -LiteralPath $PfxPath -PathType Leaf)){throw ('PFX file not found: '+$PfxPath)}
  if(-not $PfxPassword -and $env:UDM_SIGN_PFX_PASSWORD){$PfxPassword=ConvertTo-SecureString $env:UDM_SIGN_PFX_PASSWORD -AsPlainText -Force}
  $plain=if($PfxPassword){[Runtime.InteropServices.Marshal]::PtrToStringUni([Runtime.InteropServices.Marshal]::SecureStringToGlobalAllocUnicode($PfxPassword))}else{''}
  try{$certificate=New-Object System.Security.Cryptography.X509Certificates.X509Certificate2((Resolve-Path -LiteralPath $PfxPath).Path,$plain,'EphemeralKeySet')}
  catch{throw ('The PFX could not be opened (wrong password?): '+$_.Exception.Message)}
  if(-not $certificate.HasPrivateKey){throw 'The PFX holds no private key.'}
  $arguments=@('/f',(Resolve-Path -LiteralPath $PfxPath).Path);if($plain){$arguments+=@('/p',$plain)}
  return @{Kind='pfx';Certificate=$certificate;Thumbprint=$certificate.Thumbprint;Issuer='';TimestampUrl='http://timestamp.digicert.com';Arguments=$arguments}
 }
 throw 'Pass -Thumbprint <sha1 of a code-signing certificate in your store>, -PfxPath <file> or -TrustedSigningMetadata <metadata.json>.'
}

function Get-UdmCredentialDescription($Credential){
 if($Credential.Kind -eq 'trusted-signing'){return ('Azure Trusted Signing account '+$Credential.Account+', profile '+$Credential.Profile+' ('+$Credential.Endpoint+'; certificate issued by Microsoft at signing time)')}
 return ($Credential.Certificate.Subject+' ('+$Credential.Thumbprint+', '+$Credential.Kind+', expires '+$Credential.Certificate.NotAfter.ToString('yyyy-MM-dd')+')')
}

function Get-UdmSignSet([string]$Release=(Join-Path $script:udmRoot 'release')){
 # UDM's own executables. WinDivert.dll and WinDivert64.sys are the vendor's Microsoft-signed files
 # and their hashes are pinned by Udm.Network.exe: re-signing them would break that check, so they
 # are never in the set. Udm.Automation.tlb is a type library and cannot carry an Authenticode signature.
 $names=@('UDM.exe','Udm.NativeHost.exe','Udm.Monitor.exe','Udm.SetupHelper.exe','Udm.Automation.exe','network\Udm.Network.exe')
 return $names | ForEach-Object { [pscustomobject]@{Name=$_;Path=(Join-Path $Release $_);Present=(Test-Path -LiteralPath (Join-Path $Release $_) -PathType Leaf)} }
}

function Test-UdmSignature([string]$Path,[string]$ExpectedThumbprint,[string]$ExpectedIssuer){
 # Signed by the expected certificate. A self-signed or untrusted chain reports UnknownError or
 # NotTrusted; the signer thumbprint is what proves the file was signed with the given key.
 # With -ExpectedIssuer (Trusted Signing, whose certificate changes), a valid, trusted signature whose
 # signer was issued by that CA family counts instead.
 $signature=Get-AuthenticodeSignature -LiteralPath $Path
 $signer=$signature.SignerCertificate
 $byThumbprint=[bool]($signer -and $ExpectedThumbprint -and $signer.Thumbprint -eq $ExpectedThumbprint)
 $byIssuer=[bool]($signer -and $ExpectedIssuer -and $signature.Status -eq 'Valid' -and $signer.Issuer -match [regex]::Escape($ExpectedIssuer))
 return [pscustomobject]@{Path=$Path;Status=[string]$signature.Status;Signer=if($signer){$signer.Thumbprint}else{''};Issuer=if($signer){$signer.Issuer}else{''};Matches=($byThumbprint -or $byIssuer)}
}

function Test-UdmCredentialSignature([string]$Path,$Credential){
 return Test-UdmSignature -Path $Path -ExpectedThumbprint $Credential.Thumbprint -ExpectedIssuer $Credential.Issuer
}

function Invoke-UdmSign{
 param([Parameter(Mandatory=$true)][string[]]$Files,[Parameter(Mandatory=$true)]$Credential,[string]$SignTool,[string]$TimestampUrl='http://timestamp.digicert.com',[switch]$NoTimestamp,[switch]$Force,[switch]$DryRun)
 if(-not $SignTool){$SignTool=Get-UdmSignToolPath}
 if(-not $PSBoundParameters.ContainsKey('TimestampUrl') -and $Credential.TimestampUrl){$TimestampUrl=$Credential.TimestampUrl}
 if($Credential.Kind -eq 'trusted-signing'){Assert-UdmSignToolSupportsDlib $SignTool}
 $results=[Collections.Generic.List[object]]::new()
 foreach($file in $Files){
  if(-not (Test-Path -LiteralPath $file -PathType Leaf)){throw ('File to sign is missing: '+$file)}
  $before=Test-UdmCredentialSignature -Path $file -Credential $Credential
  if($before.Matches -and -not $Force){$results.Add([pscustomobject]@{Path=$file;Action='already signed';Status=$before.Status});continue}
  if($DryRun){$results.Add([pscustomobject]@{Path=$file;Action='would sign';Status=$before.Status});continue}
  $arguments=@('sign','/fd','SHA256','/d','UDM Download Manager','/du','https://github.com/usamakkhan/UDM')+$Credential.Arguments
  if(-not $NoTimestamp){$arguments+=@('/tr',$TimestampUrl,'/td','SHA256')}
  $arguments+=$file
  $output=& $SignTool @arguments 2>&1
  if($LASTEXITCODE -ne 0){throw ('signtool failed for '+$file+': '+(($output | Out-String).Trim()))}
  $after=Test-UdmCredentialSignature -Path $file -Credential $Credential
  if(-not $after.Matches){throw ('The signature on '+$file+' does not carry the expected certificate ('+$after.Status+', issuer '+$after.Issuer+').')}
  $results.Add([pscustomobject]@{Path=$file;Action='signed';Status=$after.Status})
 }
 return $results
}

function Get-UdmInnoSignToolDefinition{
 # The /S definition for ISCC: Inno substitutes $f with the file to sign (the setup and, with
 # SignedUninstaller=yes, the uninstaller). Quotes are doubled for the command line.
 param([Parameter(Mandatory=$true)]$Credential,[string]$SignTool,[string]$TimestampUrl='http://timestamp.digicert.com',[switch]$NoTimestamp)
 if(-not $SignTool){$SignTool=Get-UdmSignToolPath}
 if(-not $PSBoundParameters.ContainsKey('TimestampUrl') -and $Credential.TimestampUrl){$TimestampUrl=$Credential.TimestampUrl}
 if($Credential.Kind -eq 'trusted-signing'){Assert-UdmSignToolSupportsDlib $SignTool}
 $parts=@('$q'+$SignTool+'$q','sign','/fd','SHA256','/d','$qUDM Download Manager$q')+($Credential.Arguments | ForEach-Object { if($_ -match '\s'){'$q'+$_+'$q'}else{$_} })
 if(-not $NoTimestamp){$parts+=@('/tr',$TimestampUrl,'/td','SHA256')}
 $parts+='$f'
 return 'udm='+($parts -join ' ')
}

if($AsModule){return}
if($MyInvocation.InvocationName -eq '.'){return}

$tool=Get-UdmSignToolPath $SignToolPath
$credential=Get-UdmSigningCertificate -Thumbprint $Thumbprint -PfxPath $PfxPath -PfxPassword $PfxPassword -TrustedSigningMetadata $TrustedSigningMetadata -TrustedSigningDlib $TrustedSigningDlib
if(-not $PSBoundParameters.ContainsKey('TimestampUrl')){$TimestampUrl=$credential.TimestampUrl}
$files=if($Paths){$Paths}else{(Get-UdmSignSet | Where-Object Present).Path}
if(-not $files){throw 'Nothing to sign: build the release first or pass -Paths.'}
Write-Output ('signtool:    '+$tool)
Write-Output ('certificate: '+(Get-UdmCredentialDescription $credential))
Write-Output ('timestamp:   '+$(if($NoTimestamp){'none'}else{$TimestampUrl}))
if($DryRun){Write-Output 'dry run: no file is changed'}
$missing=Get-UdmSignSet | Where-Object { -not $_.Present }
if(-not $Paths -and $missing){Write-Output ('not built, skipped: '+(($missing | ForEach-Object Name) -join ', '))}
Invoke-UdmSign -Files $files -Credential $credential -SignTool $tool -TimestampUrl $TimestampUrl -NoTimestamp:$NoTimestamp -Force:$Force -DryRun:$DryRun | Format-Table -AutoSize | Out-String -Width 200 | Write-Output
