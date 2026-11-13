param(
 [Parameter(Mandatory=$true)][string]$BuildRoot,
 [Parameter(Mandatory=$true)][string]$RunRoot,
 [switch]$SkipBuild
)
$ErrorActionPreference='Stop'
$dialBuild=[IO.Path]::GetFullPath($BuildRoot)
$dialRun=[IO.Path]::GetFullPath($RunRoot)
if(Test-Path -LiteralPath $dialRun){throw 'Use a new RunRoot; previous results are preserved.'}
if($dialRun.Contains('"') -or $dialRun.EndsWith('\')){throw 'Unsupported quoted RunRoot.'}
if(!$SkipBuild){& (Join-Path $PSScriptRoot 'build.ps1') -DialCredentialsUiTestsOnly -OmitDebugSymbols -OutputRoot $dialBuild}
$dialExecutable=Join-Path $dialBuild 'release-native\Udm.DialCredentialsUiTests.exe'
if(!(Test-Path -LiteralPath $dialExecutable)){throw 'Dial credentials fixture is missing; build it before using -SkipBuild.'}
$dialSavedDark=[Environment]::GetEnvironmentVariable('UDM_TEST_DIAL_CREDENTIALS_DARK','Process')
try {
 foreach($dialMode in @('light','dark')){
  $env:UDM_TEST_DIAL_CREDENTIALS_DARK=if($dialMode -eq 'dark'){'1'}else{'0'}
  $dialFolder=Join-Path $dialRun $dialMode
  $dialArguments='"'+$dialFolder+'"'
  $dialProcess=Start-Process -FilePath $dialExecutable -ArgumentList $dialArguments -WindowStyle Hidden -PassThru
  if(!$dialProcess.WaitForExit(120000)){throw ('Dial credentials fixture still running; inspect PID '+$dialProcess.Id+' before retrying.')}
  $dialProcess.Refresh()
  $dialResultPath=Join-Path $dialFolder 'results.json'
  if(!(Test-Path -LiteralPath $dialResultPath)){throw ('Dial credentials fixture did not write its result; exit code '+$dialProcess.ExitCode)}
  $dialResult=Get-Content -Raw -LiteralPath $dialResultPath | ConvertFrom-Json
  if($dialProcess.ExitCode -ne 0 -or !$dialResult.passed -or !$dialResult.checks.Count -or @($dialResult.checks | Where-Object {!$_.passed}).Count){throw ('Dial credentials fixture failed: '+$dialResult.error)}
  if($dialResult.darkMode -ne ($dialMode -eq 'dark')){throw ('Requested UI mode was unavailable: '+$dialMode)}
  [pscustomobject]@{Mode=$dialMode;Passed=$dialResult.checks.Count;Failed=0;Evidence=$dialResultPath}
 }
} finally {
 [Environment]::SetEnvironmentVariable('UDM_TEST_DIAL_CREDENTIALS_DARK',$dialSavedDark,'Process')
}
