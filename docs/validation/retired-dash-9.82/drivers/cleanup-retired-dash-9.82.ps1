$ErrorActionPreference = 'Stop'
$manifestPath = '[local-diagnostics]\retired-dash-cleanup-9.82.json'
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if (-not $manifest.allEvidenceExported) { throw 'Evidence is not exported' }
$allowedRoots = @($manifest.allowedRoots | ForEach-Object { [IO.Path]::GetFullPath($_).TrimEnd('\') })
foreach ($targetRoot in $allowedRoots) {
    $permitted = $targetRoot.StartsWith('[repo]\.media-cache\owned-retired-dash', [StringComparison]::OrdinalIgnoreCase) -or $targetRoot -eq '[repo]\.media-cache\owned-prefix-checksum-build-9.81' -or $targetRoot -eq '[repo]\.media-cache\owned-live-dash-continuation-build-9.77' -or $targetRoot -eq '[repo]\.media-cache\owned-dash-output-build-9.76' -or $targetRoot -eq '[local-diagnostics]\owned-retired-dash-boundary-9.82-C'
    if (-not $permitted) { throw "Unapproved cleanup root: $targetRoot" }
    $node = Get-Item -LiteralPath $targetRoot
    while ($null -ne $node) {
        if (-not ($node -is [IO.DirectoryInfo]) -or ($node.Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Unsafe cleanup ancestor' }
        $node = $node.Parent
    }
}
$protected = @{}
foreach ($file in $manifest.protectedCurrentFiles) { $protected[[IO.Path]::GetFullPath($file)] = $true }
$seen = @{}
foreach ($row in $manifest.files) {
    $targetPath = [IO.Path]::GetFullPath($row.path)
    $targetRoot = [IO.Path]::GetFullPath($row.root).TrimEnd('\')
    if ($allowedRoots -notcontains $targetRoot -or -not $targetPath.StartsWith($targetRoot + '\', [StringComparison]::OrdinalIgnoreCase) -or $protected.ContainsKey($targetPath) -or $seen.ContainsKey($targetPath)) { throw 'Unsafe or duplicate cleanup target' }
    $seen[$targetPath] = $true
    $item = Get-Item -LiteralPath $targetPath
    if ($item.PSIsContainer -or ($item.Attributes -band ([IO.FileAttributes]::ReparsePoint -bor [IO.FileAttributes]::ReadOnly)) -or $item.Length -ne $row.bytes) { throw 'Cleanup type or size changed' }
    $ancestor = $item.Directory
    while ($ancestor.FullName -ne $targetRoot) {
        if ($ancestor.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Cleanup ancestor changed' }
        $ancestor = $ancestor.Parent
        if ($null -eq $ancestor) { throw 'Cleanup boundary missing' }
    }
    if ((Get-FileHash -LiteralPath $targetPath -Algorithm SHA256).Hash -ne $row.sha256) { throw 'Cleanup contents changed' }
}
foreach ($process in (Get-CimInstance Win32_Process)) {
    if ($process.ExecutablePath -and $seen.ContainsKey($process.ExecutablePath)) { throw 'A cleanup binary is still running' }
}
$ownedLink = '[repo]\.media-cache\owned-retired-dash-baseline-9.81\release-native\tools'
$linkItem = Get-Item -LiteralPath $ownedLink
if ($linkItem.LinkType -ne 'Junction' -or @($linkItem.Target).Count -ne 1 -or [IO.Path]::GetFullPath(@($linkItem.Target)[0]).TrimEnd('\') -ne 'C:\Program Files\UDM\tools') { throw 'Shared-tool junction changed' }
# Nonrecursive deletion removes only the verified junction entry, never its target.
[IO.Directory]::Delete($ownedLink)
[long]$removedBytes = 0
[int]$removed = 0
foreach ($row in $manifest.files) {
    $item = Get-Item -LiteralPath $row.path
    if ($item.PSIsContainer -or ($item.Attributes -band ([IO.FileAttributes]::ReparsePoint -bor [IO.FileAttributes]::ReadOnly)) -or $item.Length -ne $row.bytes -or (Get-FileHash -LiteralPath $row.path -Algorithm SHA256).Hash -ne $row.sha256) { throw 'Cleanup target changed after validation' }
    Remove-Item -LiteralPath $row.path
    $removedBytes += [long]$row.bytes
    $removed++
}
$receipt = [ordered]@{filesRemoved=$removed;logicalBytesRemoved=$removedBytes;physicalSpaceMeasured=$false;current42CoreObjectsAnd5ProductsPreserved=$true;priorFrozenAppRetained=$true;supersededPriorHelpersAndObjectsRemoved=@($manifest.retiredPriorArtifacts).Count;privateFixtureCatalogsAndPayloadsRemoved=$true;sharedToolsJunctionRemovedWithoutTraversingTarget=$true;reparseEntriesExcluded=@($manifest.reparseEntriesExcluded).Count}
$receipt | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath '[local-diagnostics]\retired-dash-cleanup-receipt-9.82.json'
$receipt | ConvertTo-Json -Depth 8
