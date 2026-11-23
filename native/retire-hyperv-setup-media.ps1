param(
 [string]$LabRoot='D:\UDM-Lab',
 [Parameter(Mandatory=$true)][string]$PlaceholderPath,
 [Parameter(Mandatory=$true)][string]$PlaceholderSha256,
 [switch]$Apply
)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath($LabRoot)
$owner=Get-Content -LiteralPath (Join-Path $root 'provisioning.json') -Raw | ConvertFrom-Json
$access=Get-Content -LiteralPath (Join-Path $root 'guest-access.json') -Raw | ConvertFrom-Json
$apps=Get-Content -LiteralPath (Join-Path $root 'apps-installed.json') -Raw | ConvertFrom-Json
if($owner.vmId -ne '86ccf599-1723-4d50-b37b-d5414b1b8a54' -or $access.vmId -ne $owner.vmId -or !$access.guestAdministratorVerified -or !$apps.passed){throw 'Installed lab acceptance is required.'}
$vm=Get-VM -Id $owner.vmId
if($vm.State -ne 'Off'){throw 'The owned VM must be off before changing media contents.'}
$disks=@(Get-VMHardDiskDrive -VM $vm)
if($disks.Count -ne 1 -or $disks[0].Path -ne $owner.diskPath){throw 'Guest disk ownership changed.'}
$template=Get-Item -LiteralPath $PlaceholderPath
if($template.PSIsContainer -or ($template.Attributes -band [IO.FileAttributes]::ReparsePoint) -or $template.Length -gt 1MB -or $template.Length -lt 32774){throw 'Invalid tiny data-disc template.'}
if((Get-FileHash -LiteralPath $template.FullName).Hash -ne $PlaceholderSha256){throw 'Template hash mismatch.'}
$bytes=[IO.File]::ReadAllBytes($template.FullName)
$ascii=[Text.Encoding]::ASCII.GetString($bytes)
if([Text.Encoding]::ASCII.GetString($bytes,32769,5) -ne 'CD001' -or !$ascii.Contains('UDM-Lab installation has completed.') -or $ascii.Contains('EL TORITO SPECIFICATION')){throw 'Expected our non-bootable retirement marker disc.'}
$targets=@(
 [pscustomobject]@{path=(Join-Path $root 'media\Windows11-Enterprise-LTSC-2024-eval-x64.iso');expectedBytes=5112850432L;expectedHash='67CEC5865EAA037A72DDC633A717A10A2BED50778862267223DDB9C60EF5DA68'},
 [pscustomobject]@{path=(Join-Path $root 'bootstrap\answer.iso');expectedBytes=55296L;expectedHash=$null}
)
$attached=@(Get-VMDvdDrive -VM $vm | Where-Object Path | Select-Object -ExpandProperty Path)
if($attached.Count -ne 2 -or @($attached | Where-Object {$_ -notin $targets.path}).Count){throw 'Unexpected optical attachments.'}
foreach($target in $targets){
 $item=Get-Item -LiteralPath $target.path
 if(!$item.FullName.StartsWith($root+'\',[StringComparison]::OrdinalIgnoreCase) -or $item.PSIsContainer -or $item.Length -ne $target.expectedBytes){throw 'Original media ownership metadata changed.'}
 $parent=$item
 while($null -ne $parent){if($parent.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Media path redirects.'};$parent=if($parent -is [IO.FileInfo]){$parent.Directory}else{$parent.Parent}}
 if((Get-DiskImage -ImagePath $item.FullName).Attached){throw 'Dismount the separate host ISO mount before retirement.'}
}
if(!$Apply){[pscustomobject]@{readOnly=$true;vmOff=$true;retireOriginalBytes=($targets | Measure-Object expectedBytes -Sum).Sum;replacementBytes=2*$template.Length;configurationPathsPreserved=$true} | ConvertTo-Json;return}
$record=[ordered]@{vmId=$owner.vmId;vmWasOff=$true;guestSystemDiskChanged=$false;dvdConfigurationChanged=$false;originalMediaRetired=$false;rows=@();startedUtc=[DateTime]::UtcNow.ToString('o')}
$freeBefore=(Get-PSDrive -Name ([IO.Path]::GetPathRoot($root).Substring(0,1))).Free
foreach($target in $targets){
 if($target.expectedHash -and (Get-FileHash -LiteralPath $target.path).Hash -ne $target.expectedHash){throw 'Original Windows distribution hash changed; preserve it.'}
 if((Get-VM -Id $owner.vmId).State -ne 'Off'){throw 'The VM started; stop before replacing any further media.'}
 $temporary=$target.path+'.retiring-'+[guid]::NewGuid().ToString('N')+'.iso'
 [IO.File]::WriteAllBytes($temporary,$bytes)
 if((Get-FileHash -LiteralPath $temporary).Hash -ne $PlaceholderSha256){throw 'Staged replacement mismatch.'}
 # Same-volume atomic replacement: never copy the 4.76 GiB installer elsewhere.
 # Retain the configured path and destination ACL; the tiny disc has no setup or credentials.
 [IO.File]::Replace($temporary,$target.path,[NullString]::Value,$true)
 if((Get-FileHash -LiteralPath $target.path).Hash -ne $PlaceholderSha256){throw 'Replacement verification failed.'}
 $record.rows += [pscustomobject]@{path=$target.path;originalBytes=$target.expectedBytes;replacementBytes=$bytes.Length;replacementSha256=$PlaceholderSha256;role='Non-bootable retired-media placeholder';containsSetupOrCredentials=$false}
 $record | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $root 'media-retirement.json') -Encoding UTF8
}
$record.originalMediaRetired=$true
$record.hostFreeBytesBefore=$freeBefore
$record.hostFreeBytesAfter=(Get-PSDrive -Name ([IO.Path]::GetPathRoot($root).Substring(0,1))).Free
$record.completedUtc=[DateTime]::UtcNow.ToString('o')
$record | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $root 'media-retirement.json') -Encoding UTF8
$record | ConvertTo-Json -Depth 5
