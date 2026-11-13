param(
 [Parameter(Mandatory=$true)][string]$BuildRoot,
 [Parameter(Mandatory=$true)][string]$RunRoot,
 [switch]$SkipBuild
)
$ErrorActionPreference='Stop'
$preferencesBuild=[IO.Path]::GetFullPath($BuildRoot)
$preferencesRun=[IO.Path]::GetFullPath($RunRoot)
if(Test-Path -LiteralPath $preferencesRun){throw 'Use a new RunRoot; previous results are preserved.'}
if($preferencesRun.Contains('"') -or $preferencesRun.EndsWith('\')){throw 'Unsupported quoted RunRoot.'}
if(!$SkipBuild){& (Join-Path $PSScriptRoot 'build.ps1') -ProgressPreferencesUiTestsOnly -OmitDebugSymbols -OutputRoot $preferencesBuild}
$preferencesExecutable=Join-Path $preferencesBuild 'release-native\Udm.ProgressPreferencesUiTests.exe'
if(!(Test-Path -LiteralPath $preferencesExecutable)){throw 'Progress preferences fixture is missing; build it before using -SkipBuild.'}
$preferencesSavedDark=[Environment]::GetEnvironmentVariable('UDM_TEST_PROGRESS_PREFS_DARK','Process')
try {
 foreach($preferencesMode in @('light','dark')){
  $env:UDM_TEST_PROGRESS_PREFS_DARK=if($preferencesMode -eq 'dark'){'1'}else{'0'}
  $preferencesFolder=Join-Path $preferencesRun $preferencesMode
  $preferencesArguments='"'+$preferencesFolder+'"'
  $preferencesProcess=Start-Process -FilePath $preferencesExecutable -ArgumentList $preferencesArguments -WindowStyle Hidden -PassThru
  if(!$preferencesProcess.WaitForExit(120000)){throw ('Progress preferences fixture still running; inspect PID '+$preferencesProcess.Id+' before retrying.')}
  $preferencesProcess.Refresh()
  $preferencesResultPath=Join-Path $preferencesFolder 'results.json'
  if(!(Test-Path -LiteralPath $preferencesResultPath)){throw ('Progress preferences fixture did not write its result; exit code '+$preferencesProcess.ExitCode)}
  $preferencesResult=Get-Content -Raw -LiteralPath $preferencesResultPath | ConvertFrom-Json
  if($preferencesProcess.ExitCode -ne 0 -or !$preferencesResult.passed -or !$preferencesResult.checks.Count -or @($preferencesResult.checks | Where-Object {!$_.passed}).Count){throw ('Progress preferences fixture failed: '+$preferencesResult.error)}
  if($preferencesResult.darkMode -ne ($preferencesMode -eq 'dark')){throw ('Requested UI mode was unavailable: '+$preferencesMode)}
  [pscustomobject]@{Mode=$preferencesMode;Passed=$preferencesResult.checks.Count;Failed=0;Evidence=$preferencesResultPath}
 }
} finally {
 [Environment]::SetEnvironmentVariable('UDM_TEST_PROGRESS_PREFS_DARK',$preferencesSavedDark,'Process')
}
