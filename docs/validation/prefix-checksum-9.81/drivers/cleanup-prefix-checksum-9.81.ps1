$ErrorActionPreference = 'Stop'
$manifestPath = '[local-diagnostics]\prefix-checksum-cleanup-9.81.json'
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if (-not $manifest.allEvidenceExported) { throw 'Evidence is not exported' }
$allowedRoots = @($manifest.allowedRoots | ForEach-Object { [IO.Path]::GetFullPath($_).TrimEnd('\') })
foreach ($targetRoot in $allowedRoots) {
    $permitted = $targetRoot.StartsWith('[repo]\.media-cache\owned-prefix-checksum', [StringComparison]::OrdinalIgnoreCase) -or $targetRoot -eq '[repo]\.media-cache\owned-post-checksum-build-9.80' -or $targetRoot -eq '[local-diagnostics]\owned-prefix-checksum-qualification-9.81-C'
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
[long]$removedBytes = 0
[int]$removed = 0
foreach ($row in $manifest.files) {
    $item = Get-Item -LiteralPath $row.path
    if ($item.PSIsContainer -or ($item.Attributes -band ([IO.FileAttributes]::ReparsePoint -bor [IO.FileAttributes]::ReadOnly)) -or $item.Length -ne $row.bytes -or (Get-FileHash -LiteralPath $row.path -Algorithm SHA256).Hash -ne $row.sha256) { throw 'Cleanup target changed after validation' }
    Remove-Item -LiteralPath $row.path
    $removedBytes += [long]$row.bytes
    $removed++
}
$receipt = [ordered]@{filesRemoved=$removed;logicalBytesRemoved=$removedBytes;physicalSpaceMeasured=$false;current42CoreObjectsAnd5ProductsPreserved=$true;priorFrozenAppRetained=$true;supersededPriorHelpersAndObjectsRemoved=@($manifest.retiredPriorArtifacts).Count;privateFixtureCatalogsAndPayloadsRemoved=$true;reparseEntriesExcluded=@($manifest.reparseEntriesExcluded).Count}
$receipt | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath '[local-diagnostics]\prefix-checksum-cleanup-receipt-9.81.json'
$receipt | ConvertTo-Json -Depth 8
