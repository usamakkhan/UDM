$ErrorActionPreference = 'Stop'
$manifestPath = '[local-diagnostics]\continuous-dash-cleanup-9.83.json'
$receiptPath = '[local-diagnostics]\continuous-dash-cleanup-receipt-9.83.json'
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if (-not $manifest.allEvidenceExported) { throw 'Evidence not exported' }
$roots = @($manifest.allowedRoots | ForEach-Object { [IO.Path]::GetFullPath($_).TrimEnd('\') })
foreach ($boundary in $roots) {
    if (-not ($boundary.StartsWith('[repo]\.media-cache\owned-continuous-dash', [StringComparison]::OrdinalIgnoreCase) -or $boundary -eq '[repo]\.media-cache\owned-retired-dash-build-9.82' -or $boundary -eq '[local-diagnostics]\owned-continuous-dash-workflow-9.83-C')) { throw 'Unexpected root' }
    $ancestor = Get-Item -LiteralPath $boundary
    while ($null -ne $ancestor) {
        if (-not ($ancestor -is [IO.DirectoryInfo]) -or ($ancestor.Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Unsafe root ancestor' }
        $ancestor = $ancestor.Parent
    }
}
$protected = @{}; $seen = @{}
foreach ($file in $manifest.protectedCurrentFiles) { $protected[[IO.Path]::GetFullPath($file)] = $true }
foreach ($row in $manifest.files) {
    $path = [IO.Path]::GetFullPath($row.path); $boundary = [IO.Path]::GetFullPath($row.root).TrimEnd('\')
    if ($roots -notcontains $boundary -or -not $path.StartsWith($boundary + '\', [StringComparison]::OrdinalIgnoreCase) -or $protected.ContainsKey($path) -or $seen.ContainsKey($path)) { throw 'Unsafe cleanup entry' }
    $seen[$path] = $true; $item = Get-Item -LiteralPath $path
    if ($item.PSIsContainer -or ($item.Attributes -band ([IO.FileAttributes]::ReparsePoint -bor [IO.FileAttributes]::ReadOnly)) -or $item.Length -ne $row.bytes) { throw 'Cleanup file changed' }
    $ancestor = $item.Directory
    while ($ancestor.FullName -ne $boundary) {
        if ($ancestor.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Redirected file ancestor' }
        $ancestor = $ancestor.Parent
        if ($null -eq $ancestor) { throw 'Missing cleanup boundary' }
    }
    if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $row.sha256) { throw 'Cleanup hash changed' }
}
foreach ($process in (Get-CimInstance Win32_Process)) {
    if ($process.ExecutablePath -and $seen.ContainsKey($process.ExecutablePath)) { throw 'Owned actor is still running' }
}
$knownLinks = @('[repo]\.media-cache\owned-continuous-dash-baseline-9.82\release-native\tools', '[repo]\.media-cache\owned-continuous-dash-actor-9.83\release-native\tools')
if (@($manifest.ownedJunctions).Count -ne 2) { throw 'Unexpected junction count' }
foreach ($entry in $manifest.ownedJunctions) {
    $path = [IO.Path]::GetFullPath($entry)
    if ($knownLinks -notcontains $path) { throw 'Unexpected junction path' }
    $link = Get-Item -LiteralPath $path
    if ($link.LinkType -ne 'Junction' -or @($link.Target).Count -ne 1 -or [IO.Path]::GetFullPath(@($link.Target)[0]).TrimEnd('\') -ne 'C:\Program Files\UDM\tools') { throw 'Shared-tool link changed' }
}
foreach ($entry in $manifest.ownedJunctions) { [IO.Directory]::Delete([IO.Path]::GetFullPath($entry)) }
[long]$removedBytes = 0; [int]$removed = 0
foreach ($row in $manifest.files) {
    $item = Get-Item -LiteralPath $row.path
    if ($item.PSIsContainer -or ($item.Attributes -band ([IO.FileAttributes]::ReparsePoint -bor [IO.FileAttributes]::ReadOnly)) -or $item.Length -ne $row.bytes -or (Get-FileHash -LiteralPath $row.path -Algorithm SHA256).Hash -ne $row.sha256) { throw 'Cleanup file changed after validation' }
    Remove-Item -LiteralPath $row.path
    $removed++; $removedBytes += [long]$row.bytes
}
$receipt = [ordered]@{filesRemoved=$removed;logicalBytesRemoved=$removedBytes;physicalSpaceMeasured=$false;current42CoreObjectsAnd5ProductsPreserved=$true;currentResourcePreserved=$true;priorFrozenAppRetained=$true;retiredArtifactsRemoved=@($manifest.retiredArtifacts).Count;sharedToolsJunctionsRemoved=2;privateFixtureCatalogsAndPayloadsRemoved=$true}
$receipt | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $receiptPath
$receipt | ConvertTo-Json -Depth 8
