param([string]$Version = '0.15.0')
$ErrorActionPreference = 'Stop'
if ($Version -notmatch '^\d+\.\d+(\.\d+)?$') { throw 'Use a numeric release version.' }
$staging = Join-Path $PSScriptRoot ('package\stage-' + [Guid]::NewGuid().ToString('N'))
$packageRoot = Join-Path $staging ('UDM-' + $Version)
New-Item -ItemType Directory -Path $packageRoot -Force | Out-Null
foreach ($folder in @('src','assets','browser','docs')) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot $folder) -Destination $packageRoot -Recurse -Force
}
# Package reproducible test sources, excluding locally compiled harnesses and artifacts.
$testSource = Join-Path $PSScriptRoot 'tests'
Get-ChildItem -LiteralPath $testSource -Recurse -File | Where-Object { $_.Extension -in @('.cpp','.hpp','.cs','.cjs','.js','.ps1','.md','.json') } | ForEach-Object {
    $relative = $_.FullName.Substring($testSource.Length + 1)
    $destination = Join-Path (Join-Path $packageRoot 'tests') $relative
    New-Item -ItemType Directory -Force -Path (Split-Path $destination -Parent) | Out-Null
    Copy-Item -LiteralPath $_.FullName -Destination $destination
}
$nativeSource = Join-Path $packageRoot 'native'
New-Item -ItemType Directory -Path $nativeSource -Force | Out-Null
Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot 'native') -File | Where-Object { $_.Extension -in @('.cpp','.hpp','.rc','.manifest','.ps1','.md','.cjs') } | Copy-Item -Destination $nativeSource
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'native\third_party') -Destination $nativeSource -Recurse -Force
$driverSource = Join-Path $packageRoot 'drivers'
New-Item -ItemType Directory -Path $driverSource -Force | Out-Null
Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot 'drivers') -File | Where-Object { $_.Extension -in @('.c','.cs','.h','.inf','.vcxproj','.ps1','.md') } | Copy-Item -Destination $driverSource
foreach ($file in @('README.md','build.ps1','build-legacy.ps1','setup-media.ps1','install.ps1','package.ps1','.gitignore')) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot $file) -Destination $packageRoot -Force
}
$packageRelease = Join-Path $packageRoot 'release'
New-Item -ItemType Directory -Path $packageRelease -Force | Out-Null
foreach ($exe in @('UDM.exe','Udm.NativeHost.exe','Udm.Monitor.exe')) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot ('release\' + $exe)) -Destination $packageRelease -Force
}
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'assets') -Destination $packageRelease -Recurse -Force
# Downloaded tools, cache, user history, partial transfers and old packages are excluded.
$archive = Join-Path $PSScriptRoot ('UDM-' + $Version + '-source-and-portable.zip')
Compress-Archive -LiteralPath $packageRoot -DestinationPath $archive -Force
Write-Output $archive
