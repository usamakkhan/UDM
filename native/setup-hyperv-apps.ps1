param([string]$LabRoot='D:\UDM-Lab',[switch]$ResumeStaging)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath($LabRoot)
$owner=Get-Content -LiteralPath (Join-Path $root 'provisioning.json') -Raw | ConvertFrom-Json
if($owner.vmId -ne '86ccf599-1723-4d50-b37b-d5414b1b8a54'){throw 'Unexpected lab identity.'}
$credential=Import-Clixml -LiteralPath (Join-Path $env:LOCALAPPDATA 'UDM-Development\hyperv-lab-credential.clixml')
$build=Get-Content -LiteralPath (Join-Path $PSScriptRoot '..\.media-cache\owned-completion-modal-build-9.88\build-result.json') -Raw | ConvertFrom-Json
if(!$build.passed -or $build.version -ne '9.88'){throw 'Unexpected native build.'}
$products=@($build.outputs | Where-Object kind -eq product)
foreach($product in $products){if((Get-FileHash -LiteralPath $product.path).Hash -ne $product.sha256){throw 'Native product hash changed.'}}
$session=New-PSSession -VMId $owner.vmId -Credential $credential
try {
 Invoke-Command -Session $session -ArgumentList ([bool]$ResumeStaging) -ScriptBlock {
  param([bool]$Resume)
  if($env:COMPUTERNAME -ne 'UDM-LAB'){throw 'Wrong guest.'}
  if((Test-Path 'C:\UDM-Lab\setup') -and !$Resume){throw 'Guest setup folder exists; inspect before retry.'}
  if(Test-Path 'C:\UDM-Lab\records\apps-installed.json'){throw 'An installed application record exists; inspect before repeating setup.'}
  New-Item -ItemType Directory -Path 'C:\UDM-Lab\setup\native','C:\UDM-Lab\records' -Force | Out-Null
 }
 Write-Output 'Copying verified application payloads into the owned guest.'
 $repo=Split-Path $PSScriptRoot -Parent
 $payloads=@(
  @((Join-Path $repo 'installer-out\UDM-9.67-Browser-0.73.31-Setup-x64.exe'),'C:\UDM-Lab\setup\udm-base.exe'),
  @((Join-Path $root 'staging\idman643build18.exe'),'C:\UDM-Lab\setup\idm-setup.exe'),
  @((Join-Path $root 'staging\chrome-win64.zip'),'C:\UDM-Lab\setup\chrome-win64.zip')
 )
 foreach($product in $products){$payloads+=,@($product.path,('C:\UDM-Lab\setup\native\'+$product.name))}
 Enable-VMIntegrationService -VMName 'UDM-Lab' -Name 'Guest Service Interface'
 foreach($payload in $payloads){
  Copy-VMFile -Name 'UDM-Lab' -SourcePath $payload[0] -DestinationPath $payload[1] -FileSource Host -CreateFullPath -Force -ErrorAction Stop
  $expected=(Get-FileHash -LiteralPath $payload[0]).Hash
  $actual=Invoke-Command -Session $session -ArgumentList $payload[1] -ScriptBlock {param($Path) (Get-FileHash -LiteralPath $Path).Hash}
  if($actual -ne $expected){throw 'Guest payload hash mismatch.'}
  Write-Output ('Transferred and verified '+[IO.Path]::GetFileName($payload[1]))
 }
 $extensionArchive=Join-Path $root 'staging\udm-chromium.zip'
 if(!(Test-Path -LiteralPath $extensionArchive)){
  Add-Type -AssemblyName System.IO.Compression.FileSystem
  [IO.Compression.ZipFile]::CreateFromDirectory((Join-Path $repo 'browser\chromium'),$extensionArchive)
 }
 Copy-VMFile -Name 'UDM-Lab' -SourcePath $extensionArchive -DestinationPath 'C:\UDM-Lab\setup\udm-chromium.zip' -FileSource Host -Force -ErrorAction Stop
 $expectedExtensionHash=(Get-FileHash -LiteralPath $extensionArchive).Hash
 $record=Invoke-Command -Session $session -ArgumentList $products,$build.extensionVersion,$expectedExtensionHash -ScriptBlock {
  param($Products,$ExtensionVersion,$ExpectedExtensionHash)
  $ErrorActionPreference='Stop'
  $setup='C:\UDM-Lab\setup'
  $udm='C:\Program Files\UDM'
  if((Get-FileHash -LiteralPath ($setup+'\udm-chromium.zip')).Hash -ne $ExpectedExtensionHash){throw 'Extension archive hash mismatch.'}
  Add-Type -AssemblyName System.IO.Compression.FileSystem
  $stagedExtension=$setup+'\chromium'
  if(Test-Path -LiteralPath $stagedExtension){
   $item=Get-Item -LiteralPath $stagedExtension
   if($item.FullName -ne 'C:\UDM-Lab\setup\chromium' -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)){throw 'Unexpected staging folder.'}
   if(Get-ChildItem -LiteralPath $stagedExtension -Recurse -Force | Where-Object {$_.Attributes -band [IO.FileAttributes]::ReparsePoint}){throw 'Redirected staging content.'}
   Remove-Item -LiteralPath $stagedExtension -Recurse -Force
  }
  [IO.Compression.ZipFile]::ExtractToDirectory(($setup+'\udm-chromium.zip'),$stagedExtension)
  function Run-Setup([string]$File,[string]$Arguments){
   $process=Start-Process -FilePath $File -ArgumentList $Arguments -PassThru -WindowStyle Hidden
   if(!$process.WaitForExit(180000)){throw ('Setup exceeded three minutes: '+[IO.Path]::GetFileName($File))}
   if($process.ExitCode -ne 0){throw ('Setup failed: '+[IO.Path]::GetFileName($File)+' exit '+$process.ExitCode)}
  }
  Run-Setup ($setup+'\udm-base.exe') '/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /SP- /LOG="C:\UDM-Lab\records\udm-base-install.log"'
  foreach($product in $Products){
   $file=Join-Path $setup ('native\'+$product.name)
   if((Get-FileHash -LiteralPath $file).Hash -ne $product.sha256){throw 'Guest payload verification failed.'}
   Copy-Item -LiteralPath $file -Destination (Join-Path $udm $product.name) -Force
  }
  Copy-Item -Path ($setup+'\chromium\*') -Destination ($udm+'\browser\chromium') -Recurse -Force
  if((Get-Content -LiteralPath ($udm+'\browser\chromium\manifest.json') -Raw | ConvertFrom-Json).version -ne $ExtensionVersion){throw 'Extension version mismatch.'}
  # This is a documented lab overlay, not a newly published release installer.
  New-Item -Path 'HKLM:\SOFTWARE\UDM-Lab' -Force | Out-Null
  New-ItemProperty -Path 'HKLM:\SOFTWARE\UDM-Lab' -Name NativeOverlayVersion -Value '9.88' -Force | Out-Null
  $idmInstaller=Get-AuthenticodeSignature -LiteralPath ($setup+'\idm-setup.exe')
  if($idmInstaller.Status -ne 'Valid' -or $idmInstaller.SignerCertificate.Subject -notmatch 'CN=Tonec Inc\.'){throw 'IDM publisher check failed.'}
  $nativeRows=@($Products | ForEach-Object {[pscustomobject]@{name=$_.name;sha256=(Get-FileHash -LiteralPath (Join-Path $udm $_.name)).Hash;version=(Get-Item -LiteralPath (Join-Path $udm $_.name)).VersionInfo.ProductVersion}})
  $result=[ordered]@{nativeOverlayPrepared=$true;nativeVersion='9.88';extensionVersion=$ExtensionVersion;native=$nativeRows;personalProfilesCopied=$false;setupFilesRemoved=$false}
  $result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath 'C:\UDM-Lab\records\native-overlay.json' -Encoding UTF8
  [pscustomobject]$result
 }
 $record | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath (Join-Path $root 'native-overlay.json') -Encoding UTF8
 $record | ConvertTo-Json -Depth 7
} finally {Remove-PSSession $session}
$browserSignatureRecord=Join-Path $root 'staging\browser-signature.json'
if(!(Test-Path -LiteralPath $browserSignatureRecord)){
 Add-Type -AssemblyName System.IO.Compression.FileSystem
 $archive=[IO.Compression.ZipFile]::OpenRead((Join-Path $root 'staging\chrome-win64.zip'))
 $checkPath=Join-Path $root 'staging\chrome-signature-check.exe'
 try {
  if(Test-Path -LiteralPath $checkPath){throw 'Preserve existing browser signature fixture.'}
  $entry=$archive.GetEntry('chrome-win64/chrome.exe')
  if(!$entry){throw 'Browser executable absent in the downloaded archive.'}
  [IO.Compression.ZipFileExtensions]::ExtractToFile($entry,$checkPath)
 } finally {$archive.Dispose()}
 $signature=Get-AuthenticodeSignature -LiteralPath $checkPath
 [ordered]@{status=[string]$signature.Status;publisher=$signature.SignerCertificate.Subject;sha256=(Get-FileHash -LiteralPath $checkPath).Hash;length=(Get-Item -LiteralPath $checkPath).Length} | ConvertTo-Json | Set-Content -LiteralPath $browserSignatureRecord -Encoding UTF8
}
& (Join-Path $PSScriptRoot 'finish-hyperv-apps.ps1') -LabRoot $LabRoot
