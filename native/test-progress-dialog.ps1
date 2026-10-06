param(
 [Parameter(Mandatory=$true)][string]$BuildRoot,
 [Parameter(Mandatory=$true)][string]$RunRoot,
 [string]$ReferencePath=(Join-Path (Split-Path $PSScriptRoot -Parent) 'docs\reference\progress-dialogs.json'),
 [switch]$SkipBuild
)
$ErrorActionPreference='Stop'
$progressBuild=[IO.Path]::GetFullPath($BuildRoot)
$progressRun=[IO.Path]::GetFullPath($RunRoot)
$progressReference=[IO.Path]::GetFullPath($ReferencePath)
if(Test-Path -LiteralPath $progressRun){throw 'Use a new RunRoot; previous results are preserved.'}
foreach($progressArgument in @($progressRun,$progressReference)){if($progressArgument.Contains('"') -or $progressArgument.EndsWith('\')){throw 'Unsupported quoted argument.'}}
if(!$SkipBuild){& (Join-Path $PSScriptRoot 'build.ps1') -ProgressDialogUiTestsOnly -OutputRoot $progressBuild}
$progressExecutable=Join-Path $progressBuild 'release-native\Udm.ProgressDialogUiTests.exe'
$progressProcess=Start-Process -FilePath $progressExecutable -ArgumentList ('"'+$progressRun+'" "'+$progressReference+'"') -WindowStyle Hidden -PassThru
if(!$progressProcess.WaitForExit(30000)){throw ('Progress fixture still running; inspect PID '+$progressProcess.Id+' before retrying.')}
$progressResultPath=Join-Path $progressRun 'results.json'
if(!(Test-Path -LiteralPath $progressResultPath)){throw ('Progress fixture did not write its result; exit code '+$progressProcess.ExitCode)}
$progressResult=Get-Content -Raw -LiteralPath $progressResultPath | ConvertFrom-Json
if($progressProcess.ExitCode -ne 0 -or !$progressResult.passed){throw ('Progress fixture failed: '+$progressResult.error)}
[pscustomobject]@{Passed=$progressResult.checks.Count;Failed=0;Evidence=$progressResultPath}
