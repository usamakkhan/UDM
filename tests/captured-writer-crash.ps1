param([Parameter(Mandatory=$true)][string]$Executable,[Parameter(Mandatory=$true)][string]$RunRoot)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $RunRoot){throw 'Use a fresh crash-test directory'}
New-Item -ItemType Directory -Path $RunRoot|Out-Null
$results=@()
foreach($mode in @('partial','complete')){
 $data=Join-Path $RunRoot $mode
 $writer=Start-Process -FilePath $Executable -ArgumentList @('--crash-writer',('"'+$data+'"'),$mode) -WindowStyle Hidden -PassThru
 try{
  $deadline=[DateTime]::UtcNow.AddSeconds(15)
  while(!(Test-Path -LiteralPath (Join-Path $data 'writer-ready.json'))){if($writer.HasExited){throw 'Writer exited before durable checkpoint'};if([DateTime]::UtcNow -gt $deadline){throw 'Writer checkpoint timed out'};Start-Sleep -Milliseconds 100}
  $ready=Get-Content (Join-Path $data 'writer-ready.json') -Raw|ConvertFrom-Json
  if($ready.pid -ne $writer.Id -or $writer.HasExited){throw 'Writer identity changed'}
  $writer.Kill();if(!$writer.WaitForExit(5000)){throw 'Owned writer did not exit'}
  $output=& $Executable --recover-crashed-writer $data $mode
  $code=$LASTEXITCODE;$output|Set-Content (Join-Path $RunRoot ($mode+'.log'))
  $results+=@{mode=$mode;writerPid=$writer.Id;exitCode=$code;passed=($code -eq 0);output=@($output)}
  if($code){throw 'Writer recovery checks failed'}
 }finally{if(!$writer.HasExited){$writer.Kill();$writer.WaitForExit(5000)|Out-Null};$writer.Dispose();$results|ConvertTo-Json -Depth 5|Set-Content (Join-Path $RunRoot 'results.json')}
}
$results|ConvertTo-Json -Depth 5