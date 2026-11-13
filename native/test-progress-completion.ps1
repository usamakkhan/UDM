param(
 [Parameter(Mandatory=$true)][string]$BuildRoot,
 [Parameter(Mandatory=$true)][string]$RunRoot,
 [switch]$SkipBuild
)
$ErrorActionPreference='Stop'
$completionBuild=[IO.Path]::GetFullPath($BuildRoot)
$completionRun=[IO.Path]::GetFullPath($RunRoot)
if(Test-Path -LiteralPath $completionRun){throw 'Use a new RunRoot; previous results are preserved.'}
if($completionRun.Contains('"') -or $completionRun.EndsWith('\')){throw 'Unsupported quoted RunRoot.'}
if(!$SkipBuild){& (Join-Path $PSScriptRoot 'build.ps1') -ProgressCompletionUiTestsOnly -OmitDebugSymbols -OutputRoot $completionBuild}
$completionExecutable=Join-Path $completionBuild 'release-native\Udm.ProgressCompletionUiTests.exe'
if(!(Test-Path -LiteralPath $completionExecutable)){throw 'Progress completion fixture is missing; build it before using -SkipBuild.'}
$completionSavedDark=[Environment]::GetEnvironmentVariable('UDM_TEST_COMPLETION_DARK','Process')
try {
 foreach($completionMode in @('light','dark')){
  $env:UDM_TEST_COMPLETION_DARK=if($completionMode -eq 'dark'){'1'}else{'0'}
  $completionFolder=Join-Path $completionRun $completionMode
  $completionArguments='"'+$completionFolder+'"'
  $completionProcess=Start-Process -FilePath $completionExecutable -ArgumentList $completionArguments -WindowStyle Hidden -PassThru
  if(!$completionProcess.WaitForExit(120000)){throw ('Progress completion fixture still running; inspect PID '+$completionProcess.Id+' before retrying.')}
  $completionProcess.Refresh()
  $completionResultPath=Join-Path $completionFolder 'results.json'
  if(!(Test-Path -LiteralPath $completionResultPath)){throw ('Progress completion fixture did not write its result; exit code '+$completionProcess.ExitCode)}
  $completionResult=Get-Content -Raw -LiteralPath $completionResultPath | ConvertFrom-Json
  if($completionProcess.ExitCode -ne 0 -or !$completionResult.passed -or !$completionResult.checks.Count -or @($completionResult.checks | Where-Object {!$_.passed}).Count){throw ('Progress completion fixture failed: '+$completionResult.error)}
  if($completionResult.darkMode -ne ($completionMode -eq 'dark')){throw ('Requested UI mode was unavailable: '+$completionMode)}
  [pscustomobject]@{Mode=$completionMode;Passed=$completionResult.checks.Count;Failed=0;Evidence=$completionResultPath}
 }
} finally {
 [Environment]::SetEnvironmentVariable('UDM_TEST_COMPLETION_DARK',$completionSavedDark,'Process')
}
