param(
 [Parameter(Mandatory=$true)][string]$BuildRoot,
 [Parameter(Mandatory=$true)][string]$RunRoot,
 [switch]$SkipBuild
)
$ErrorActionPreference='Stop'
$menuBuild=[IO.Path]::GetFullPath($BuildRoot)
$menuRun=[IO.Path]::GetFullPath($RunRoot)
if(Test-Path -LiteralPath $menuRun){throw 'Use a new RunRoot; previous results are preserved.'}
if($menuRun.Contains('"') -or $menuRun.EndsWith('\')){throw 'Unsupported quoted RunRoot.'}
if(!$SkipBuild){& (Join-Path $PSScriptRoot 'build.ps1') -ProgressMenuUiTestsOnly -OmitDebugSymbols -OutputRoot $menuBuild}
$menuExecutable=Join-Path $menuBuild 'release-native\Udm.ProgressMenuUiTests.exe'
if(!(Test-Path -LiteralPath $menuExecutable)){throw 'Progress menu fixture is missing; build it before using -SkipBuild.'}
$menuSavedDark=[Environment]::GetEnvironmentVariable('UDM_TEST_MENU_DARK','Process')
try {
 foreach($menuMode in @('light','dark')){
  $env:UDM_TEST_MENU_DARK=if($menuMode -eq 'dark'){'1'}else{'0'}
  $menuFolder=Join-Path $menuRun $menuMode
  $menuArguments='"'+$menuFolder+'"'
  $menuProcess=Start-Process -FilePath $menuExecutable -ArgumentList $menuArguments -WindowStyle Hidden -PassThru
  if(!$menuProcess.WaitForExit(120000)){throw ('Progress menu fixture still running; inspect PID '+$menuProcess.Id+' before retrying.')}
  $menuProcess.Refresh()
  $menuResultPath=Join-Path $menuFolder 'results.json'
  if(!(Test-Path -LiteralPath $menuResultPath)){throw ('Progress menu fixture did not write its result; exit code '+$menuProcess.ExitCode)}
  $menuResult=Get-Content -Raw -LiteralPath $menuResultPath | ConvertFrom-Json
  if($menuProcess.ExitCode -ne 0 -or !$menuResult.passed -or !$menuResult.checks.Count -or @($menuResult.checks | Where-Object {!$_.passed}).Count){throw ('Progress menu fixture failed: '+$menuResult.error)}
  if($menuResult.darkMode -ne ($menuMode -eq 'dark')){throw ('Requested UI mode was unavailable: '+$menuMode)}
  [pscustomobject]@{Mode=$menuMode;Passed=$menuResult.checks.Count;Failed=0;Evidence=$menuResultPath}
 }
} finally {
 [Environment]::SetEnvironmentVariable('UDM_TEST_MENU_DARK',$menuSavedDark,'Process')
}
