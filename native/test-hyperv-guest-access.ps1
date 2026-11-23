param(
 [string]$LabRoot='D:\UDM-Lab',
 [string]$CredentialPath=(Join-Path $env:LOCALAPPDATA 'UDM-Development\hyperv-lab-credential.clixml'),
 [switch]$ProbeKernelTrace
)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath($LabRoot)
$owner=Get-Content -LiteralPath (Join-Path $root 'provisioning.json') -Raw | ConvertFrom-Json
$vm=Get-VM -Id $owner.vmId -ErrorAction Stop
if($vm.State -ne 'Running'){throw 'The owned guest is not running.'}
$credential=Import-Clixml -LiteralPath $CredentialPath
$session=$null
try {
 $session=New-PSSession -VMId $vm.Id -Credential $credential -ErrorAction Stop
 $guest=Invoke-Command -Session $session -ArgumentList ([bool]$ProbeKernelTrace) -ErrorAction Stop -ScriptBlock {
  param([bool]$Trace)
  $ErrorActionPreference='Stop'
  if($env:COMPUTERNAME -ne 'UDM-LAB'){throw 'Unexpected guest computer.'}
  $identity=[Security.Principal.WindowsIdentity]::GetCurrent()
  $principal=New-Object Security.Principal.WindowsPrincipal($identity)
  if(!$principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)){throw 'Guest session is not an administrator.'}
  $nonce=[guid]::NewGuid().ToString('N')
  $folder=Join-Path $env:ProgramFiles ('UDM-Lab-Access-'+$nonce)
  $file=Join-Path $folder 'owned.txt'
  $key='HKLM:\SOFTWARE\UDM-Lab-Access-'+$nonce
  try {
   New-Item -ItemType Directory -Path $folder -ErrorAction Stop | Out-Null
   'UDM owned guest administration probe' | Set-Content -LiteralPath $file -Encoding UTF8
   New-Item -Path $key -ErrorAction Stop | Out-Null
   New-ItemProperty -LiteralPath $key -Name Scope -Value 'UDM-LAB' -PropertyType String -ErrorAction Stop | Out-Null
   if((Get-ItemProperty -LiteralPath $key).Scope -ne 'UDM-LAB' -or !(Test-Path -LiteralPath $file)){throw 'Guest administration probe failed.'}
  } finally {
   if(Test-Path -LiteralPath $file){Remove-Item -LiteralPath $file}
   if(Test-Path -LiteralPath $folder){Remove-Item -LiteralPath $folder}
   if(Test-Path -LiteralPath $key){Remove-Item -LiteralPath $key}
  }
  $os=Get-CimInstance Win32_OperatingSystem
  $volume=Get-Volume -DriveLetter C
  $compact=(& compact.exe /CompactOS:Query 2>&1 | Out-String).Trim()
  $tracePassed=$false;$traceError=$null;$traceBytes=0
  if($Trace){
   $traceName='UDM-Lab-Probe-'+$nonce
   $tracePath=Join-Path $env:TEMP ($traceName+'.etl')
   $started=$false
   try {
    $message=(& logman.exe start $traceName -ets -p Microsoft-Windows-Kernel-File 0x10 5 -o $tracePath -bs 64 -nb 2 4 2>&1 | Out-String)
    if($LASTEXITCODE -ne 0){throw ('Guest trace start failed: '+$message.Trim())}
    $started=$true
    Get-Item -LiteralPath $env:WINDIR | Out-Null
   } catch {$traceError=$_.Exception.Message}
   finally {
    if($started){
     $message=(& logman.exe stop $traceName -ets 2>&1 | Out-String)
     if($LASTEXITCODE -ne 0){$traceError='Guest trace stop failed: '+$message.Trim()}
    }
    if(Test-Path -LiteralPath $tracePath){$traceBytes=(Get-Item -LiteralPath $tracePath).Length;Remove-Item -LiteralPath $tracePath}
   }
   $tracePassed=$started -and !$traceError -and $traceBytes -gt 0
  }
  [pscustomobject]@{computerName=$env:COMPUTERNAME;guestAdministratorVerified=$true;programFilesWriteVerified=$true;machineRegistryWriteVerified=$true;ownedProbesRemoved=$true;osCaption=$os.Caption;osVersion=$os.Version;compactStatus=$compact;guestDiskFreeGiB=[math]::Round($volume.SizeRemaining/1GB,2);guestDiskSizeGiB=[math]::Round($volume.Size/1GB,2);kernelTracingTested=$Trace;kernelTraceStartStopVerified=$tracePassed;kernelTraceBytesRemoved=$traceBytes;kernelTraceError=$traceError;traceEventContentsValidated=$false;interactiveUser=(Get-CimInstance Win32_ComputerSystem).UserName}
 }
 $record=[ordered]@{vmId=[string]$vm.Id;vmName=$vm.Name;guestAdministratorVerified=$guest.guestAdministratorVerified;guest=$guest;hostPermissionsChanged=$false;verifiedAtUtc=[DateTime]::UtcNow.ToString('o')}
 $record | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $root 'guest-access.json') -Encoding UTF8
 $owner.guestInstalled=$true;$owner.guestAdminConfigured=$true
 $owner | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $root 'provisioning.json') -Encoding UTF8
 $record | ConvertTo-Json -Depth 6
} finally {if($null -ne $session){Remove-PSSession -Session $session}}
