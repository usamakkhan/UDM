param([string]$Version,[string]$CompilerPath)
$ErrorActionPreference = 'Stop'
# Omitted arguments use the installer builder's canonical defaults.
& (Join-Path $PSScriptRoot 'installer\build-setup.ps1') @PSBoundParameters
