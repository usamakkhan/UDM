param(
 [string]$Name='UDM-Lab',
 [string]$LabRoot='D:\UDM-Lab',
 [switch]$Create
)
$ErrorActionPreference='Stop'
Import-Module Hyper-V -ErrorAction Stop
$root=[IO.Path]::GetFullPath($LabRoot).TrimEnd('\')
if($Name -notmatch '^[A-Za-z0-9_-]{1,40}$'){throw 'Use a simple VM name.'}
$existing=@(Get-VM -ErrorAction Stop)
if(@($existing | Where-Object Name -eq $Name).Count){throw 'That VM already exists; inspect it before retrying.'}
if(Test-Path -LiteralPath $root){throw 'Use a new lab folder; existing lab files are preserved.'}
$parent=Get-Item -LiteralPath (Split-Path $root -Parent)
while($null -ne $parent){if($parent.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Lab ancestors must not redirect.'};$parent=$parent.Parent}
$drive=Get-PSDrive -Name ([IO.Path]::GetPathRoot($root).Substring(0,1)) -ErrorAction Stop
$switch=Get-VMSwitch -Name 'Default Switch' -ErrorAction Stop
$plan=[ordered]@{
 name=$Name;root=$root;guestOS='Windows 11 Enterprise LTSC 2024 evaluation';evaluationDays=90
 generation=2;virtualDiskGiB=64;dynamicallyExpanding=$true;startupRamGiB=4;minimumRamGiB=2;maximumRamGiB=6;virtualProcessors=2
 secureBoot=$true;virtualTpm=$true;automaticCheckpoints=$false;startRequested=$false
 hostFreeGiB=[math]::Round($drive.Free/1GB,2);created=$false;guestInstalled=$false;guestAdminConfigured=$false
 fullDiskCapacityNotReserved=$true;hostPermissionsChanged=$false
}
if(!$Create){$plan | ConvertTo-Json -Depth 5;return}
# This floor admits only a powered-off, empty VM. It is not an OS-install budget.
if($drive.Free -lt 20GB){throw 'Keep at least 20 GiB free before creating this empty lab. OS installation requires a separate measured space check.'}
New-Item -ItemType Directory -Path $root | Out-Null
$receipt=Join-Path $root 'provisioning.json'
try {
 New-Item -ItemType Directory -Path (Join-Path $root 'config'),(Join-Path $root 'disks') | Out-Null
 $disk=New-VHD -Path (Join-Path $root 'disks\system.vhdx') -SizeBytes 64GB -Dynamic -ErrorAction Stop
 $plan.diskPath=$disk.Path;$plan.stage='disk-created';$plan | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $receipt -Encoding UTF8
 $vm=New-VM -Name $Name -Generation 2 -MemoryStartupBytes 4GB -VHDPath $disk.Path -Path (Join-Path $root 'config') -SwitchName $switch.Name -ErrorAction Stop
 $plan.vmId=[string]$vm.Id;$plan.stage='vm-created';$plan | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $receipt -Encoding UTF8
 Set-VMProcessor -VM $vm -Count 2 -ErrorAction Stop
 Set-VMMemory -VM $vm -DynamicMemoryEnabled $true -StartupBytes 4GB -MinimumBytes 2GB -MaximumBytes 6GB -ErrorAction Stop
 Set-VM -VM $vm -AutomaticCheckpointsEnabled $false -AutomaticStartAction Nothing -AutomaticStopAction ShutDown -CheckpointType ProductionOnly -SmartPagingFilePath $root -ErrorAction Stop
 Set-VMFirmware -VM $vm -EnableSecureBoot On -SecureBootTemplate MicrosoftWindows -ErrorAction Stop
 Set-VMKeyProtector -VM $vm -NewLocalKeyProtector -ErrorAction Stop
 Enable-VMTPM -VM $vm -ErrorAction Stop
 Add-VMDvdDrive -VM $vm -ErrorAction Stop | Out-Null
 $current=Get-VM -Id $vm.Id -ErrorAction Stop
 if($current.State -ne 'Off' -or $current.Generation -ne 2 -or $current.AutomaticCheckpointsEnabled){throw 'VM verification failed.'}
 $plan.created=$true;$plan.stage='powered-off-lab-ready';$plan.diskFileBytes=(Get-Item -LiteralPath $disk.Path).Length
} catch {$plan.error=$_.Exception.Message;$plan.stage='partial-failure-inspect-before-retry';throw}
finally {$plan | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $receipt -Encoding UTF8}
$plan | ConvertTo-Json -Depth 5
