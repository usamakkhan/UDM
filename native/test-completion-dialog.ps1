param(
 [Parameter(Mandatory=$true)][string]$BuildRoot,
 [Parameter(Mandatory=$true)][string]$RunRoot,
 [string]$ReferencePath=(Join-Path (Split-Path $PSScriptRoot -Parent) 'docs\reference\completion-dialog.json'),
 [switch]$SkipBuild
)
$ErrorActionPreference='Stop'
$completionBuild=[IO.Path]::GetFullPath($BuildRoot)
$completionRun=[IO.Path]::GetFullPath($RunRoot)
$completionReference=[IO.Path]::GetFullPath($ReferencePath)
if(Test-Path -LiteralPath $completionRun){throw 'Use a new RunRoot; previous results are preserved.'}
foreach($completionArgument in @($completionRun,$completionReference)){if($completionArgument.Contains('"') -or $completionArgument.EndsWith('\')){throw 'Unsupported quoted argument.'}}
if(!$SkipBuild){& (Join-Path $PSScriptRoot 'build.ps1') -CompletionDialogUiTestsOnly -OutputRoot $completionBuild}
$completionExecutable=Join-Path $completionBuild 'release-native\Udm.CompletionDialogUiTests.exe'
$completionProcess=Start-Process -FilePath $completionExecutable -ArgumentList ('"'+$completionRun+'" "'+$completionReference+'"') -WindowStyle Hidden -PassThru
if(!$completionProcess.WaitForExit(30000)){throw ('Completion fixture still running; inspect PID '+$completionProcess.Id+' before retrying.')}
$completionResultPath=Join-Path $completionRun 'results.json'
if(!(Test-Path -LiteralPath $completionResultPath)){throw ('Completion fixture did not write its result; exit code '+$completionProcess.ExitCode)}
$completionResult=Get-Content -Raw -LiteralPath $completionResultPath | ConvertFrom-Json
if($completionProcess.ExitCode -ne 0 -or !$completionResult.passed){throw ('Completion fixture failed: '+$completionResult.error)}
[pscustomobject]@{Passed=$completionResult.checks.Count;Failed=0;Evidence=$completionResultPath}
