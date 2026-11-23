param([string]$LabRoot='D:\UDM-Lab')
$ErrorActionPreference='Stop'
$owner=Get-Content -LiteralPath (Join-Path $LabRoot 'provisioning.json') -Raw | ConvertFrom-Json
if($owner.vmId -ne '86ccf599-1723-4d50-b37b-d5414b1b8a54'){throw 'Unexpected lab.'}
$credential=Import-Clixml -LiteralPath (Join-Path $env:LOCALAPPDATA 'UDM-Development\hyperv-lab-credential.clixml')
$session=New-PSSession -VMId $owner.vmId -Credential $credential
try {
 $result=Invoke-Command -Session $session -ArgumentList $credential -ScriptBlock {
  param([pscredential]$GuestCredential)
  $ErrorActionPreference='Stop'
  if($env:COMPUTERNAME -ne 'UDM-LAB'){throw 'Wrong guest.'}
  $installed=Get-Content -LiteralPath 'C:\UDM-Lab\records\apps-installed.json' -Raw | ConvertFrom-Json
  if(!$installed.desktopReady -and !$installed.passed){throw 'Desktop application preparation is incomplete.'}
  $fixture='C:\UDM-Lab\fixtures'
  New-Item -ItemType Directory -Path $fixture,'C:\UDM-Lab\profiles' -Force | Out-Null
  @'
<!doctype html><meta charset="utf-8"><title>UDM-Lab browser control</title>
<style>body{font:22px Segoe UI;margin:60px;max-width:800px}button{font:inherit;padding:15px}#result{margin-top:25px}</style>
<h1>UDM-Lab browser control</h1><p>This local page contains no personal data or external requests.</p>
<button id="verify" onclick="document.querySelector('#result').textContent='Browser input verified';document.body.dataset.verified='true'">Verify browser control</button>
<p id="result">Waiting for browser input</p>
'@ | Set-Content -LiteralPath ($fixture+'\control.html') -Encoding UTF8
  $chrome='C:\UDM-Lab\browser\chrome-win64\chrome.exe'
  $arguments='--user-data-dir=C:\UDM-Lab\profiles\chromium --remote-debugging-port=9222 --remote-debugging-address=127.0.0.1 --enable-unsafe-extension-debugging --load-extension="C:\Program Files\UDM\browser\chromium" --no-first-run --no-default-browser-check file:///C:/UDM-Lab/fixtures/control.html'
  $action=New-ScheduledTaskAction -Execute $chrome -Argument $arguments
  $trigger=New-ScheduledTaskTrigger -AtLogOn -User 'UDM-LAB\UDMWorker'
  $principal=New-ScheduledTaskPrincipal -UserId 'UDM-LAB\UDMWorker' -LogonType Interactive -RunLevel Limited
  Register-ScheduledTask -TaskName 'UDM-Lab Browser' -Action $action -Trigger $trigger -Principal $principal -Force | Out-Null
  $cleanup='C:\UDM-Lab\clear-one-time-signin.ps1'
  @'
$ErrorActionPreference='Stop'
$key='HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon'
Set-ItemProperty -LiteralPath $key -Name AutoAdminLogon -Value '0'
Remove-ItemProperty -LiteralPath $key -Name DefaultPassword -ErrorAction SilentlyContinue
Remove-ItemProperty -LiteralPath $key -Name AutoLogonCount -ErrorAction SilentlyContinue
Unregister-ScheduledTask -TaskName 'UDM-Lab Clear One-Time Sign-In' -Confirm:$false
[ordered]@{automaticSignInDisabled=$true;defaultPasswordRemoved=$true;completedUtc=[DateTime]::UtcNow.ToString('o')} | ConvertTo-Json | Set-Content -LiteralPath 'C:\UDM-Lab\records\one-time-signin-cleanup.json' -Encoding UTF8
Remove-Item -LiteralPath $PSCommandPath
'@ | Set-Content -LiteralPath $cleanup -Encoding UTF8
  $cleanupAction=New-ScheduledTaskAction -Execute 'powershell.exe' -Argument ('-NoProfile -ExecutionPolicy Bypass -File "'+$cleanup+'"')
  $cleanupPrincipal=New-ScheduledTaskPrincipal -UserId 'SYSTEM' -LogonType ServiceAccount -RunLevel Highest
  Register-ScheduledTask -TaskName 'UDM-Lab Clear One-Time Sign-In' -Action $cleanupAction -Trigger $trigger -Principal $cleanupPrincipal -Force | Out-Null
  $key='HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon'
  New-ItemProperty -LiteralPath $key -Name DefaultUserName -Value 'UDMWorker' -PropertyType String -Force | Out-Null
  New-ItemProperty -LiteralPath $key -Name DefaultDomainName -Value 'UDM-LAB' -PropertyType String -Force | Out-Null
  New-ItemProperty -LiteralPath $key -Name DefaultPassword -Value ($GuestCredential.GetNetworkCredential().Password) -PropertyType String -Force | Out-Null
  New-ItemProperty -LiteralPath $key -Name AutoLogonCount -Value 1 -PropertyType DWord -Force | Out-Null
  New-ItemProperty -LiteralPath $key -Name AutoAdminLogon -Value '1' -PropertyType String -Force | Out-Null
  # Only the new, synthetic lab session is being restarted. No host credentials are used.
  [pscustomobject]@{prepared=$true;oneTimeAutomaticSignIn=$true;cleanupTaskRegistered=$true;browserTaskRegistered=$true;guestShutdownRequested=$false}
 }
 $result | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $LabRoot 'desktop-preparation.json') -Encoding UTF8
 $result | ConvertTo-Json
} finally {Remove-PSSession $session}
