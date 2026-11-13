param(
 [Parameter(Mandatory=$true)][string]$BuildRoot,
 [Parameter(Mandatory=$true)][string]$RunRoot,
 [string]$LiveEndpoint,
 [switch]$SkipBuild
)
$ErrorActionPreference='Stop'
$limiterBuild=[IO.Path]::GetFullPath($BuildRoot)
$limiterRun=[IO.Path]::GetFullPath($RunRoot)
if(Test-Path -LiteralPath $limiterRun){throw 'Use a new RunRoot; previous results are preserved.'}
if($limiterRun.Contains('"') -or $limiterRun.EndsWith('\')){throw 'Unsupported quoted RunRoot.'}
if($LiveEndpoint.Contains('"')){throw 'Unsupported quoted endpoint.'}
if(!$SkipBuild){& (Join-Path $PSScriptRoot 'build.ps1') -ProgressLimiterUiTestsOnly -OmitDebugSymbols -OutputRoot $limiterBuild}
$limiterExecutable=Join-Path $limiterBuild 'release-native\Udm.ProgressLimiterUiTests.exe'
if(!(Test-Path -LiteralPath $limiterExecutable)){throw 'Progress limiter fixture is missing; build it before using -SkipBuild.'}
$limiterSavedDark=[Environment]::GetEnvironmentVariable('UDM_TEST_LIMITER_DARK','Process')
try {
 foreach($limiterMode in @('light','dark')){
  $env:UDM_TEST_LIMITER_DARK=if($limiterMode -eq 'dark'){'1'}else{'0'}
  $limiterFolder=Join-Path $limiterRun $limiterMode
  $limiterArguments='"'+$limiterFolder+'"'
  if($LiveEndpoint){$limiterArguments+=' "'+$LiveEndpoint+'"'}
  $limiterProcess=Start-Process -FilePath $limiterExecutable -ArgumentList $limiterArguments -WindowStyle Hidden -PassThru
  if(!$limiterProcess.WaitForExit(120000)){throw ('Progress limiter fixture still running; inspect PID '+$limiterProcess.Id+' before retrying.')}
  $limiterProcess.Refresh()
  $limiterResultPath=Join-Path $limiterFolder 'results.json'
  if(!(Test-Path -LiteralPath $limiterResultPath)){throw ('Progress limiter fixture did not write its result; exit code '+$limiterProcess.ExitCode)}
  $limiterResult=Get-Content -Raw -LiteralPath $limiterResultPath | ConvertFrom-Json
  if($limiterProcess.ExitCode -ne 0 -or !$limiterResult.passed -or !$limiterResult.checks.Count -or @($limiterResult.checks | Where-Object {!$_.passed}).Count){throw ('Progress limiter fixture failed: '+$limiterResult.error)}
  if($limiterResult.darkMode -ne ($limiterMode -eq 'dark')){throw ('Requested UI mode was unavailable: '+$limiterMode)}
  [pscustomobject]@{Mode=$limiterMode;Passed=$limiterResult.checks.Count;Failed=0;Evidence=$limiterResultPath}
 }
} finally {
 [Environment]::SetEnvironmentVariable('UDM_TEST_LIMITER_DARK',$limiterSavedDark,'Process')
}
