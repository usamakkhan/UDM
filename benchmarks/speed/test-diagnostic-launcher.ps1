param([Parameter(Mandatory=$true)][string]$OutputDir)
$ErrorActionPreference='Stop'
if (Test-Path -LiteralPath $OutputDir) { throw 'Control output must be fresh' }
$OutputDir=[IO.Path]::GetFullPath($OutputDir)
New-Item -ItemType Directory -Path $OutputDir | Out-Null
[IO.File]::WriteAllText((Join-Path $OutputDir '.udm-diagnostic-launcher-controls'),'owned mock-only controls')
$global:udmLaunchTest=@{scenario='';calls=0}
function Start-Process {
 [CmdletBinding()]param([string]$FilePath,[string[]]$ArgumentList,[string]$Verb,[string]$WindowStyle,[switch]$PassThru)
 $global:udmLaunchTest.calls++
 if ($global:udmLaunchTest.scenario -eq 'cancel') { throw [ComponentModel.Win32Exception]::new(1223) }
 if ($global:udmLaunchTest.scenario -eq 'error') { Write-Error 'Simulated nonterminating launch failure'; return }
 if ($global:udmLaunchTest.scenario -eq 'missing') { return [PSCustomObject]@{Id=$null} }
 return [PSCustomObject]@{Id=42}
}
$results=@()
foreach ($case in @('success','cancel','error','missing')) {
 $queue=Join-Path $OutputDir $case
 New-Item -ItemType Directory -Path $queue | Out-Null
 [IO.File]::WriteAllText((Join-Path $queue '.udm-diagnostic-worker'),'owned test')
 $worker=Join-Path $queue 'worker.ps1'
 [IO.File]::WriteAllText($worker,'# Mock launch control; never executed')
 $global:udmLaunchTest.scenario=$case; $global:udmLaunchTest.calls=0
 $failed=$false
 try { $output=& (Join-Path $PSScriptRoot 'launch-diagnostic-session.ps1') -WorkerScript $worker -Queue $queue -Collector 'fixture.exe' -CollectorSha256 ('A'*64) -Area $OutputDir -DataParent $OutputDir -OwnerSid 'S-1-5-21-test' } catch { $failed=$true }
 $record=Get-Content -LiteralPath (Join-Path $queue 'elevation.json') -Raw | ConvertFrom-Json
 if ($global:udmLaunchTest.calls -ne 1) { throw 'Mock was not used exactly once' }
 if ($case -eq 'success') {
  if ($failed -or $output -ne 42 -or $record.state -ne 'process-created') { throw 'Success control failed' }
 } elseif (!$failed) { throw 'Launch failure was masked' }
 if ($case -eq 'cancel' -and ($record.state -ne 'canceled' -or $record.nativeErrorCode -ne 1223)) { throw 'Cancellation control failed' }
 $results+=@{case=$case;passed=$true;failedAsExpected=$failed;state=$record.state;nativeErrorCode=$record.nativeErrorCode}
}
$value=@{passed=$true;controls=$results;actualProcessLaunches=0;newElevationRequested=$false}
$text=$value | ConvertTo-Json -Depth 6
[IO.File]::WriteAllText((Join-Path $OutputDir 'result.json'),$text,[Text.UTF8Encoding]::new($false))
Write-Output $text
