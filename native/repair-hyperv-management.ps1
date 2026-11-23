param([string]$LabRoot='D:\UDM-Lab')
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath($LabRoot)
$owner=Get-Content -LiteralPath (Join-Path $root 'provisioning.json') -Raw | ConvertFrom-Json
if($owner.vmId -ne '86ccf599-1723-4d50-b37b-d5414b1b8a54'){throw 'Unexpected lab identity.'}
$record=[ordered]@{vmId=$owner.vmId;scope='Recover only the stuck Hyper-V management service';hostRestarted=$false;vmWorkerProcessesStopped=$false;passed=$false}
try {
 $service=Get-Service -Name vmms
 if($service.Status -ne 'StopPending'){throw 'Service is not in the observed stuck StopPending state; inspect before changing it.'}
 $query=(& sc.exe queryex vmms | Out-String)
 if($query -notmatch 'PID\s*:\s*(\d+)'){throw 'Could not identify the management service process.'}
 $managementId=[int]$Matches[1]
 $process=Get-Process -Id $managementId
 if($process.ProcessName -ne 'vmms'){throw 'Service process identity mismatch.'}
 Stop-Process -Id $managementId -Force -ErrorAction Stop
 $record.stoppedManagementProcessId=$managementId
 Start-Sleep -Seconds 2
 if((Get-Service vmms).Status -ne 'Running'){Start-Service vmms}
 $deadline=[DateTime]::UtcNow.AddSeconds(25)
 do {
  $status=(Get-Service vmms).Status
  if($status -eq 'Running'){break}
  Start-Sleep -Milliseconds 500
 } while([DateTime]::UtcNow -lt $deadline)
 if($status -ne 'Running'){throw 'Management service did not return to Running within the bounded wait.'}
 $record.passed=$true
 $record.serviceStatus=[string]$status
} catch {$record.error=$_.Exception.Message}
$record.completedUtc=[DateTime]::UtcNow.ToString('o')
$record | ConvertTo-Json -Compress | Add-Content -LiteralPath (Join-Path $root 'management-recovery-history.jsonl') -Encoding UTF8
$record | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $root 'management-recovery.json') -Encoding UTF8
if(!$record.passed){exit 1}
