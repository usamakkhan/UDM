param(
 [Parameter(Mandatory=$true)][string]$Queue,
 [Parameter(Mandatory=$true)][string]$Collector,
 [Parameter(Mandatory=$true)][string]$CollectorSha256,
 [Parameter(Mandatory=$true)][string]$Area,
 [Parameter(Mandatory=$true)][string]$DataParent,
 [Parameter(Mandatory=$true)][string]$OwnerSid,
 [ValidateRange(1,480)][int]$SessionMinutes = 480,
 [switch]$ValidateOnly
)
$ErrorActionPreference = 'Stop'
function Hash([string]$Path) {
 $stream=[IO.File]::OpenRead($Path); $algorithm=[Security.Cryptography.SHA256]::Create()
 try { return [BitConverter]::ToString($algorithm.ComputeHash($stream)).Replace('-','') }
 finally { $algorithm.Dispose(); $stream.Dispose() }
}
function Full([string]$Path) { [IO.Path]::GetFullPath($Path).TrimEnd('\') }
function Ordinary([string]$Path) {
 $item = Get-Item -LiteralPath $Path -Force
 for ($p=$item; $null -ne $p; $p=$p.Parent) {
  if (($p.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw 'Reparse path refused' }
 }
 return $item
}
function Save([string]$Name, $Value) {
 $target = Join-Path $Queue $Name
 $text = $Value | ConvertTo-Json -Depth 12
 [IO.File]::WriteAllText($target, $text, [Text.UTF8Encoding]::new($false))
}
$Queue=Full $Queue; $Collector=Full $Collector; $Area=Full $Area; $DataParent=Full $DataParent
$null=Ordinary $Queue; $null=Ordinary $Collector; $null=Ordinary $Area; $null=Ordinary $DataParent
if (!(Test-Path -LiteralPath (Join-Path $Queue '.udm-diagnostic-worker'))) { throw 'Missing worker ownership marker' }
if ((Hash $Collector) -ne $CollectorSha256) { throw 'Collector hash mismatch' }
if ([IO.Path]::GetDirectoryName([IO.Path]::GetDirectoryName($Collector)) -ne $Area) { throw 'Collector outside declared area' }
if ($ValidateOnly) { 'Diagnostic worker preflight passed'; exit 0 }
$identity=[Security.Principal.WindowsIdentity]::GetCurrent()
if (!([Security.Principal.WindowsPrincipal]::new($identity)).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) { throw 'Administrator token required' }
$expires=[DateTime]::UtcNow.AddMinutes($SessionMinutes); $count=0
Save 'ready.json' @{ pid=$PID; expiresUtc=$expires.ToString('o'); sessionMinutes=$SessionMinutes; maxTraceRequests=20; scope='hash-pinned realtime diagnostic collector only'; persistent=$false }
try {
 while ([DateTime]::UtcNow -lt $expires -and $count -lt 20) {
  if (Test-Path -LiteralPath (Join-Path $Queue 'stop.flag')) { break }
  foreach ($file in @(Get-ChildItem -LiteralPath $Queue -Filter 'request-*.json' -File)) {
   if ($file.Name -notmatch '^request-([0-9a-f]{32})\.json$') { continue }
   $id=$Matches[1]; $reply='response-'+$id+'.json'
   if (Test-Path -LiteralPath (Join-Path $Queue $reply)) { continue }
   ++$count
   try {
    $null=Ordinary $file.FullName
    if ($file.Length -gt 4096) { throw 'Request too large' }
    $r=Get-Content -LiteralPath $file.FullName -Raw | ConvertFrom-Json
    if ($r.action -ne 'trace') { throw 'Unknown operation; arbitrary commands are not supported' }
    $capture=Full ([string]$r.captureRoot); $data=Full ([string]$r.dataRoot); $prefix=[string]$r.prefix
    if ([IO.Path]::GetDirectoryName($capture) -ne $Area -or [IO.Path]::GetFileName($capture) -notmatch '^live-[0-9]+$') { throw 'Capture outside owned area' }
    if ([IO.Path]::GetDirectoryName($data) -ne $DataParent -or [IO.Path]::GetFileName($data) -notmatch '^owned-realtime-8\.58-[0-9a-f]{32}$') { throw 'Data outside owned area' }
    $null=Ordinary $capture; $null=Ordinary $data
    if (!(Test-Path -LiteralPath (Join-Path $capture '.udm-lifetime-capture')) -or !(Test-Path -LiteralPath (Join-Path $data '.udm-lifetime-data'))) { throw 'Missing fixture ownership markers' }
    if ($prefix -notmatch '^u[0-9a-f]{8}$') { throw 'Invalid target image prefix' }
    $parentId=[int]$r.controllerPid
    if ($parentId -le 0) { throw 'Invalid controller' }
    $process=Get-CimInstance Win32_Process -Filter "ProcessId=$parentId"
    if (!$process) { throw 'Controller has exited' }
    $sid=Invoke-CimMethod -InputObject $process -MethodName GetOwnerSid
    if ($sid.ReturnValue -ne 0 -or $sid.Sid -ne $OwnerSid) { throw 'Controller belongs to another account' }
    if ((Hash $Collector) -ne $CollectorSha256) { throw 'Collector changed; new approval required' }
    # Exactly four data arguments; none is interpreted as PowerShell code.
    $arguments=@(('"'+$capture+'"'), [string]$parentId, $prefix, ('"'+$data+'"'))
    $child=Start-Process -FilePath $Collector -ArgumentList $arguments -WindowStyle Hidden -PassThru
    Save $reply @{ accepted=$true; pid=$child.Id; collectorSha256=$CollectorSha256 }
    # Keep one trace at a time. Collector owns its stop/deadline cleanup.
    $child.WaitForExit()
    Save ('exit-'+$id+'.json') @{ pid=$child.Id; exitCode=$child.ExitCode }
   } catch { Save $reply @{ accepted=$false; error=$_.Exception.Message } }
  }
  Start-Sleep -Milliseconds 200
 }
} finally { Save 'stopped.json' @{ pid=$PID; requests=$count; endedUtc=[DateTime]::UtcNow.ToString('o'); persistent=$false } }
