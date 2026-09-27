param([string]$Version = '0.31.0')
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'installer\build-setup.ps1') -Version $Version
