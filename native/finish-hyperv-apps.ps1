param([string]$LabRoot='D:\UDM-Lab')
$ErrorActionPreference='Stop'
$credential=Import-Clixml -LiteralPath (Join-Path $env:LOCALAPPDATA 'UDM-Development\hyperv-lab-credential.clixml')
$build=Get-Content -LiteralPath (Join-Path $PSScriptRoot '..\.media-cache\owned-completion-modal-build-9.88\build-result.json') -Raw | ConvertFrom-Json
$products=@($build.outputs | Where-Object kind -eq product)
$browserSource=Get-Content -LiteralPath (Join-Path $LabRoot 'staging\browser-source.json') -Raw | ConvertFrom-Json
$browserCheck=Get-Content -LiteralPath (Join-Path $LabRoot 'staging\browser-signature.json') -Raw | ConvertFrom-Json
if($browserSource.url -notlike 'https://storage.googleapis.com/chrome-for-testing-public/*/win64/chrome-win64.zip' -or $browserCheck.sha256 -notmatch '^[A-F0-9]{64}$'){throw 'Missing official-distribution browser provenance.'}
$session=New-PSSession -VMId '86ccf599-1723-4d50-b37b-d5414b1b8a54' -Credential $credential
try {
 $result=Invoke-Command -Session $session -ArgumentList $products,$browserCheck.sha256 -ScriptBlock {
  param($Products,$ExpectedBrowserHash)
  $ErrorActionPreference='Stop'
  if($env:COMPUTERNAME -ne 'UDM-LAB'){throw 'Wrong guest.'}
  $udm='C:\Program Files\UDM'
  if(@($Products).Count -ne 5){throw 'Expected all five native products.'}
  $rows=@()
  foreach($product in $Products){
   $file=Join-Path $udm $product.name
   $hash=(Get-FileHash -LiteralPath $file).Hash
   if($hash -ne $product.sha256){throw 'Installed native product mismatch.'}
   $rows += [pscustomobject]@{name=$product.name;version=(Get-Item -LiteralPath $file).VersionInfo.ProductVersion;sha256=$hash}
  }
  $extension=(Get-Content -LiteralPath ($udm+'\browser\chromium\manifest.json') -Raw | ConvertFrom-Json).version
  if($extension -ne '0.73.52'){throw 'Unexpected extension version.'}
  $chrome='C:\UDM-Lab\browser\chrome-win64\chrome.exe'
  if(!(Test-Path -LiteralPath $chrome)){
   Add-Type -AssemblyName System.IO.Compression.FileSystem
   [IO.Compression.ZipFile]::ExtractToDirectory('C:\UDM-Lab\setup\chrome-win64.zip','C:\UDM-Lab\browser')
  }
  $signature=Get-AuthenticodeSignature -LiteralPath $chrome
  $browserHash=(Get-FileHash -LiteralPath $chrome).Hash
  if($browserHash -ne $ExpectedBrowserHash){throw 'Browser differs from the executable in the official downloaded archive.'}
  # Chrome for Testing's Windows binary is unsigned. Record that fact; do not label it signature-verified.
  if($signature.Status -ne 'NotSigned' -and !($signature.Status -eq 'Valid' -and $signature.SignerCertificate.Subject -match 'CN=Google LLC')){throw 'Unexpected browser signature state.'}
  $idm=@('C:\Program Files\Internet Download Manager\IDMan.exe','C:\Program Files (x86)\Internet Download Manager\IDMan.exe') | Where-Object {Test-Path -LiteralPath $_} | Select-Object -First 1
  if(!$idm){
   # The bootstrapper can return before its child finishes. Inspect pending setup before retrying interactively.
   foreach($process in @(Get-CimInstance Win32_Process | Where-Object {$_.Name -match '^IDM\d+\.tmp$' -and $_.SessionId -eq 0 -and $_.ExecutablePath -like 'C:\Users\UDMWorker\AppData\Local\Temp\IDM_Setup_Temp\*'})){
    Stop-Process -Id $process.ProcessId -ErrorAction Stop
   }
   $action=New-ScheduledTaskAction -Execute 'C:\UDM-Lab\setup\idm-setup.exe' -Argument '/skipdlgs'
   $trigger=New-ScheduledTaskTrigger -AtLogOn -User 'UDM-LAB\UDMWorker'
   $principal=New-ScheduledTaskPrincipal -UserId 'UDM-LAB\UDMWorker' -LogonType Interactive -RunLevel Highest
   Register-ScheduledTask -TaskName 'UDM-Lab Finish IDM Setup' -Action $action -Trigger $trigger -Principal $principal -Force | Out-Null
  }
  $shell=New-Object -ComObject WScript.Shell
  $entries=@(@('UDM Lab',($udm+'\UDM.exe')),@('Chrome Lab',$chrome))
  if($idm){$entries+=,@('IDM Lab',$idm)}
  foreach($entry in $entries){$link=$shell.CreateShortcut(('C:\Users\Public\Desktop\'+$entry[0]+'.lnk'));$link.TargetPath=$entry[1];$link.WorkingDirectory=Split-Path $entry[1];$link.Save()}
  $result=[ordered]@{passed=[bool]$idm;desktopReady=$true;nativeVersion='9.88';extensionVersion=$extension;native=$rows;udmInstallation='9.67 installer plus hash-verified 9.88 native and 0.73.52 Chromium lab overlay';browserVersion=(Get-Item -LiteralPath $chrome).VersionInfo.ProductVersion;browserSignatureStatus=[string]$signature.Status;browserSha256=$browserHash;browserMatchesOfficialArchive=$true;idmInstalled=[bool]$idm;idmInstallPendingInteractiveSession=(!$idm);personalProfilesCopied=$false;setupFilesRemoved=$false}
  if($idm){$result.idmPath=$idm;$result.idmVersion=(Get-Item -LiteralPath $idm).VersionInfo.ProductVersion;$result.idmSignatureStatus=[string](Get-AuthenticodeSignature -LiteralPath $idm).Status}
  $result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath 'C:\UDM-Lab\records\apps-installed.json' -Encoding UTF8
  [pscustomobject]$result
 }
 $result | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath (Join-Path $LabRoot 'apps-installed.json') -Encoding UTF8
 $result | ConvertTo-Json -Depth 7
} finally {Remove-PSSession $session}
