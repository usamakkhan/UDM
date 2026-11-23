param([string]$LabRoot='D:\UDM-Lab')
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath($LabRoot)
$access=Get-Content -LiteralPath (Join-Path $root 'guest-access.json') -Raw | ConvertFrom-Json
$apps=Get-Content -LiteralPath (Join-Path $root 'apps-installed.json') -Raw | ConvertFrom-Json
$browser=Get-Content -LiteralPath (Join-Path $root 'browser-control.json') -Raw | ConvertFrom-Json
if($access.vmId -ne '86ccf599-1723-4d50-b37b-d5414b1b8a54' -or !$access.guestAdministratorVerified -or !$apps.passed -or !$browser.passed){throw 'Guest, app and browser acceptance are required before setup cleanup.'}
$credential=Import-Clixml -LiteralPath (Join-Path $env:LOCALAPPDATA 'UDM-Development\hyperv-lab-credential.clixml')
$session=New-PSSession -VMId $access.vmId -Credential $credential
try {
 $guest=Invoke-Command -Session $session -ScriptBlock {
  $ErrorActionPreference='Stop'
  if($env:COMPUTERNAME -ne 'UDM-LAB'){throw 'Wrong guest.'}
  $key=Get-ItemProperty -LiteralPath 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon'
  if($key.AutoAdminLogon -ne '0' -or $null -ne $key.PSObject.Properties['DefaultPassword']){throw 'Wait for one-time sign-in cleanup before removing setup files.'}
  $removed=@()
  foreach($folder in @('C:\UDM-Lab\setup','C:\Users\UDMWorker\AppData\Local\Temp\IDM_Setup_Temp')){
   $full=[IO.Path]::GetFullPath($folder)
   if($full -notin @('C:\UDM-Lab\setup','C:\Users\UDMWorker\AppData\Local\Temp\IDM_Setup_Temp')){throw 'Unexpected guest cleanup scope.'}
   if(!(Test-Path -LiteralPath $full)){continue}
   $item=Get-Item -LiteralPath $full
   $parent=$item
   while($null -ne $parent){if($parent.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Guest cleanup path redirects.'};$parent=$parent.Parent}
   $children=@(Get-ChildItem -LiteralPath $full -Recurse -Force)
   if($children | Where-Object {$_.Attributes -band [IO.FileAttributes]::ReparsePoint}){throw 'Guest staging content redirects.'}
   $bytes=($children | Where-Object {!$_.PSIsContainer} | Measure-Object Length -Sum).Sum
   Remove-Item -LiteralPath $full -Recurse -Force
   $removed += [pscustomobject]@{path=$full;logicalBytes=[long]$bytes}
  }
  foreach($file in @('C:\Windows\Panther\unattend.xml','C:\Windows\Panther\Unattend\unattend.xml')){
   if(!(Test-Path -LiteralPath $file)){continue}
   [xml]$xml=Get-Content -LiteralPath $file -Raw
   if($null -ne $xml.SelectSingleNode("//*[local-name()='LocalAccount']/*[local-name()='Name' and text()='UDMWorker']")){
    $item=Get-Item -LiteralPath $file
    if($item.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Cached answer redirects.'}
    Remove-Item -LiteralPath $file
    $removed += [pscustomobject]@{path=$file;logicalBytes=$item.Length}
   }
  }
  foreach($name in @('UDM-Lab Finish IDM Setup','UDM-Lab Clear One-Time Sign-In')){
   $task=Get-ScheduledTask -TaskName $name -ErrorAction SilentlyContinue
   if($task -and $task.State -ne 'Running'){Unregister-ScheduledTask -TaskName $name -Confirm:$false}
  }
  $task=Get-ScheduledTask -TaskName 'UDM-Lab Browser'
  $shell=New-Object -ComObject WScript.Shell
  $link=$shell.CreateShortcut('C:\Users\Public\Desktop\Chrome Lab.lnk')
  $link.TargetPath=$task.Actions[0].Execute;$link.Arguments=$task.Actions[0].Arguments;$link.WorkingDirectory=Split-Path $link.TargetPath;$link.Save()
  $record=[ordered]@{passed=$true;temporaryGuestSetupRemoved=$true;removed=$removed;automaticSignInDisabled=$true;defaultPasswordAbsent=$true;browserShortcutUsesIsolatedProfile=$true;virtualDiskCompacted=$false;completedUtc=[DateTime]::UtcNow.ToString('o')}
  $record | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath 'C:\UDM-Lab\records\setup-cleanup.json' -Encoding UTF8
  [pscustomobject]$record
 }
} finally {Remove-PSSession $session}
$removedHost=@()
$hostFiles=@(
 (Join-Path $root 'bootstrap\Autounattend.xml'),
 (Join-Path $root 'staging\idman643build18.exe'),
 (Join-Path $root 'staging\chrome-win64.zip'),
 (Join-Path $root 'staging\chrome-signature-check.exe'),
 (Join-Path $root 'staging\udm-chromium.zip')
)
foreach($file in $hostFiles){
 $full=[IO.Path]::GetFullPath($file)
 if(!$full.StartsWith($root+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Host cleanup escaped the owned lab.'}
 if(!(Test-Path -LiteralPath $full)){continue}
 $item=Get-Item -LiteralPath $full
 if($item.PSIsContainer -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)){throw 'Unexpected host cleanup target.'}
 $parent=$item.Directory
 while($null -ne $parent){if($parent.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Host cleanup ancestor redirects.'};$parent=$parent.Parent}
 Remove-Item -LiteralPath $full
 $removedHost += [pscustomobject]@{path=$full;bytes=$item.Length}
}
$retained=@('media\Windows11-Enterprise-LTSC-2024-eval-x64.iso','bootstrap\answer.iso') | ForEach-Object { $path=Join-Path $root $_; if(Test-Path -LiteralPath $path){[pscustomobject]@{path=$path;bytes=(Get-Item -LiteralPath $path).Length;reason='Still attached: host Hyper-V DVD-device removal hangs, including offline elevated attempts.'}} }
$record=[ordered]@{vmId=$access.vmId;applicationSetupCleanupPassed=$true;guest=$guest;removedHost=$removedHost;retainedAttachedMedia=@($retained);allSetupFilesRemoved=(@($retained).Count -eq 0);encryptedCredentialExportRetained=$true;completedUtc=[DateTime]::UtcNow.ToString('o')}
$record | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $root 'setup-cleanup.json') -Encoding UTF8
$record | ConvertTo-Json -Depth 8
