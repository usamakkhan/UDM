# End-to-end check of installer\sign.ps1 without a real publisher certificate: a temporary self-signed
# code-signing certificate signs copies of UDM's executables, the signatures are verified by signer
# thumbprint, the dry run changes nothing, a second run is idempotent, and the vendor's WinDivert files
# are never in the sign set. The Azure Trusted Signing route is checked up to the signtool command line
# with fixture metadata and a placeholder dlib (no Azure call). The certificate is removed afterwards;
# nothing is trusted or installed.
param([Parameter(Mandatory=$true)][string]$RunRoot)
$ErrorActionPreference='Stop'
$RunRoot=[IO.Path]::GetFullPath($RunRoot)
if(Test-Path -LiteralPath $RunRoot){throw 'Use a new isolated RunRoot.'}
New-Item -ItemType Directory -Path $RunRoot | Out-Null
$checks=[Collections.Generic.List[object]]::new();$failure=''
function check([bool]$ok,[string]$name){$checks.Add([pscustomobject]@{name=$name;passed=$ok});if(-not $ok){throw $name}}
. (Join-Path $PSScriptRoot 'sign.ps1') -AsModule
$root=Split-Path $PSScriptRoot -Parent
$certificate=$null
try{
 $release=Join-Path $RunRoot 'release';New-Item -ItemType Directory -Path (Join-Path $release 'network\runtime') -Force | Out-Null
 foreach($entry in Get-UdmSignSet){ if($entry.Present){Copy-Item -LiteralPath $entry.Path -Destination (Join-Path $release $entry.Name)} }
 $fixtures=Get-UdmSignSet -Release $release | Where-Object Present
 check ($fixtures.Count -ge 2) 'Built release executables are available as signing fixtures'
 check (-not ((Get-UdmSignSet | ForEach-Object Name) -match 'WinDivert')) 'The sign set never includes the vendor-signed WinDivert driver or DLL'
 check (-not ((Get-UdmSignSet | ForEach-Object Name) -match '\.tlb$')) 'The sign set never includes the type library'
 $tool=Get-UdmSignToolPath;check (Test-Path -LiteralPath $tool -PathType Leaf) 'signtool is located in the cached SDK or an installed Windows Kit'
 $certificate=New-SelfSignedCertificate -Type CodeSigningCert -Subject 'CN=UDM signing test (temporary)' -CertStoreLocation Cert:\CurrentUser\My -NotAfter (Get-Date).AddDays(1) -KeyExportPolicy Exportable
 $credential=Get-UdmSigningCertificate -Thumbprint $certificate.Thumbprint
 check ($credential.Kind -eq 'store' -and $credential.Thumbprint -eq $certificate.Thumbprint) 'A code-signing certificate is found in the user store by thumbprint'
 $rejected=$false;try{Get-UdmSigningCertificate -Thumbprint ('0'*40) | Out-Null}catch{$rejected=$true};check $rejected 'An unknown thumbprint is refused'
 $hashesBefore=@{};foreach($f in $fixtures){$hashesBefore[$f.Path]=(Get-FileHash -LiteralPath $f.Path -Algorithm SHA256).Hash}
 $dry=Invoke-UdmSign -Files $fixtures.Path -Credential $credential -SignTool $tool -NoTimestamp -DryRun
 check (@($dry | Where-Object Action -eq 'would sign').Count -eq $fixtures.Count) 'The dry run reports every unsigned file as one it would sign'
 check (($fixtures | Where-Object { (Get-FileHash -LiteralPath $_.Path -Algorithm SHA256).Hash -ne $hashesBefore[$_.Path] }).Count -eq 0) 'The dry run changes no file'
 $signed=Invoke-UdmSign -Files $fixtures.Path -Credential $credential -SignTool $tool -NoTimestamp
 check (@($signed | Where-Object Action -eq 'signed').Count -eq $fixtures.Count) 'Every fixture is signed'
 $verified=$fixtures | ForEach-Object { Test-UdmSignature -Path $_.Path -ExpectedThumbprint $certificate.Thumbprint }
 check (@($verified | Where-Object Matches).Count -eq $fixtures.Count) 'Each signature carries the test certificate as its signer'
 $again=Invoke-UdmSign -Files $fixtures.Path -Credential $credential -SignTool $tool -NoTimestamp
 check (@($again | Where-Object Action -eq 'already signed').Count -eq $fixtures.Count) 'A second run leaves files already signed with this certificate alone'
 $forced=Invoke-UdmSign -Files @($fixtures[0].Path) -Credential $credential -SignTool $tool -NoTimestamp -Force
 check (@($forced | Where-Object Action -eq 'signed').Count -eq 1) '-Force re-signs'
 $pfx=Join-Path $RunRoot 'test.pfx';$password=ConvertTo-SecureString 'fixture-password' -AsPlainText -Force
 Export-PfxCertificate -Cert $certificate -FilePath $pfx -Password $password | Out-Null
 $fromPfx=Get-UdmSigningCertificate -PfxPath $pfx -PfxPassword $password
 check ($fromPfx.Kind -eq 'pfx' -and $fromPfx.Thumbprint -eq $certificate.Thumbprint) 'A PFX with its password yields the same certificate'
 $wrong=$false;try{Get-UdmSigningCertificate -PfxPath $pfx -PfxPassword (ConvertTo-SecureString 'wrong' -AsPlainText -Force) | Out-Null}catch{$wrong=$true};check $wrong 'A wrong PFX password is refused'
 $definition=Get-UdmInnoSignToolDefinition -Credential $credential -SignTool $tool -NoTimestamp
 check ($definition -like 'udm=$q*signtool.exe$q sign /fd SHA256*/sha1 '+$certificate.Thumbprint+' $f') 'The Inno Setup SignTool definition names signtool, the digest, the certificate and $f'
 $acsDir=Join-Path $RunRoot 'acs';New-Item -ItemType Directory -Path $acsDir | Out-Null
 $dlib=Join-Path $acsDir 'Azure.CodeSigning.Dlib.dll';Set-Content -LiteralPath $dlib -Value 'placeholder'
 $metadata=Join-Path $acsDir 'metadata.json';Set-Content -LiteralPath $metadata -Value '{"Endpoint":"https://weu.codesigning.azure.net","CodeSigningAccountName":"udm-test","CertificateProfileName":"udm-public"}'
 $acs=Get-UdmSigningCertificate -TrustedSigningMetadata $metadata -TrustedSigningDlib $dlib
 check ($acs.Kind -eq 'trusted-signing' -and $acs.Account -eq 'udm-test' -and $acs.Profile -eq 'udm-public' -and $acs.Arguments[0] -eq '/dlib' -and $acs.Arguments[1] -eq $dlib -and $acs.Arguments[2] -eq '/dmdf' -and $acs.Arguments[3] -eq $metadata -and $acs.TimestampUrl -eq 'http://timestamp.acs.microsoft.com') 'Trusted Signing metadata yields a dlib/dmdf credential with the Microsoft timestamp server'
 $acsDry=Invoke-UdmSign -Files @($fixtures[1].Path) -Credential $acs -SignTool $tool -DryRun
 check (@($acsDry | Where-Object Action -eq 'would sign').Count -eq 1) 'A file signed by another certificate would be re-signed under Trusted Signing'
 $acsDefinition=Get-UdmInnoSignToolDefinition -Credential $acs -SignTool $tool
 check ($acsDefinition -like ('udm=$q*signtool.exe$q sign /fd SHA256*/dlib '+$dlib+' /dmdf '+$metadata+' /tr http://timestamp.acs.microsoft.com /td SHA256 $f')) 'The Inno definition for Trusted Signing passes the dlib, the metadata and the Microsoft timestamp server'
 $incomplete=Join-Path $acsDir 'incomplete.json';Set-Content -LiteralPath $incomplete -Value '{"Endpoint":"https://weu.codesigning.azure.net"}'
 $refused=$false;try{Get-UdmSigningCertificate -TrustedSigningMetadata $incomplete -TrustedSigningDlib $dlib | Out-Null}catch{$refused=$true};check $refused 'Metadata without the account and profile names is refused'
 $both=$false;try{Get-UdmSigningCertificate -Thumbprint $certificate.Thumbprint -TrustedSigningMetadata $metadata -TrustedSigningDlib $dlib | Out-Null}catch{$both=$true};check $both 'Trusted Signing and a store certificate cannot both be given'
 $noDlib=$false;try{Get-UdmSigningCertificate -TrustedSigningMetadata $metadata -TrustedSigningDlib (Join-Path $acsDir 'absent.dll') | Out-Null}catch{$noDlib=$true};check $noDlib 'A missing dlib is reported with the package to install'
 check (-not (Test-UdmSignature -Path $fixtures[0].Path -ExpectedIssuer 'Microsoft ID Verified').Matches) 'A self-signed signature does not pass as a Trusted Signing one'
 # build-setup.ps1 dot-sources sign.ps1, whose param block shares names with its own; the dry run proves the values survive.
 $setupDry=& (Join-Path $PSScriptRoot 'build-setup.ps1') -TrustedSigningMetadata $metadata -TrustedSigningDlib $dlib -SignToolPath $tool -NoTimestamp -SignDryRun
 $setupText=($setupDry | Out-String)
 check (($setupText -match 'Azure Trusted Signing account udm-test, profile udm-public') -and ($setupText -match 'timestamp:\s+none') -and ($setupText -match 'dry run: nothing was signed or built')) 'build-setup.ps1 keeps the Trusted Signing metadata, signtool path and -NoTimestamp across dot-sourcing sign.ps1'
 $setupDefault=& (Join-Path $PSScriptRoot 'build-setup.ps1') -TrustedSigningMetadata $metadata -TrustedSigningDlib $dlib -SignDryRun | Out-String
 check ($setupDefault -match 'timestamp:\s+http://timestamp\.acs\.microsoft\.com') 'build-setup.ps1 defaults the timestamp authority to the one the credential names'
 $release2=Join-Path $root 'release'
 check (@(Get-UdmSignSet -Release $release2 | Where-Object Name -eq 'network\Udm.Network.exe').Count -eq 1) 'The network helper is part of the release sign set'
 $result=0
}catch{$failure=$_.Exception.Message;$result=1}
finally{
 if($certificate){Remove-Item -LiteralPath ('Cert:\CurrentUser\My\'+$certificate.Thumbprint) -ErrorAction SilentlyContinue}
 $gone=-not (Get-ChildItem Cert:\CurrentUser\My | Where-Object Subject -eq 'CN=UDM signing test (temporary)')
 $checks.Add([pscustomobject]@{name='The temporary certificate is removed from the store';passed=$gone});if(-not $gone){$result=1}
}
[pscustomobject]@{passed=($result -eq 0);error=$failure;checks=$checks} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $RunRoot 'results.json') -Encoding UTF8
$checks | ForEach-Object { Write-Output (($(if($_.passed){'PASS'}else{'FAIL'}))+' '+$_.name) }
Write-Output ("$(@($checks | Where-Object passed).Count) passed, $(@($checks | Where-Object { -not $_.passed }).Count) failed")
exit $result
