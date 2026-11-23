param([string]$LabRoot='D:\UDM-Lab',[int]$ReserveGiB=8)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath($LabRoot)
$owner=Get-Content -LiteralPath (Join-Path $root 'provisioning.json') -Raw | ConvertFrom-Json
$vmId=[guid]$owner.vmId
$driveName=[IO.Path]::GetPathRoot($root).Substring(0,1)
$started=[DateTime]::UtcNow;$minimum=[long]::MaxValue
while($true){
 $vm=Get-VM -Id $vmId -ErrorAction Stop
 $free=(Get-PSDrive -Name $driveName).Free;$minimum=[math]::Min($minimum,$free)
 $record=[ordered]@{vmId=[string]$vmId;state='watching';vmState=[string]$vm.State;freeGiB=[math]::Round($free/1GB,2);minimumFreeGiB=[math]::Round($minimum/1GB,2);vhdBytes=(Get-Item -LiteralPath $owner.diskPath).Length;startedUtc=$started.ToString('o');updatedUtc=[DateTime]::UtcNow.ToString('o')}
 if($free -lt $ReserveGiB*1GB){
  $disks=@(Get-VMHardDiskDrive -VM $vm)
  if($disks.Count -ne 1 -or [IO.Path]::GetFullPath($disks[0].Path) -ne [IO.Path]::GetFullPath($owner.diskPath)){throw 'Disk attachments changed; refuse automatic power control.'}
  if($vm.State -eq 'Running'){$vm | Stop-VM -TurnOff -Force -Confirm:$false}
  $record.state='stopped-owned-install-for-host-space';$record.guestInstallationMayBeIncomplete=$true
 } elseif(Test-Path -LiteralPath (Join-Path $root 'guest-access.json')){
  $access=Get-Content -LiteralPath (Join-Path $root 'guest-access.json') -Raw | ConvertFrom-Json
  if($access.vmId -eq [string]$vmId -and $access.guestAdministratorVerified){$record.state='guest-access-verified'}
 } elseif($vm.State -eq 'Off'){$record.state='vm-off'}
 $record | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $root 'storage-guard.json') -Encoding UTF8
 if($record.state -ne 'watching'){$record|ConvertTo-Json;break}
 Start-Sleep -Seconds 3
}
