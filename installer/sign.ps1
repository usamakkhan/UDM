# Authenticode signing for UDM's release binaries and setup.
#
# The certificate is the user's: a thumbprint of a code-signing certificate with a private key in
# the CurrentUser\My or LocalMachine\My store, or a PFX file with its password (parameter or the
# UDM_SIGN_PFX_PASSWORD environment variable). Nothing here creates, imports or trusts a certificate.
#
#   installer\sign.ps1 -Thumbprint <sha1> -DryRun          # report what would be signed and with what
#   installer\sign.ps1 -Thumbprint <sha1>                  # sign the release binaries, then verify them
#   installer\sign.ps1 -PfxPath cert.pfx -Paths a.exe b.exe
#
# Dot-source the file to use Get-UdmSignToolPath, Get-UdmSigningCertificate, Get-UdmSignSet,
# Invoke-UdmSign and Test-UdmSignature from build-setup.ps1 or a test.
param(
 [string]$Thumbprint,
 [string]$PfxPath,
 [securestring]$PfxPassword,
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

function Get-UdmSigningCertificate([string]$Thumbprint,[string]$PfxPath,[securestring]$PfxPassword){
 # Returns @{ Kind='store'|'pfx'; Certificate=<X509Certificate2>; Thumbprint; Arguments=<signtool args> }.
 if($Thumbprint -and $PfxPath){throw 'Pass either -Thumbprint or -PfxPath, not both.'}
 if($Thumbprint){
  $clean=($Thumbprint -replace '[^0-9A-Fa-f]','').ToUpperInvariant()
  if($clean.Length -ne 40){throw 'A certificate thumbprint is 40 hexadecimal characters.'}
  $certificate=@('Cert:\CurrentUser\My','Cert:\LocalMachine\My') | ForEach-Object { Get-ChildItem -LiteralPath $_ -ErrorAction SilentlyContinue | Where-Object Thumbprint -eq $clean } | Select-Object -First 1
  if(-not $certificate){throw ('No certificate with thumbprint '+$clean+' in CurrentUser\My or LocalMachine\My.')}
  if(-not $certificate.HasPrivateKey){throw 'The certificate has no private key here; signing needs the key.'}
  if(-not ($certificate.EnhancedKeyUsageList | Where-Object ObjectId -eq '1.3.6.1.5.5.7.3.3')){throw 'The certificate is not enabled for code signing (EKU 1.3.6.1.5.5.7.3.3).'}
  return @{Kind='store';Certificate=$certificate;Thumbprint=$clean;Arguments=@('/sha1',$clean)}
 }
 if($PfxPath){
  if(-not (Test-Path -LiteralPath $PfxPath -PathType Leaf)){throw ('PFX file not found: '+$PfxPath)}
  if(-not $PfxPassword -and $env:UDM_SIGN_PFX_PASSWORD){$PfxPassword=ConvertTo-SecureString $env:UDM_SIGN_PFX_PASSWORD -AsPlainText -Force}
  $plain=if($PfxPassword){[Runtime.InteropServices.Marshal]::PtrToStringUni([Runtime.InteropServices.Marshal]::SecureStringToGlobalAllocUnicode($PfxPassword))}else{''}
  try{$certificate=New-Object System.Security.Cryptography.X509Certificates.X509Certificate2((Resolve-Path -LiteralPath $PfxPath).Path,$plain,'EphemeralKeySet')}
  catch{throw ('The PFX could not be opened (wrong password?): '+$_.Exception.Message)}
  if(-not $certificate.HasPrivateKey){throw 'The PFX holds no private key.'}
  $arguments=@('/f',(Resolve-Path -LiteralPath $PfxPath).Path);if($plain){$arguments+=@('/p',$plain)}
  return @{Kind='pfx';Certificate=$certificate;Thumbprint=$certificate.Thumbprint;Arguments=$arguments}
 }
 throw 'Pass -Thumbprint <sha1 of a code-signing certificate in your store> or -PfxPath <file>.'
}

function Get-UdmSignSet([string]$Release=(Join-Path $script:udmRoot 'release')){
 # UDM's own executables. WinDivert.dll and WinDivert64.sys are the vendor's Microsoft-signed files
 # and their hashes are pinned by Udm.Network.exe: re-signing them would break that check, so they
 # are never in the set. Udm.Automation.tlb is a type library and cannot carry an Authenticode signature.
 $names=@('UDM.exe','Udm.NativeHost.exe','Udm.Monitor.exe','Udm.SetupHelper.exe','Udm.Automation.exe','network\Udm.Network.exe')
 return $names | ForEach-Object { [pscustomobject]@{Name=$_;Path=(Join-Path $Release $_);Present=(Test-Path -LiteralPath (Join-Path $Release $_) -PathType Leaf)} }
}

function Test-UdmSignature([string]$Path,[string]$ExpectedThumbprint){
 # Signed by the expected certificate. A self-signed or untrusted chain reports UnknownError or
 # NotTrusted; the signer thumbprint is what proves the file was signed with the given key.
 $signature=Get-AuthenticodeSignature -LiteralPath $Path
 $signer=$signature.SignerCertificate
 return [pscustomobject]@{Path=$Path;Status=[string]$signature.Status;Signer=if($signer){$signer.Thumbprint}else{''};Matches=($signer -and $ExpectedThumbprint -and $signer.Thumbprint -eq $ExpectedThumbprint)}
}

function Invoke-UdmSign{
 param([Parameter(Mandatory=$true)][string[]]$Files,[Parameter(Mandatory=$true)]$Credential,[string]$SignTool,[string]$TimestampUrl='http://timestamp.digicert.com',[switch]$NoTimestamp,[switch]$Force,[switch]$DryRun)
 if(-not $SignTool){$SignTool=Get-UdmSignToolPath}
 $results=[Collections.Generic.List[object]]::new()
 foreach($file in $Files){
  if(-not (Test-Path -LiteralPath $file -PathType Leaf)){throw ('File to sign is missing: '+$file)}
  $before=Test-UdmSignature -Path $file -ExpectedThumbprint $Credential.Thumbprint
  if($before.Matches -and -not $Force){$results.Add([pscustomobject]@{Path=$file;Action='already signed';Status=$before.Status});continue}
  if($DryRun){$results.Add([pscustomobject]@{Path=$file;Action='would sign';Status=$before.Status});continue}
  $arguments=@('sign','/fd','SHA256','/d','UDM Download Manager','/du','https://github.com/usamakkhan/UDM')+$Credential.Arguments
  if(-not $NoTimestamp){$arguments+=@('/tr',$TimestampUrl,'/td','SHA256')}
  $arguments+=$file
  $output=& $SignTool @arguments 2>&1
  if($LASTEXITCODE -ne 0){throw ('signtool failed for '+$file+': '+(($output | Out-String).Trim()))}
  $after=Test-UdmSignature -Path $file -ExpectedThumbprint $Credential.Thumbprint
  if(-not $after.Matches){throw ('The signature on '+$file+' does not carry the expected certificate ('+$after.Status+').')}
  $results.Add([pscustomobject]@{Path=$file;Action='signed';Status=$after.Status})
 }
 return $results
}

function Get-UdmInnoSignToolDefinition{
 # The /S definition for ISCC: Inno substitutes $f with the file to sign (the setup and, with
 # SignedUninstaller=yes, the uninstaller). Quotes are doubled for the command line.
 param([Parameter(Mandatory=$true)]$Credential,[string]$SignTool,[string]$TimestampUrl='http://timestamp.digicert.com',[switch]$NoTimestamp)
 if(-not $SignTool){$SignTool=Get-UdmSignToolPath}
 $parts=@('$q'+$SignTool+'$q','sign','/fd','SHA256','/d','$qUDM Download Manager$q')+($Credential.Arguments | ForEach-Object { if($_ -match '\s'){'$q'+$_+'$q'}else{$_} })
 if(-not $NoTimestamp){$parts+=@('/tr',$TimestampUrl,'/td','SHA256')}
 $parts+='$f'
 return 'udm='+($parts -join ' ')
}

if($AsModule){return}
if($MyInvocation.InvocationName -eq '.'){return}

$tool=Get-UdmSignToolPath $SignToolPath
$credential=Get-UdmSigningCertificate -Thumbprint $Thumbprint -PfxPath $PfxPath -PfxPassword $PfxPassword
$files=if($Paths){$Paths}else{(Get-UdmSignSet | Where-Object Present).Path}
if(-not $files){throw 'Nothing to sign: build the release first or pass -Paths.'}
Write-Output ('signtool:    '+$tool)
Write-Output ('certificate: '+$credential.Certificate.Subject+' ('+$credential.Thumbprint+', '+$credential.Kind+', expires '+$credential.Certificate.NotAfter.ToString('yyyy-MM-dd')+')')
Write-Output ('timestamp:   '+$(if($NoTimestamp){'none'}else{$TimestampUrl}))
if($DryRun){Write-Output 'dry run: no file is changed'}
$missing=Get-UdmSignSet | Where-Object { -not $_.Present }
if(-not $Paths -and $missing){Write-Output ('not built, skipped: '+(($missing | ForEach-Object Name) -join ', '))}
Invoke-UdmSign -Files $files -Credential $credential -SignTool $tool -TimestampUrl $TimestampUrl -NoTimestamp:$NoTimestamp -Force:$Force -DryRun:$DryRun | Format-Table -AutoSize | Out-String -Width 200 | Write-Output
