# Read-only diagnostics. No service, trust store or boot setting is changed.
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$out=Join-Path $PSScriptRoot 'out\admin-readiness.json'
$report=[ordered]@{checkedUtc=[DateTime]::UtcNow.ToString('o');administrator=$false;monitor=$null;secureBoot=$null;secureBootError=$null;virtualMachineCount=$null;virtualMachineError=$null;error=$null}
try{
    $report.administrator=[Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
    $report.monitor=(& (Join-Path $root 'release\Udm.Monitor.exe') --diagnose | ConvertFrom-Json)
    try{$report.secureBoot=Confirm-SecureBootUEFI}catch{$report.secureBootError=$_.Exception.Message}
    try{$report.virtualMachineCount=@(Get-VM -ErrorAction Stop).Count}catch{$report.virtualMachineError=$_.Exception.Message}
}catch{$report.error=$_.Exception.Message}
$report | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $out
