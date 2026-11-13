param(
 [Parameter(Mandatory=$true)][string]$BuildRoot,
 [Parameter(Mandatory=$true)][string]$RunRoot,
 [switch]$SkipBuild
)
$ErrorActionPreference='Stop'
$scheduleBuild=[IO.Path]::GetFullPath($BuildRoot)
$scheduleRun=[IO.Path]::GetFullPath($RunRoot)
if(Test-Path -LiteralPath $scheduleRun){throw 'Use a new RunRoot; previous results are preserved.'}
if($scheduleRun.Contains('"') -or $scheduleRun.EndsWith('\')){throw 'Unsupported quoted RunRoot.'}
if(!$SkipBuild){& (Join-Path $PSScriptRoot 'build.ps1') -ScheduleDownloadWizardUiTestsOnly -OmitDebugSymbols -OutputRoot $scheduleBuild}
$scheduleExecutable=Join-Path $scheduleBuild 'release-native\Udm.ScheduleDownloadWizardUiTests.exe'
if(!(Test-Path -LiteralPath $scheduleExecutable)){throw 'Scheduled-download wizard fixture is missing; build it before using -SkipBuild.'}
foreach($scheduleHelper in @('ffmpeg.exe','ffprobe.exe')){
 if(!(Test-Path -LiteralPath (Join-Path $scheduleBuild ('release-native\tools\'+$scheduleHelper)) -PathType Leaf)){throw 'Media helpers are missing beside the fixture. Supply release-native\tools with ffmpeg.exe and ffprobe.exe; a junction to existing helpers avoids duplicate copies.'}
}
$scheduleSavedDark=[Environment]::GetEnvironmentVariable('UDM_TEST_PENDING_DARK','Process')
$scheduleSavedConflict=[Environment]::GetEnvironmentVariable('UDM_TEST_PENDING_CONFLICT_ONLY','Process')
try {
 $env:UDM_TEST_PENDING_CONFLICT_ONLY='0'
 foreach($scheduleMode in @('light','dark')){
  $env:UDM_TEST_PENDING_DARK=if($scheduleMode -eq 'dark'){'1'}else{'0'}
  $scheduleFolder=Join-Path $scheduleRun $scheduleMode
  $scheduleProcess=Start-Process -FilePath $scheduleExecutable -ArgumentList ('"'+$scheduleFolder+'"') -WindowStyle Hidden -PassThru
  if(!$scheduleProcess.WaitForExit(120000)){throw ('Schedule File Info fixture still running; inspect PID '+$scheduleProcess.Id+' before retrying.')}
  $scheduleProcess.Refresh()
  $scheduleResultPath=Join-Path $scheduleFolder 'results.json'
  if(!(Test-Path -LiteralPath $scheduleResultPath)){throw ('Schedule File Info fixture did not write its result; exit code '+$scheduleProcess.ExitCode)}
  $scheduleResult=Get-Content -Raw -LiteralPath $scheduleResultPath | ConvertFrom-Json
  if($scheduleProcess.ExitCode -ne 0 -or !$scheduleResult.passed -or !$scheduleResult.checks.Count -or @($scheduleResult.checks | Where-Object {!$_.passed}).Count){throw ('Schedule File Info fixture failed: '+$scheduleResult.error)}
  if($scheduleResult.darkMode -ne ($scheduleMode -eq 'dark')){throw ('Requested UI mode was unavailable: '+$scheduleMode)}
  [pscustomobject]@{Mode=$scheduleMode;Passed=$scheduleResult.checks.Count;Failed=0;Evidence=$scheduleResultPath}
 }
} finally {
 [Environment]::SetEnvironmentVariable('UDM_TEST_PENDING_DARK',$scheduleSavedDark,'Process')
 [Environment]::SetEnvironmentVariable('UDM_TEST_PENDING_CONFLICT_ONLY',$scheduleSavedConflict,'Process')
}
