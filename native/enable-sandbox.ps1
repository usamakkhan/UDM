param(
 [switch]$Enable,
 [Parameter(Mandatory=$true)][string]$ReceiptPath
)
$ErrorActionPreference='Stop'
$receipt=[IO.Path]::GetFullPath($ReceiptPath)
if(Test-Path -LiteralPath $receipt){throw 'Use a new receipt path; previous results are preserved.'}
$result=[ordered]@{feature='Containers-DisposableClientVM';enableRequested=[bool]$Enable;featureChanged=$false;restartInitiated=$false;restartNeeded=$null;passed=$false;error=$null}
try {
 $identity=[Security.Principal.WindowsIdentity]::GetCurrent()
 $principal=New-Object Security.Principal.WindowsPrincipal($identity)
 if(!$principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)){throw 'Windows requires an administrator to query or enable this optional feature.'}
 $before=Get-WindowsOptionalFeature -Online -FeatureName $result.feature -ErrorAction Stop
 $result.previousState=[string]$before.State
 if($before.State -eq 'EnablePending'){$result.restartNeeded=$true}
 if($Enable -and $before.State -notin @('Enabled','EnablePending')){
  $change=Enable-WindowsOptionalFeature -Online -FeatureName $result.feature -All -NoRestart -ErrorAction Stop
  $result.featureChanged=$true;$result.restartNeeded=[bool]$change.RestartNeeded
 }
 $after=Get-WindowsOptionalFeature -Online -FeatureName $result.feature -ErrorAction Stop
 $result.finalState=[string]$after.State
 $result.passed=(!$Enable -or $after.State -eq 'Enabled' -or $after.State -eq 'EnablePending')
} catch {$result.error=$_.Exception.Message}
$result | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $receipt -Encoding UTF8
$result | ConvertTo-Json -Depth 4
if(!$result.passed){exit 1}
