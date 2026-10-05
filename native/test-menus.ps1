param(
 [Parameter(Mandatory=$true)][string]$BuildRoot,
 [Parameter(Mandatory=$true)][string]$RunRoot,
 [switch]$SkipBuild,
 [switch]$UseInstalledToolchain,
 [string]$ToolchainRoot
)
$ErrorActionPreference='Stop'
$menuBuildRoot=[IO.Path]::GetFullPath($BuildRoot)
$menuRunRoot=[IO.Path]::GetFullPath($RunRoot)
if(Test-Path -LiteralPath $menuRunRoot){throw 'Use a new RunRoot; existing test results will not be overwritten.'}
if(!$SkipBuild){
 & (Join-Path $PSScriptRoot 'build.ps1') -MenuStateTestsOnly -OutputRoot $menuBuildRoot -UseInstalledToolchain:$UseInstalledToolchain -ToolchainRoot $ToolchainRoot
}
$menuExe=Join-Path $menuBuildRoot 'release-native\Udm.MenuStateTests.exe'
if(!(Test-Path -LiteralPath $menuExe)){throw 'Build the menu fixture before using -SkipBuild.'}
# This GUI executable can return asynchronously to PowerShell; wait for its process.
$menuProcess=Start-Process -FilePath $menuExe -ArgumentList ('"'+$menuRunRoot+'"') -WindowStyle Hidden -PassThru
$menuProcess.WaitForExit()
$menuResultPath=Join-Path $menuRunRoot 'results.json'
if(!(Test-Path -LiteralPath $menuResultPath)){throw 'Menu fixture did not write its acceptance result.'}
$menuResult=Get-Content -LiteralPath $menuResultPath -Raw | ConvertFrom-Json
$menuFailures=@($menuResult.checks | Where-Object { !$_.passed })
if($menuProcess.ExitCode -ne 0 -or !$menuResult.passed -or $menuFailures.Count){throw ('Menu fixture failed: '+$menuResult.error)}
[pscustomobject]@{Passed=@($menuResult.checks).Count;Failed=0;Evidence=$menuResultPath;ReusedBinary=[bool]$SkipBuild}
