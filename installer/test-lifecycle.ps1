param([Parameter(Mandatory=$true)][string]$Compiler,[Parameter(Mandatory=$true)][string]$RunRoot)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $RunRoot){throw 'Use a fresh isolated RunRoot'}
New-Item -ItemType Directory -Path $RunRoot | Out-Null
$checks=[Collections.Generic.List[object]]::new()
function Check($Value,$Name){$checks.Add(@{name=$Name;passed=[bool]$Value});if(!$Value){throw $Name}}
$fixture=Join-Path $RunRoot 'LifecycleFixture.exe'
$app=Join-Path $RunRoot 'app'
$history=Join-Path $RunRoot 'history'
$lockName='Local\UDMLifecycleFixture'+[guid]::NewGuid().ToString('N')
$mutex=$null
function Run-Fixture($Program,$Label){
 $arguments=@('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/SP-',('/DIR="'+$app+'"'),('/UDMLOCKNAME='+$lockName),('/LOG="'+(Join-Path $RunRoot ($Label+'.log'))+'"'))
 $child=Start-Process -FilePath $Program -ArgumentList $arguments -WindowStyle Hidden -PassThru
 if(!$child.WaitForExit(30000)){throw ('Owned fixture process '+$child.Id+' is still running; not restarted')}
 return $child.ExitCode
}
try {
 & $Compiler ('/O'+$RunRoot) (Join-Path $PSScriptRoot 'tests\LifecycleFixture.iss') *> (Join-Path $RunRoot 'compile.log')
 if($LASTEXITCODE){throw 'Lifecycle fixture compilation failed'}
 New-Item -ItemType Directory -Path $history | Out-Null
 $sentinel=Join-Path $history 'retained.part';[IO.File]::WriteAllBytes($sentinel,[byte[]](1,2,3,4,5))
 $hash=(Get-FileHash -LiteralPath $sentinel).Hash
 $mutex=[Threading.Mutex]::new($true,$lockName)
 Check ((Run-Fixture $fixture 'blocked-install') -ne 0) 'Competing installation is rejected before file copying'
 Check (!(Test-Path -LiteralPath (Join-Path $app 'LifecycleFixture.txt'))) 'Blocked installation publishes no payload'
 $mutex.ReleaseMutex();$mutex.Dispose();$mutex=$null
 Check ((Run-Fixture $fixture 'install') -eq 0) 'Installation succeeds after competing transaction releases its lock'
 Check (Test-Path -LiteralPath (Join-Path $app 'LifecycleFixture.txt')) 'Actual fixture payload is installed'
 $uninstaller=Join-Path $app 'unins000.exe';Check (Test-Path -LiteralPath $uninstaller) 'Actual uninstaller is created'
 $mutex=[Threading.Mutex]::new($true,$lockName)
 Check ((Run-Fixture $uninstaller 'blocked-uninstall') -ne 0) 'Competing uninstall is rejected before payload removal'
 Check (Test-Path -LiteralPath (Join-Path $app 'LifecycleFixture.txt')) 'Blocked uninstall preserves installed payload'
 $mutex.ReleaseMutex();$mutex.Dispose();$mutex=$null
 Check ((Run-Fixture $uninstaller 'uninstall') -eq 0) 'Uninstall succeeds after the competing lock is released'
 Check (!(Test-Path -LiteralPath (Join-Path $app 'LifecycleFixture.txt'))) 'Successful uninstall removes its owned payload'
 Check ((Get-FileHash -LiteralPath $sentinel).Hash -eq $hash) 'Separate partial-download bytes survive the entire lifecycle'
} finally {
 if($mutex){$mutex.ReleaseMutex();$mutex.Dispose()}
 $checks | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $RunRoot 'checks.json') -Encoding utf8
}
Write-Output ($checks.Count.ToString()+' installer lifecycle checks passed')