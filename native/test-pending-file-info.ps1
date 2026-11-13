param(
 [Parameter(Mandatory=$true)][string]$BuildRoot,
 [Parameter(Mandatory=$true)][string]$RunRoot,
 [switch]$SkipBuild
)
$ErrorActionPreference='Stop'
$pendingBuild=[IO.Path]::GetFullPath($BuildRoot)
$pendingRun=[IO.Path]::GetFullPath($RunRoot)
if(Test-Path -LiteralPath $pendingRun){throw 'Use a new RunRoot; previous results are preserved.'}
if($pendingRun.Contains('"') -or $pendingRun.EndsWith('\')){throw 'Unsupported quoted RunRoot.'}
if(!$SkipBuild){& (Join-Path $PSScriptRoot 'build.ps1') -PendingFileInfoUiTestsOnly -OmitDebugSymbols -OutputRoot $pendingBuild}
$pendingExecutable=Join-Path $pendingBuild 'release-native\Udm.PendingFileInfoUiTests.exe'
if(!(Test-Path -LiteralPath $pendingExecutable)){throw 'Pending File Info fixture is missing; build it before using -SkipBuild.'}
foreach($pendingHelper in @('ffmpeg.exe','ffprobe.exe')){
 if(!(Test-Path -LiteralPath (Join-Path $pendingBuild ('release-native\tools\'+$pendingHelper)) -PathType Leaf)){throw 'Media helpers are missing beside the fixture. Supply release-native\tools with ffmpeg.exe and ffprobe.exe; a junction to existing helpers avoids duplicate copies.'}
}
$pendingSavedDark=[Environment]::GetEnvironmentVariable('UDM_TEST_PENDING_DARK','Process')
$pendingSavedConflict=[Environment]::GetEnvironmentVariable('UDM_TEST_PENDING_CONFLICT_ONLY','Process')
try {
 $env:UDM_TEST_PENDING_CONFLICT_ONLY='0'
 foreach($pendingMode in @('light','dark')){
  $env:UDM_TEST_PENDING_DARK=if($pendingMode -eq 'dark'){'1'}else{'0'}
  $pendingFolder=Join-Path $pendingRun $pendingMode
  $pendingProcess=Start-Process -FilePath $pendingExecutable -ArgumentList ('"'+$pendingFolder+'"') -WindowStyle Hidden -PassThru
  if(!$pendingProcess.WaitForExit(120000)){throw ('Pending File Info fixture still running; inspect PID '+$pendingProcess.Id+' before retrying.')}
  $pendingProcess.Refresh()
  $pendingResultPath=Join-Path $pendingFolder 'results.json'
  if(!(Test-Path -LiteralPath $pendingResultPath)){throw ('Pending File Info fixture did not write its result; exit code '+$pendingProcess.ExitCode)}
  $pendingResult=Get-Content -Raw -LiteralPath $pendingResultPath | ConvertFrom-Json
  if($pendingProcess.ExitCode -ne 0 -or !$pendingResult.passed -or !$pendingResult.checks.Count -or @($pendingResult.checks | Where-Object {!$_.passed}).Count){throw ('Pending File Info fixture failed: '+$pendingResult.error)}
  if($pendingResult.darkMode -ne ($pendingMode -eq 'dark')){throw ('Requested UI mode was unavailable: '+$pendingMode)}
  [pscustomobject]@{Mode=$pendingMode;Passed=$pendingResult.checks.Count;Failed=0;Evidence=$pendingResultPath}
 }
} finally {
 [Environment]::SetEnvironmentVariable('UDM_TEST_PENDING_DARK',$pendingSavedDark,'Process')
 [Environment]::SetEnvironmentVariable('UDM_TEST_PENDING_CONFLICT_ONLY',$pendingSavedConflict,'Process')
}
