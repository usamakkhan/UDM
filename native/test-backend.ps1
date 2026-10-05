param(
 [string]$OutputRoot=(Join-Path ([IO.Path]::GetTempPath()) 'UDM-backend-build'),
 [string]$ToolchainRoot,
 [switch]$UseInstalledToolchain,
 [switch]$FocusedOnly,
 [string[]]$Case=@()
)
$ErrorActionPreference='Stop'
if(![IO.Path]::IsPathRooted($OutputRoot)){throw 'OutputRoot must be an absolute path.'}
$buildArguments=@{OutputRoot=$OutputRoot;UseInstalledToolchain=$UseInstalledToolchain}
if($ToolchainRoot){$buildArguments.ToolchainRoot=$ToolchainRoot}
$cases=@(Get-Content -Raw -LiteralPath (Join-Path $PSScriptRoot 'backend-tests.json') | ConvertFrom-Json)
if($Case.Count){
 if(!$FocusedOnly){throw 'Use -Case with -FocusedOnly for a targeted run.'}
 foreach($name in $Case){if(!@($cases|Where-Object{(($_.name,$_.source)|Where-Object{$_}|Select-Object -First 1) -eq $name}).Count){throw "Unknown backend case: $name"}}
 $cases=@($cases|Where-Object{ $name=if($_.name){$_.name}else{$_.source};$name -in $Case })
}
$runRoot=Join-Path $OutputRoot ('backend-results\'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force -Path $runRoot | Out-Null
$results=[Collections.Generic.List[object]]::new()
$report=Join-Path $runRoot 'summary.json'
function Save-Report {
 [PSCustomObject]@{passed=($results.Count -gt 0 -and @($results|Where-Object{!$_.passed}).Count -eq 0);focusedOnly=[bool]$FocusedOnly;results=@($results.ToArray())} |
  ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $report -Encoding utf8
}
function Run-Check([string]$Name,[string]$Executable,[string[]]$Arguments,[int]$TimeoutSeconds,[string]$ResultPath='') {
 $stdout=Join-Path $runRoot ($Name+'.stdout.log');$stderr=Join-Path $runRoot ($Name+'.stderr.log')
 # Generated arguments contain no quotes or trailing backslashes; quote each
 # independently so the runner also works under Windows accounts with spaces.
 foreach($argument in $Arguments){if($argument.Contains('"') -or $argument.EndsWith('\')){throw 'Unsupported test argument.'}}
 $quoted=@($Arguments|ForEach-Object{'"'+$_+'"'})
 $start=@{FilePath=$Executable;PassThru=$true;WindowStyle='Hidden';RedirectStandardOutput=$stdout;RedirectStandardError=$stderr}
 if($quoted.Count){$start.ArgumentList=$quoted}
 $process=Start-Process @start
 $timedOut=!$process.WaitForExit($TimeoutSeconds*1000)
 if($timedOut){$process.Kill($true)}
 $process.WaitForExit();$exitCode=$process.ExitCode;$process.Dispose()
 $passed=!$timedOut -and $exitCode -eq 0;$checks=0;$errorMessage=''
 if($ResultPath){
  if(!(Test-Path -LiteralPath $ResultPath)){$passed=$false;$errorMessage='Test did not write its result file.'}
  else{
   $detail=Get-Content -Raw -LiteralPath $ResultPath|ConvertFrom-Json
   if($detail.passed -is [bool]){$passed=$passed -and $detail.passed}else{$checks=[int]$detail.passed;$passed=$passed -and $detail.failed -eq 0}
   if($detail.checks){$checks=@($detail.checks).Count;$passed=$passed -and @($detail.checks|Where-Object{!$_.passed}).Count -eq 0}
   if($detail.error){$passed=$false;$errorMessage=$detail.error}
  }
 }else{
  $text=Get-Content -Raw -LiteralPath $stdout
  $matches=[regex]::Matches($text,'(?m)^(\d+) passed, (\d+) failed\s*$')
  if(!$matches.Count){$passed=$false;$errorMessage='Native suite did not report a final count.'}
  else{$last=$matches[$matches.Count-1];$checks=[int]$last.Groups[1].Value;$passed=$passed -and [int]$last.Groups[2].Value -eq 0}
 }
 $results.Add([PSCustomObject]@{name=$Name;passed=$passed;checks=$checks;exitCode=$exitCode;timedOut=$timedOut;error=$errorMessage;stdout=$stdout;stderr=$stderr;result=$ResultPath})
 Save-Report
 Write-Output "$Name : passed=$passed checks=$checks"
}
try {
 if(!$FocusedOnly){
  & (Join-Path $PSScriptRoot 'build.ps1') @buildArguments -TestsOnly *> (Join-Path $runRoot 'native-build.log')
 }
 & (Join-Path $PSScriptRoot 'build.ps1') @buildArguments -BackendTestsOnly -BackendTestNames @($cases.source|Sort-Object -Unique) *> (Join-Path $runRoot 'backend-build.log')
 $bin=Join-Path $OutputRoot 'release-native'
 if(!$FocusedOnly){
  $tools=Join-Path $bin 'tools';New-Item -ItemType Directory -Force -Path $tools|Out-Null
  foreach($name in @('ffmpeg.exe','ffprobe.exe','FFmpeg-LICENSE.txt')){
   $source=Join-Path (Split-Path $PSScriptRoot -Parent) ('release\tools\'+$name)
   if(!(Test-Path -LiteralPath $source)){throw "Missing media test dependency: $source"}
   Copy-Item -LiteralPath $source -Destination $tools -Force
  }
  Run-Check 'NativeSuite' (Join-Path $bin 'Udm.NativeTests.exe') @('--test-root',(Join-Path $runRoot 'native-data')) 1800
 }
 foreach($testCase in $cases){
  $name=if($testCase.name){$testCase.name}else{$testCase.source}
  $data=Join-Path $runRoot $name
  $arguments=@();if($testCase.arguments){$arguments+=@($testCase.arguments)};$arguments+=$data
  Run-Check $name (Join-Path $bin ('Udm.'+$testCase.source+'.exe')) $arguments $testCase.timeoutSeconds (Join-Path $data 'results.json')
 }
 if(@($results|Where-Object{!$_.passed}).Count){throw "Backend checks failed. Review $report"}
 Write-Output "All backend checks passed. Evidence: $report"
}catch{
 $results.Add([PSCustomObject]@{name='Runner';passed=$false;error=$_.Exception.Message})
 Save-Report
 throw
}
