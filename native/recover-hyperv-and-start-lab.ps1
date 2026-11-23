param([switch]$TryBootMediaCleanup,[string]$LabRoot='D:\UDM-Lab')
$ErrorActionPreference='Stop'
if($PSVersionTable.PSEdition -ne 'Desktop'){throw 'Use Windows PowerShell.'}
$identity=[Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()
if(!$identity.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)){throw 'This bounded management recovery requires elevation.'}
$record=[ordered]@{vmId='86ccf599-1723-4d50-b37b-d5414b1b8a54';hostRestarted=$false;vmWorkersTerminated=$false;cleanupAttempted=[bool]$TryBootMediaCleanup;cleanupCompleted=$false;labStarted=$false}
function Recover-LabManagement {
 if((Get-Service vmms).Status -ne 'StopPending'){& sc.exe stop vmms | Out-Null;Start-Sleep -Seconds 1}
 & (Join-Path $PSScriptRoot 'repair-hyperv-management.ps1') -LabRoot $LabRoot
}
try {
 Recover-LabManagement
 if($TryBootMediaCleanup){
  $cleanup=Join-Path $PSScriptRoot 'cleanup-hyperv-boot-media.ps1'
  $process=Start-Process -FilePath "$env:WINDIR\System32\WindowsPowerShell\v1.0\powershell.exe" -ArgumentList @('-NoProfile','-NonInteractive','-ExecutionPolicy','Bypass','-File',$cleanup,'-Apply') -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $LabRoot 'boot-cleanup-output.txt') -RedirectStandardError (Join-Path $LabRoot 'boot-cleanup-error.txt')
  if(!$process.WaitForExit(60000)){
   Stop-Process -Id $process.Id -ErrorAction SilentlyContinue
   $record.cleanupTimedOut=$true
   Recover-LabManagement
  } else {
   $process.Refresh()
   $record.cleanupExitCode=$process.ExitCode
   $record.cleanupCompleted=Test-Path -LiteralPath (Join-Path $LabRoot 'boot-media-cleanup.json')
  }
 }
 $vm=Get-VM -Id $record.vmId
 if($vm.State -eq 'Off'){Start-VM -VM $vm}
 $record.labStarted=((Get-VM -Id $record.vmId).State -eq 'Running')
} catch {$record.error=$_.Exception.Message}
$record.completedUtc=[DateTime]::UtcNow.ToString('o')
$record | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $LabRoot 'bounded-recovery.json') -Encoding UTF8
if(!$record.labStarted){exit 1}
