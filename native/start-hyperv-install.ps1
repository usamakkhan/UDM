param([string]$LabRoot='D:\UDM-Lab',[switch]$Start)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath($LabRoot)
$owner=Get-Content -LiteralPath (Join-Path $root 'provisioning.json') -Raw | ConvertFrom-Json
$media=Get-Content -LiteralPath (Join-Path $root 'media\download.json') -Raw | ConvertFrom-Json
$answer=Get-Content -LiteralPath (Join-Path $root 'answer-preparation.json') -Raw | ConvertFrom-Json
$vm=Get-VM -Id $owner.vmId -ErrorAction Stop
if($vm.State -ne 'Off' -or !$owner.created -or $owner.guestInstalled){throw 'Only the owned empty powered-off lab can be installed.'}
$disks=@(Get-VMHardDiskDrive -VM $vm -ErrorAction Stop)
if($disks.Count -ne 1 -or [IO.Path]::GetFullPath($disks[0].Path) -ne [IO.Path]::GetFullPath($owner.diskPath)){throw 'Unexpected guest disk attachment.'}
$disk=Get-VHD -Path $owner.diskPath -ErrorAction Stop
if($disk.Size -ne 64GB -or $disk.FileSize -ne $owner.diskFileBytes){throw 'System disk changed; inspect it instead of wiping it.'}
if($media.state -ne 'complete' -or $media.bytes -ne 5112850432 -or (Get-Item -LiteralPath $media.file).Length -ne $media.bytes){throw 'Installation media is incomplete.'}
if((Get-FileHash -LiteralPath $media.file -Algorithm SHA256).Hash -ne $media.sha256){throw 'Installation media hash mismatch.'}
if($answer.vmId -ne $owner.vmId -or $answer.imageIndex -ne 1 -or !$answer.compactOsRequested){throw 'Answer media belongs to another plan.'}
$drive=Get-PSDrive -Name ([IO.Path]::GetPathRoot($root).Substring(0,1))
if($drive.Free -lt 22GB){throw 'Need 22 GiB free after staging the ISO before attempting Compact OS installation.'}
$plan=[ordered]@{vmId=[string]$vm.Id;vmName=$vm.Name;mediaSha256=$media.sha256;onlyOwnedEmptyVhdAttached=$true;compactOsRequested=$true;hostFreeGiB=[math]::Round($drive.Free/1GB,2);started=$false;guestInstalled=$false;hostRestarted=$false;networkDisconnectedForOfflineSetup=$false}
if(!$Start){$plan|ConvertTo-Json;return}
$receipt=Join-Path $root 'installation-start.json'
if(Test-Path -LiteralPath $receipt){throw 'Installation was already attempted; inspect its state before retrying.'}
$dvds=@(Get-VMDvdDrive -VM $vm)
if($dvds.Count -ne 1 -or $dvds[0].Path){throw 'Unexpected DVD configuration.'}
Set-VMDvdDrive -VMDvdDrive $dvds[0] -Path $media.file -ErrorAction Stop
Add-VMDvdDrive -VM $vm -Path $answer.answerIso -ErrorAction Stop | Out-Null
$installer=Get-VMDvdDrive -VM $vm | Where-Object Path -eq $media.file
Set-VMFirmware -VM $vm -FirstBootDevice $installer -ErrorAction Stop
Disconnect-VMNetworkAdapter -VMName $vm.Name -ErrorAction Stop
$plan.networkDisconnectedForOfflineSetup=$true
Start-VM -VM $vm -ErrorAction Stop | Out-Null
$plan.started=$true
$plan|ConvertTo-Json | Set-Content -LiteralPath $receipt -Encoding UTF8
$plan|ConvertTo-Json
