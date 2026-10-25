# Interop check for the updater's manifest signing: a temporary key made by tools\update-key.ps1 signs a
# manifest with tools\publish-manifest.ps1, and the native verifier (Udm.NativeTests.exe --verify-manifest)
# accepts it, reports the version as newer, and rejects a tampered manifest, a wrong key and a wrong
# algorithm. The key and the header it writes stay inside RunRoot; nothing in the repository changes.
param([Parameter(Mandatory=$true)][string]$RunRoot,[Parameter(Mandatory=$true)][string]$Tests)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Security
$RunRoot=[IO.Path]::GetFullPath($RunRoot)
if(Test-Path -LiteralPath $RunRoot){throw 'Use a new isolated RunRoot.'}
if(-not (Test-Path -LiteralPath $Tests -PathType Leaf)){throw 'Pass the built Udm.NativeTests.exe.'}
New-Item -ItemType Directory -Path $RunRoot | Out-Null
$checks=[Collections.Generic.List[object]]::new();$failure='';$result=1
function check([bool]$ok,[string]$name){$checks.Add([pscustomobject]@{name=$name;passed=$ok});if(-not $ok){throw $name}}
$root=Split-Path $PSScriptRoot -Parent
$keyName='UDM updater test '+[guid]::NewGuid().ToString('N')
try{
 $header=Join-Path $RunRoot 'UpdateKey.hpp'
 $created=& (Join-Path $root 'tools\update-key.ps1') -Create -Name $keyName -Header $header
 check ([System.Security.Cryptography.CngKey]::Exists($keyName)) 'update-key.ps1 -Create makes a key in the user store'
 $hex=($created | Where-Object { $_ -like 'public key: *' }) -replace '^public key: ',''
 check ($hex -match '^[0-9a-f]{128}$') 'The tool prints the 64-byte public key'
 $headerText=Get-Content -LiteralPath $header -Raw
 check (($headerText -match 'updatePublicKey\[64\]=\{0x[0-9a-f]{2}(,0x[0-9a-f]{2}){63}\}') -and ($headerText -notmatch '=\{0\}')) 'The header carries the public key for the build'
 $shown=& (Join-Path $root 'tools\update-key.ps1') -Show -Name $keyName -Header $header
 check ((($shown | Where-Object { $_ -like 'public key: *' }) -replace '^public key: ','') -eq $hex) '-Show prints the same public key'
 check ((Get-Content -LiteralPath $header -Raw) -eq $headerText) '-Show does not rewrite the header'
 $setup=Join-Path $RunRoot 'UDM-9.9.9-Browser-0.99.0-Setup-x64.exe';Copy-Item -LiteralPath (Join-Path $root 'release\Udm.SetupHelper.exe') -Destination $setup
 $out=Join-Path $RunRoot 'latest.json'
 & (Join-Path $root 'tools\publish-manifest.ps1') -Setup $setup -Version 9.9.9 -BrowserVersion 0.99.0 -SetupUrl 'https://github.com/usamakkhan/UDM/releases/download/UDM-9.9.9/UDM-9.9.9-Browser-0.99.0-Setup-x64.exe' -Notes 'https://github.com/usamakkhan/UDM/releases/tag/UDM-9.9.9' -Out $out -KeyName $keyName | Out-Null
 $envelope=Get-Content -LiteralPath $out -Raw | ConvertFrom-Json
 check ($envelope.algorithm -eq 'ecdsa-p256-sha256' -and $envelope.manifest -and $envelope.signature) 'publish-manifest.ps1 writes the signed envelope'
 $inner=[Text.Encoding]::UTF8.GetString([Convert]::FromBase64String($envelope.manifest)) | ConvertFrom-Json
 check ($inner.version -eq '9.9.9' -and $inner.setup.sha256 -eq (Get-FileHash -LiteralPath $setup -Algorithm SHA256).Hash.ToLowerInvariant() -and $inner.setup.size -eq (Get-Item -LiteralPath $setup).Length) 'The manifest carries the setup hash and size'
 $verified=& $Tests --verify-manifest $out $hex 1.0.4
 check (($verified -join ' ') -like 'ok version=9.9.9 newer=1 setup=UDM-9.9.9-Browser-0.99.0-Setup-x64.exe sha256=*') 'The native verifier accepts the PowerShell-signed manifest and reports it as newer'
 $same=& $Tests --verify-manifest $out $hex 9.9.9
 check (($same -join ' ') -like 'ok version=9.9.9 newer=0*') 'An installed version equal to the manifest is not offered the update'
 $tampered=Join-Path $RunRoot 'tampered.json'
 $changed=$inner | Select-Object *;$changed.version='9.9.10'
 $bad=$envelope | Select-Object *;$bad.manifest=[Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes(($changed | ConvertTo-Json -Compress -Depth 4)))
 [IO.File]::WriteAllText($tampered,($bad | ConvertTo-Json -Compress),(New-Object System.Text.UTF8Encoding($false)))
 $rejected=& $Tests --verify-manifest $tampered $hex 1.0.4
 check (($rejected -join ' ') -like 'error:*signature*') 'A manifest changed after signing is rejected natively'
 $wrongKey=& $Tests --verify-manifest $out ('0'*128) 1.0.4
 check (($wrongKey -join ' ') -like 'error:*') 'A build without a key, or with another key, rejects the manifest'
 $otherName='UDM updater test other '+[guid]::NewGuid().ToString('N')
 $other=& (Join-Path $root 'tools\update-key.ps1') -Create -Name $otherName -Header (Join-Path $RunRoot 'other.hpp')
 $otherHex=($other | Where-Object { $_ -like 'public key: *' }) -replace '^public key: ',''
 try{$otherResult=& $Tests --verify-manifest $out $otherHex 1.0.4}finally{[System.Security.Cryptography.CngKey]::Open($otherName).Delete()}
 check (($otherResult -join ' ') -like 'error:*signature*') 'A manifest signed by another key is rejected natively'
 $duplicate=$false;try{& (Join-Path $root 'tools\update-key.ps1') -Create -Name $keyName -Header (Join-Path $RunRoot 'dup.hpp') | Out-Null}catch{$duplicate=$true}
 check $duplicate 'Creating a key that already exists is refused'
 $result=0
}catch{$failure=$_.Exception.Message}
finally{
 if([System.Security.Cryptography.CngKey]::Exists($keyName)){[System.Security.Cryptography.CngKey]::Open($keyName).Delete()}
 $gone=-not [System.Security.Cryptography.CngKey]::Exists($keyName)
 $checks.Add([pscustomobject]@{name='The temporary key is deleted from the store';passed=$gone});if(-not $gone){$result=1}
}
[pscustomobject]@{passed=($result -eq 0);error=$failure;checks=$checks} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $RunRoot 'results.json') -Encoding UTF8
$checks | ForEach-Object { Write-Output (($(if($_.passed){'PASS'}else{'FAIL'}))+' '+$_.name) }
Write-Output ("$(@($checks | Where-Object passed).Count) passed, $(@($checks | Where-Object { -not $_.passed }).Count) failed")
exit $result
