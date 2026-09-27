param([string]$Version = '0.40.0',[string]$CompilerPath)
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'installer\build-setup.ps1') -Version $Version -CompilerPath $CompilerPath
