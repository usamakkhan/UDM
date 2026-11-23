param([string]$LabRoot='D:\UDM-Lab',[switch]$Apply)
$ErrorActionPreference='Stop'
if($PSVersionTable.PSEdition -ne 'Desktop'){throw 'Run this Hyper-V device cleanup with Windows PowerShell 5.1 (powershell.exe), not PowerShell 7.'}
$root=[IO.Path]::GetFullPath($LabRoot)
$owner=Get-Content -LiteralPath (Join-Path $root 'provisioning.json') -Raw | ConvertFrom-Json
$access=Get-Content -LiteralPath (Join-Path $root 'guest-access.json') -Raw | ConvertFrom-Json
if($owner.vmId -ne '86ccf599-1723-4d50-b37b-d5414b1b8a54' -or $access.vmId -ne $owner.vmId -or !$access.guestAdministratorVerified){throw 'Guest acceptance does not match this lab.'}
$vm=Get-VM -Id $owner.vmId
Write-Output ('Owned guest state: '+[string]$vm.State)
if($vm.State -ne 'Off'){throw 'Stop only the owned guest normally before changing its installation media. Live DVD changes previously hung the management provider.'}
if(!$Apply){
 [ordered]@{readOnly=$true;vmId=$owner.vmId;knownHostDvdEditHang=$true;cleanupPerformed=$false;nextStep='Repair the host Hyper-V device-edit failure before an explicit -Apply retry.'} | ConvertTo-Json
 return
}
$source=Join-Path $root 'media\Windows11-Enterprise-LTSC-2024-eval-x64.iso'
$answer=Join-Path $root 'bootstrap\answer.iso'
$paths=@($source,$answer,(Join-Path $root 'bootstrap\Autounattend.xml'))
foreach($path in $paths){
 if(!(Test-Path -LiteralPath $path)){continue}
 $item=Get-Item -LiteralPath $path
 if($item.PSIsContainer -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)){throw 'Unexpected cleanup target.'}
 $parent=$item.Directory
 while($null -ne $parent){if($parent.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Cleanup ancestor redirects.'};$parent=$parent.Parent}
 if($item.FullName -eq $source -and $item.Length -ne 5112850432){throw 'Source ISO ownership metadata changed.'}
}
$dvds=@(Get-VMDvdDrive -VM $vm)
foreach($dvd in $dvds){if($dvd.Path -and $dvd.Path -notin @($source,$answer)){throw 'Unexpected DVD attachment.'}}
if(Test-Path -LiteralPath $source){$mounted=Get-DiskImage -ImagePath $source;if($mounted.Attached){Dismount-DiskImage -ImagePath $source | Out-Null}}
Write-Output 'Host ISO mount is detached.'
# Setting Path to null hung this host's provider both online and offline. Remove the
# now-unused optical devices while Off instead of retrying that failed operation.
foreach($dvd in $dvds){Write-Output ('Removing unused virtual DVD at controller location '+$dvd.ControllerLocation);Remove-VMDvdDrive -VMDvdDrive $dvd -Confirm:$false -ErrorAction Stop}
$disk=@(Get-VMHardDiskDrive -VM $vm)
if($disk.Count -ne 1 -or $disk[0].Path -ne $owner.diskPath){throw 'Guest disk attachment changed.'}
Set-VMFirmware -VM $vm -FirstBootDevice $disk[0]
if(@(Get-VMDvdDrive -VM $vm | Where-Object Path).Count){throw 'Guest still has attached setup media.'}
$removed=@()
foreach($path in $paths){
 if(Test-Path -LiteralPath $path){$bytes=(Get-Item -LiteralPath $path).Length;Remove-Item -LiteralPath $path -ErrorAction Stop;$removed+=[pscustomobject]@{path=$path;bytes=$bytes}}
}
[ordered]@{vmId=$owner.vmId;passed=$true;guestWasOff=$true;setupDvdsDetached=$true;systemDiskFirst=$true;removed=$removed;encryptedCredentialRetained=$true;applicationSetupCleanupPending=$true;completedUtc=[DateTime]::UtcNow.ToString('o')} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $root 'boot-media-cleanup.json') -Encoding UTF8
Get-Content -LiteralPath (Join-Path $root 'boot-media-cleanup.json')
