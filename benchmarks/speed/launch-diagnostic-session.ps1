param(
 [Parameter(Mandatory=$true)][string]$WorkerScript,
 [Parameter(Mandatory=$true)][string]$Queue,
 [Parameter(Mandatory=$true)][string]$Collector,
 [Parameter(Mandatory=$true)][string]$CollectorSha256,
 [Parameter(Mandatory=$true)][string]$Area,
 [Parameter(Mandatory=$true)][string]$DataParent,
 [Parameter(Mandatory=$true)][string]$OwnerSid,
 [ValidateRange(1,480)][int]$SessionMinutes=480
)
$ErrorActionPreference='Stop'
$Queue=[IO.Path]::GetFullPath($Queue)
$WorkerScript=[IO.Path]::GetFullPath($WorkerScript)
if ([IO.Path]::GetDirectoryName($WorkerScript) -ne $Queue -or [IO.Path]::GetFileName($WorkerScript) -ne 'worker.ps1') { throw 'Worker outside queue' }
if (!(Test-Path -LiteralPath (Join-Path $Queue '.udm-diagnostic-worker')) -or !(Test-Path -LiteralPath $WorkerScript -PathType Leaf)) { throw 'Missing owned worker' }
$outcome=Join-Path $Queue 'elevation.json'
if (Test-Path -LiteralPath $outcome) { throw 'Existing elevation outcome; inspect original request' }
$arguments=@('-NoProfile','-File',$WorkerScript,'-Queue',$Queue,'-Collector',$Collector,'-CollectorSha256',$CollectorSha256,'-Area',$Area,'-DataParent',$DataParent,'-OwnerSid',$OwnerSid,'-SessionMinutes',[string]$SessionMinutes)
# These are argv values for -File, never interpolated into PowerShell code.
foreach ($value in $arguments) { if ($value.Contains('"')) { throw 'Quote in diagnostic argument' } }
$quoted=@($arguments | ForEach-Object { '"'+$_+'"' })
try {
 $process=Start-Process -FilePath (Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe') -ArgumentList $quoted -Verb RunAs -WindowStyle Hidden -PassThru -ErrorAction Stop
 $workerId=0
 if ($null -eq $process -or ![int]::TryParse([string]$process.Id,[ref]$workerId) -or $workerId -le 0) { throw 'Elevation returned no process ID; inspect readiness before retrying' }
 [IO.File]::WriteAllText($outcome,(@{state='process-created';pid=$workerId;readyObserved=$false}|ConvertTo-Json),[Text.UTF8Encoding]::new($false))
 Write-Output $workerId
} catch {
 $exception=$_.Exception; $nativeCode=$null
 for ($cause=$exception; $null -ne $cause; $cause=$cause.InnerException) {
  if ($cause -is [ComponentModel.Win32Exception]) { $nativeCode=$cause.NativeErrorCode; break }
 }
 $state=if ($nativeCode -eq 1223) { 'canceled' } else { 'failed-or-indeterminate' }
 [IO.File]::WriteAllText($outcome,(@{state=$state;pid=$null;nativeErrorCode=$nativeCode;error=$exception.Message;readyObserved=$false}|ConvertTo-Json),[Text.UTF8Encoding]::new($false))
 throw
}
