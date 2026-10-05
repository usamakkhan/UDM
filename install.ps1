param([string]$Destination = (Join-Path $env:LOCALAPPDATA 'Programs\UDM'), [switch]$StartMenu, [switch]$MediaTools)
$ErrorActionPreference = 'Stop'
$target = [IO.Path]::GetFullPath($Destination)
# Validate the complete requested payload before creating or changing an install.
$requiredFiles = @('UDM.exe','Udm.NativeHost.exe','Udm.Monitor.exe')
foreach ($name in $requiredFiles) {
    if (-not (Test-Path -LiteralPath (Join-Path $PSScriptRoot "release\$name") -PathType Leaf)) {
        throw ('Build UDM first with .\build.ps1. Missing payload: ' + $name)
    }
}
$sourceAssets = Join-Path $PSScriptRoot 'assets'
if (-not (Test-Path -LiteralPath $sourceAssets -PathType Container)) { throw 'The UDM assets directory is missing.' }
if ($MediaTools) {
    foreach ($name in @('ffmpeg.exe','ffprobe.exe','FFmpeg-LICENSE.txt')) {
        if (-not (Test-Path -LiteralPath (Join-Path $PSScriptRoot "release\tools\$name") -PathType Leaf)) {
            throw ('Run setup-media.ps1 before installing with -MediaTools. Missing payload: ' + $name)
        }
    }
}
New-Item -ItemType Directory -Force -Path $target | Out-Null
foreach ($name in $requiredFiles) {
    $source = Join-Path $PSScriptRoot "release\$name"
    if (-not (Test-Path -LiteralPath $source)) { throw 'Build UDM first with .\build.ps1.' }
    if ((Join-Path $target $name) -ne $source) { Copy-Item -LiteralPath $source -Destination (Join-Path $target $name) -Force }
}
$sourceAssets = Join-Path $PSScriptRoot 'assets'
if ([IO.Path]::GetFullPath((Join-Path $target 'assets')) -ne [IO.Path]::GetFullPath($sourceAssets)) {
    Copy-Item -LiteralPath $sourceAssets -Destination $target -Recurse -Force
}
if ($MediaTools) {
    $sourceTools = Join-Path $PSScriptRoot 'release\tools'
    if (-not (Test-Path -LiteralPath (Join-Path $sourceTools 'ffmpeg.exe'))) { throw 'Run setup-media.ps1 before installing with -MediaTools.' }
    $targetTools = Join-Path $target 'tools'
    if ([IO.Path]::GetFullPath($sourceTools) -ne [IO.Path]::GetFullPath($targetTools)) {
        New-Item -ItemType Directory -Force -Path $targetTools | Out-Null
        foreach ($mediaFile in @('ffmpeg.exe','ffprobe.exe','FFmpeg-LICENSE.txt')) {
            Copy-Item -LiteralPath (Join-Path $sourceTools $mediaFile) -Destination $targetTools -Force
        }
    }
}
$exe = Join-Path $target 'UDM.exe'
$scheme = 'HKCU:\Software\Classes\udm'
New-Item -Path "$scheme\shell\open\command" -Force | Out-Null
Set-Item -LiteralPath $scheme -Value 'URL:UDM Download Manager'
New-ItemProperty -LiteralPath $scheme -Name 'URL Protocol' -Value '' -Force | Out-Null
Set-Item -LiteralPath "$scheme\shell\open\command" -Value ('"' + $exe + '" "%1"')
if ($StartMenu) {
    $shortcutPath = Join-Path ([Environment]::GetFolderPath('Programs')) 'UDM Download Manager.lnk'
    $shell = New-Object -ComObject WScript.Shell
    $shortcut = $shell.CreateShortcut($shortcutPath)
    $shortcut.TargetPath = $exe
    $shortcut.WorkingDirectory = $target
    $shortcut.Save()
}
Write-Output "Installed for the current user at $target. Browser extension registration is a separate step."
