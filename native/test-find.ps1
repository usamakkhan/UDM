param(
 [Parameter(Mandatory=$true)][string]$BuildRoot,
 [Parameter(Mandatory=$true)][string]$RunRoot,
 [switch]$SkipBuild,
 [switch]$UseInstalledToolchain,
 [string]$ToolchainRoot
)
$ErrorActionPreference='Stop'
$findBuildRoot=[IO.Path]::GetFullPath($BuildRoot)
$findRunRoot=[IO.Path]::GetFullPath($RunRoot)
if(Test-Path -LiteralPath $findRunRoot){throw 'Use a new RunRoot; existing test results will not be overwritten.'}
if(!$SkipBuild){
 & (Join-Path $PSScriptRoot 'build.ps1') -FindDialogUiTestsOnly -OutputRoot $findBuildRoot -UseInstalledToolchain:$UseInstalledToolchain -ToolchainRoot $ToolchainRoot
}
$findExe=Join-Path $findBuildRoot 'release-native\Udm.FindDialogUiTests.exe'
if(!(Test-Path -LiteralPath $findExe)){throw 'Build the find fixture before using -SkipBuild.'}
# This GUI executable can return asynchronously to PowerShell; wait for its process.
$findProcess=Start-Process -FilePath $findExe -ArgumentList ('"'+$findRunRoot+'"') -WindowStyle Hidden -PassThru
$findProcess.WaitForExit()
$findResultPath=Join-Path $findRunRoot 'results.json'
if(!(Test-Path -LiteralPath $findResultPath)){throw 'Find fixture did not write its acceptance result.'}
$findResult=Get-Content -LiteralPath $findResultPath -Raw | ConvertFrom-Json
$findFailures=@($findResult.checks | Where-Object { !$_.passed })
if($findProcess.ExitCode -ne 0 -or !$findResult.passed -or $findFailures.Count){throw ('Find fixture failed: '+$findResult.error)}
[pscustomobject]@{Passed=@($findResult.checks).Count;Failed=0;Evidence=$findResultPath;ReusedBinary=[bool]$SkipBuild}
