param([string]$PlanPath='D:\UDM\candidates\deployment-184\plan.json',[string]$StatePath='D:\UDM\user-data\state.json')
$ErrorActionPreference='Stop'
$plan=Get-Content -LiteralPath $PlanPath -Raw | ConvertFrom-Json
$checks=[Collections.Generic.List[object]]::new()
function Check([string]$Name,[bool]$Passed,[string]$Detail=''){$checks.Add([pscustomobject]@{Name=$Name;Passed=$Passed;Detail=$Detail})}
function HashMatches([string]$Path,[string]$Expected){if(!(Test-Path -LiteralPath $Path -PathType Leaf)){return $false};return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash -eq $Expected}
$expected=@('UDM.exe','Udm.NativeHost.exe','Udm.Monitor.exe','Udm.SetupHelper.exe')
Check 'Plan names exactly the four native programs' ((@($plan.files).Count -eq 4) -and (@($plan.files.name | Sort-Object -Unique).Count -eq 4) -and !(@($plan.files.name | Where-Object {$_ -notin $expected}).Count))
$destinations=@($plan.files | ForEach-Object {[IO.Path]::GetFullPath($_.destination)})
Check 'All destinations share one release directory' (@($destinations | ForEach-Object {Split-Path $_ -Parent} | Sort-Object -Unique).Count -eq 1)
foreach($file in $plan.files){
 Check ($file.name+': source hash') (HashMatches $file.source $file.sourceHash)
 Check ($file.name+': installed hash') (HashMatches $file.destination $file.currentHash)
 Check ($file.name+': backup hash') (HashMatches (Join-Path $plan.backupRoot $file.name) $file.currentHash)
 Check ($file.name+': destination filename') ([IO.Path]::GetFileName($file.destination) -ieq $file.name)
}
$names=@($expected | ForEach-Object {[IO.Path]::GetFileNameWithoutExtension($_)})
$running=@(Get-Process -Name $names -ErrorAction SilentlyContinue)
foreach($process in $running){$processPath=$null;try{$processPath=$process.Path}catch{};if(!$processPath){Check ('Process '+$process.Id+' has unverified executable path') $false $process.ProcessName}elseif($processPath -in $destinations){Check ('Installed executable still running: '+$process.ProcessName) $false ('PID '+$process.Id)}}
Check 'Personal catalog matches approved deployment snapshot' (HashMatches $StatePath $plan.personalStateHash)
$catalog=Get-Content -LiteralPath $StatePath -Raw | ConvertFrom-Json
Check 'Personal record count matches snapshot' (@($catalog.Downloads).Count -eq $plan.records)
$active=@($catalog.Downloads | Where-Object {$_.Status -notin @('Complete','Failed','Paused','Queued','Stopped','Canceled','Cancelled')})
Check 'No active or unrecognized download states' ($active.Count -eq 0) (($active | Group-Object Status | ForEach-Object {$_.Name+': '+$_.Count}) -join ', ')
[pscustomobject]@{Ready=(@($checks | Where-Object {!$_.Passed}).Count -eq 0);Checks=$checks;Scope='Read-only preflight. Must be rerun immediately before replacement; this does not hold file locks or deploy.'}
