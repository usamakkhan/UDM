param([Parameter(Mandatory=$true)][ValidateSet('Enable','Disable')][string]$Mode)
# Run only after the user has approved this Windows boot-policy change.
# This script does not install a driver, trust certificates or restart Windows.
$ErrorActionPreference='Stop'
$admin=[Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
if(!$admin.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)){throw 'Administrator elevation is required.'}
$out=Join-Path $PSScriptRoot 'out'
New-Item -ItemType Directory -Path $out -Force | Out-Null
$record=[ordered]@{requestedUtc=[DateTime]::UtcNow.ToString('o');mode=$Mode;changed=$false;restartRequired=$false;restarted=$false;error=$null}
try{
    if($Mode -eq 'Enable'){
        # Do not silently suspend disk encryption or risk a recovery-key prompt.
        if(!(Get-Command Get-BitLockerVolume -ErrorAction SilentlyContinue)){throw 'Cannot check system-drive BitLocker protection; boot policy was not changed.'}
        $volume=Get-BitLockerVolume -MountPoint $env:SystemDrive -ErrorAction Stop
        if([int]$volume.ProtectionStatus -ne 0){throw 'System-drive BitLocker protection is active. Confirm recovery readiness before changing boot policy; nothing was changed.'}
    }
    & bcdedit.exe /enum | Set-Content (Join-Path $out 'boot-policy-before.txt')
    if($LASTEXITCODE -ne 0){throw 'Could not read current boot policy.'}
    $value=if($Mode -eq 'Enable'){'on'}else{'off'}
    & bcdedit.exe /set testsigning $value | Set-Content (Join-Path $out 'boot-policy-change.txt')
    if($LASTEXITCODE -ne 0){throw 'Windows rejected the Test Mode change. No workaround was attempted.'}
    $record.changed=$true;$record.restartRequired=$true
    & bcdedit.exe /enum | Set-Content (Join-Path $out 'boot-policy-after.txt')
    if($LASTEXITCODE -ne 0){throw 'Boot policy changed, but the follow-up read failed.'}
}catch{$record.error=$_.Exception.Message;throw}
finally{$record | ConvertTo-Json | Set-Content (Join-Path $out 'test-mode-change.json')}
Write-Output 'Boot policy updated. Save your work and restart manually to apply it.'
