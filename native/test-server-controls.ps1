param(
 [Parameter(Mandatory=$true)][string]$BuildRoot,
 [Parameter(Mandatory=$true)][string]$RunRoot,
 [switch]$SkipBuild,
 [switch]$UseInstalledToolchain,
 [string]$ToolchainRoot
)
$ErrorActionPreference='Stop'
$serverBuildRoot=[IO.Path]::GetFullPath($BuildRoot)
$serverRunRoot=[IO.Path]::GetFullPath($RunRoot)
if(Test-Path -LiteralPath $serverRunRoot){throw 'Use a new RunRoot; existing test results will not be overwritten.'}
if(!$SkipBuild){
 & (Join-Path $PSScriptRoot 'build.ps1') -ServerControlsUiTestsOnly -OutputRoot $serverBuildRoot -UseInstalledToolchain:$UseInstalledToolchain -ToolchainRoot $ToolchainRoot
}
$serverExe=Join-Path $serverBuildRoot 'release-native\Udm.ServerControlsUiTests.exe'
if(!(Test-Path -LiteralPath $serverExe)){throw 'Build the server-controls fixture before using -SkipBuild.'}
# This GUI executable can return asynchronously to PowerShell; wait for its process.
$serverProcess=Start-Process -FilePath $serverExe -ArgumentList ('"'+$serverRunRoot+'"') -WindowStyle Hidden -PassThru
$serverProcess.WaitForExit()
$serverResultPath=Join-Path $serverRunRoot 'results.json'
if(!(Test-Path -LiteralPath $serverResultPath)){throw 'Server-controls fixture did not write its acceptance result.'}
$serverResult=Get-Content -LiteralPath $serverResultPath -Raw | ConvertFrom-Json
$serverFailures=@($serverResult.checks | Where-Object { !$_.passed })
if($serverProcess.ExitCode -ne 0 -or !$serverResult.passed -or $serverFailures.Count){throw ('Server-controls fixture failed: '+$serverResult.error)}
[pscustomobject]@{Passed=@($serverResult.checks).Count;Failed=0;Evidence=$serverResultPath;ReusedBinary=[bool]$SkipBuild}
