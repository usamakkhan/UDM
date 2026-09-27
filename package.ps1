param([string]$Version = '0.34.0')
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'installer\build-setup.ps1') -Version $Version
