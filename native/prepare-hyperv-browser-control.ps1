param([string]$LabRoot='D:\UDM-Lab')
$ErrorActionPreference='Stop'
$owner=Get-Content -LiteralPath (Join-Path $LabRoot 'provisioning.json') -Raw | ConvertFrom-Json
if($owner.vmId -ne '86ccf599-1723-4d50-b37b-d5414b1b8a54'){throw 'Unexpected lab.'}
$credential=Import-Clixml -LiteralPath (Join-Path $env:LOCALAPPDATA 'UDM-Development\hyperv-lab-credential.clixml')
$session=New-PSSession -VMId $owner.vmId -Credential $credential
try {
 $result=Invoke-Command -Session $session -ScriptBlock {
  if($env:COMPUTERNAME -ne 'UDM-LAB'){throw 'Wrong guest.'}
  $chrome='C:\UDM-Lab\browser\chrome-win64\chrome.exe'
  if(!(Test-Path -LiteralPath $chrome)){throw 'Lab browser is not installed.'}
  # A separate functional profile avoids depending on a visible desktop for browser tests.
  # Use the same browser files; do not copy binaries or any personal profile.
  $arguments='--headless=new --user-data-dir=C:\UDM-Lab\profiles\functional --remote-debugging-port=9224 --remote-debugging-address=127.0.0.1 --enable-unsafe-extension-debugging --load-extension="C:\Program Files\UDM\browser\chromium" --no-first-run --no-default-browser-check file:///C:/UDM-Lab/fixtures/control.html'
  $action=New-ScheduledTaskAction -Execute $chrome -Argument $arguments
  $principal=New-ScheduledTaskPrincipal -UserId 'UDM-LAB\UDMWorker' -LogonType Interactive -RunLevel Limited
  $settings=New-ScheduledTaskSettingsSet -ExecutionTimeLimit (New-TimeSpan -Minutes 30)
  Register-ScheduledTask -TaskName 'UDM-Lab Functional Browser' -Action $action -Principal $principal -Settings $settings -Force | Out-Null
  Start-ScheduledTask -TaskName 'UDM-Lab Functional Browser'
  [pscustomobject]@{prepared=$true;task='UDM-Lab Functional Browser';separateProfile=$true;sameBrowserBinaries=$true;hostPortExposed=$false;guestLoopbackPort=9224;runOnDemand=$true;executionLimitMinutes=30}
 }
 $result | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $LabRoot 'functional-browser-preparation.json') -Encoding UTF8
 $result | ConvertTo-Json
} finally {Remove-PSSession $session}
