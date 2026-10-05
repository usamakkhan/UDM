param(
 [Parameter(Mandatory=$true)][string]$BuildRoot,
 [Parameter(Mandatory=$true)][string]$RunRoot,
 [switch]$SkipBuild,
 [switch]$UseInstalledToolchain,
 [string]$ToolchainRoot
)
$ErrorActionPreference='Stop'
$propertiesBuildRoot=[IO.Path]::GetFullPath($BuildRoot)
$propertiesRunRoot=[IO.Path]::GetFullPath($RunRoot)
if(Test-Path -LiteralPath $propertiesRunRoot){throw 'Use a new RunRoot; existing test results will not be overwritten.'}
if(!$SkipBuild){
 & (Join-Path $PSScriptRoot 'build.ps1') -PropertiesDialogUiTestsOnly -OutputRoot $propertiesBuildRoot -UseInstalledToolchain:$UseInstalledToolchain -ToolchainRoot $ToolchainRoot
}
$propertiesExe=Join-Path $propertiesBuildRoot 'release-native\Udm.PropertiesDialogUiTests.exe'
if(!(Test-Path -LiteralPath $propertiesExe)){throw 'Build the properties fixture before using -SkipBuild.'}
# This GUI executable can return asynchronously to PowerShell; wait for its process.
$propertiesProcess=Start-Process -FilePath $propertiesExe -ArgumentList ('"'+$propertiesRunRoot+'"') -WindowStyle Hidden -PassThru
$propertiesProcess.WaitForExit()
$propertiesResultPath=Join-Path $propertiesRunRoot 'results.json'
if(!(Test-Path -LiteralPath $propertiesResultPath)){throw 'Properties fixture did not write its acceptance result.'}
$propertiesResult=Get-Content -LiteralPath $propertiesResultPath -Raw | ConvertFrom-Json
$propertiesFailures=@($propertiesResult.checks | Where-Object { !$_.passed })
if($propertiesProcess.ExitCode -ne 0 -or !$propertiesResult.passed -or $propertiesFailures.Count){throw ('Properties fixture failed: '+$propertiesResult.error)}
[pscustomobject]@{Passed=@($propertiesResult.checks).Count;Failed=0;Evidence=$propertiesResultPath;ReusedBinary=[bool]$SkipBuild}
