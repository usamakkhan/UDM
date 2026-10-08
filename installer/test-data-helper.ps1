param([Parameter(Mandatory=$true)][string]$Helper,[Parameter(Mandatory=$true)][string]$RunRoot)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $RunRoot){throw 'Use a new isolated RunRoot'}
New-Item -ItemType Directory -Path $RunRoot | Out-Null
$checks=[Collections.Generic.List[object]]::new()
function Write-Json($Path,$Value){[IO.File]::WriteAllText($Path,($Value|ConvertTo-Json -Depth 10),[Text.UTF8Encoding]::new($false))}
function Check($Value,$Name){$checks.Add(@{name=$Name;passed=[bool]$Value});if(!$Value){throw $Name}}
function Invoke-Helper([string[]]$Arguments,[bool]$ExpectFailure=$false){$raw=& $Helper @Arguments 2> (Join-Path $RunRoot 'last-stderr.txt');$code=$LASTEXITCODE;if($ExpectFailure){Check ($code -ne 0) 'Conflicting histories are rejected';return};if($code){throw 'Setup helper failed'};return ($raw|ConvertFrom-Json)}
try{
 $old=Join-Path $RunRoot 'old-app';$new=Join-Path $RunRoot 'new-app';$history=Join-Path $RunRoot 'custom-history';$fallback=Join-Path $RunRoot 'default-history'
 foreach($folder in @($old,$new,$history,$fallback)){New-Item -ItemType Directory -Path $folder | Out-Null}
 foreach($name in @('UDM.exe','Udm.NativeHost.exe')){[IO.File]::WriteAllText((Join-Path $new $name),'fixture placeholder')}
 Write-Json (Join-Path $old 'udm-data.json') @{dataDirectory=$history}
 Write-Json (Join-Path $history 'state.json') @{Downloads=@(@{Id='retained';Status='Paused';Received=7});Queues=@(@{Name='Main queue'})}
 [IO.File]::WriteAllBytes((Join-Path $history 'retained.part'),[byte[]](1,2,3,4,5,6,7))
 $before=@(Get-ChildItem $history -File | Get-FileHash | Select-Object Path,Hash)
 $manifest=Join-Path $RunRoot 'host.json';Write-Json $manifest @{name='com.udm.download_manager';type='stdio';path=(Join-Path $old 'Udm.NativeHost.exe')}
 $request=Join-Path $RunRoot 'request.json';Write-Json $request @{installationDirectory=$new;fallbackDirectory=$fallback;manifests=@($manifest)}
 $plan=Invoke-Helper @('--inspect-data',$request);Check ($plan.disposition -eq 'preserve-custom' -and $plan.dataDirectory -eq $history) 'Upgrade discovers the registered custom history'
 $receipt=Join-Path $RunRoot 'receipt.json';$prepared=Invoke-Helper @('--prepare-data',$request,$receipt)
 $config=Join-Path $new 'udm-data.json';Check ((Get-Content $config -Raw|ConvertFrom-Json).dataDirectory -eq $history) 'Upgrade publishes the original history location'
 $rolled=Invoke-Helper @('--rollback-data',$receipt);Check ($rolled.status -eq 'removed-created-config' -and !(Test-Path $config)) 'Rollback removes only its newly created configuration'
 $again=Invoke-Helper @('--rollback-data',$receipt);Check ($again.status -eq 'already-absent') 'Rollback can be repeated safely'
 $receipt2=Join-Path $RunRoot 'receipt2.json';$null=Invoke-Helper @('--prepare-data',$request,$receipt2)
 Write-Json $config @{dataDirectory=$history;userSetting='changed since prepare'};$configHash=(Get-FileHash $config).Hash
 $changed=Invoke-Helper @('--rollback-data',$receipt2);Check ($changed.status -eq 'preserved-changed-config' -and (Get-FileHash $config).Hash -eq $configHash) 'Rollback preserves a subsequently edited configuration'
 $receipt3=Join-Path $RunRoot 'receipt3.json';$existing=Invoke-Helper @('--prepare-data',$request,$receipt3);Check ($existing.disposition -eq 'existing' -and (Get-FileHash $config).Hash -eq $configHash) 'Repeated upgrade preserves existing configuration bytes'
 $nochange=Invoke-Helper @('--rollback-data',$receipt3);Check ($nochange.status -eq 'no-change') 'No-change upgrade rollback leaves existing configuration'
 Write-Json $config @{dataDirectory=$fallback};$conflictingHash=(Get-FileHash $config).Hash
 $null=Invoke-Helper @('--prepare-data',$request,(Join-Path $RunRoot 'conflict-receipt.json')) $true
 Check ((Get-FileHash $config).Hash -eq $conflictingHash -and !(Test-Path (Join-Path $RunRoot 'conflict-receipt.json'))) 'Rejected history conflict leaves configuration and receipt untouched'
 Check (@($before|Where-Object {(Get-FileHash $_.Path).Hash -ne $_.Hash}).Count -eq 0) 'History and partial file hashes remain unchanged across all operations'
 Write-Json (Join-Path $RunRoot 'results.json') @{passed=$true;checks=@($checks.ToArray())}
 Write-Output ($checks.Count.ToString()+' setup data checks passed')
 # The conflict case intentionally returns a nonzero native exit code.
 # Clear it after all assertions pass so callers see the suite outcome.
 $global:LASTEXITCODE=0
}catch{Write-Json (Join-Path $RunRoot 'results.json') @{passed=$false;error=$_.Exception.Message;checks=@($checks.ToArray())};throw}