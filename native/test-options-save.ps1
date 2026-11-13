param(
 [Parameter(Mandatory=$true)][string]$BuildRoot,
 [Parameter(Mandatory=$true)][string]$RunRoot,
 [switch]$SkipBuild
)
$ErrorActionPreference='Stop'
$optionsBuild=[IO.Path]::GetFullPath($BuildRoot)
$optionsRun=[IO.Path]::GetFullPath($RunRoot)
if(Test-Path -LiteralPath $optionsRun){throw 'Use a new RunRoot; previous results are preserved.'}
if($optionsRun.Contains('"') -or $optionsRun.EndsWith('\')){throw 'Unsupported quoted RunRoot.'}
if(!$SkipBuild){& (Join-Path $PSScriptRoot 'build.ps1') -OptionsTabsUiTestsOnly -OmitDebugSymbols -OutputRoot $optionsBuild}
$optionsExecutable=Join-Path $optionsBuild 'release-native\Udm.OptionsTabsUiTests.exe'
if(!(Test-Path -LiteralPath $optionsExecutable)){throw 'Options fixture is missing; build it before using -SkipBuild.'}
$optionsSavedDark=[Environment]::GetEnvironmentVariable('UDM_TEST_OPTIONS_DARK','Process')
try {
 foreach($optionsMode in @('light','dark')){
  $env:UDM_TEST_OPTIONS_DARK=if($optionsMode -eq 'dark'){'1'}else{'0'}
  $optionsFolder=Join-Path $optionsRun $optionsMode
  $optionsArguments='"'+$optionsFolder+'"'
  $optionsProcess=Start-Process -FilePath $optionsExecutable -ArgumentList $optionsArguments -WindowStyle Hidden -PassThru
  if(!$optionsProcess.WaitForExit(120000)){throw ('Options fixture still running; inspect PID '+$optionsProcess.Id+' before retrying.')}
  $optionsProcess.Refresh()
  $optionsResultPath=Join-Path $optionsFolder 'results.json'
  if(!(Test-Path -LiteralPath $optionsResultPath)){throw ('Options fixture did not write its result; exit code '+$optionsProcess.ExitCode)}
  $optionsResult=Get-Content -Raw -LiteralPath $optionsResultPath | ConvertFrom-Json
  if($optionsProcess.ExitCode -ne 0 -or !$optionsResult.passed -or !$optionsResult.checks.Count -or @($optionsResult.checks | Where-Object {!$_.passed}).Count){throw ('Options fixture failed: '+$optionsResult.error)}
  if($optionsResult.darkMode -ne ($optionsMode -eq 'dark')){throw ('Requested UI mode was unavailable: '+$optionsMode)}
  [pscustomobject]@{Mode=$optionsMode;Passed=$optionsResult.checks.Count;Failed=0;Evidence=$optionsResultPath}
 }
} finally {
 [Environment]::SetEnvironmentVariable('UDM_TEST_OPTIONS_DARK',$optionsSavedDark,'Process')
}
