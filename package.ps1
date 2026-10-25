param([string]$Version,[string]$CompilerPath,[string]$SignThumbprint,[string]$SignPfxPath,[securestring]$SignPfxPassword,[string]$TrustedSigningMetadata,[string]$TrustedSigningDlib,[string]$TimestampUrl,[string]$SignToolPath,[switch]$SignDryRun,[switch]$NoTimestamp)
$ErrorActionPreference = 'Stop'
# Omitted arguments use the installer builder's canonical defaults.
& (Join-Path $PSScriptRoot 'installer\build-setup.ps1') @PSBoundParameters
