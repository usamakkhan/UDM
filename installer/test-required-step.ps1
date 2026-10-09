param(
 [Parameter(Mandatory=$true)][string]$CompilerPath,
 [Parameter(Mandatory=$true)][string]$RunRoot,
 [ValidateSet('success','exit','missing')][string[]]$Cases=@('success','exit','missing'),
 [ValidateSet('prepare','postinstall')][string[]]$Phases=@('prepare','postinstall')
)
$ErrorActionPreference='Stop'
$RunRoot=[IO.Path]::GetFullPath($RunRoot)
if(Test-Path -LiteralPath $RunRoot){throw 'Use a new isolated RunRoot.'}
if(-not (Test-Path -LiteralPath $CompilerPath -PathType Leaf)){throw 'Inno Setup compiler is missing.'}
New-Item -ItemType Directory -Path $RunRoot | Out-Null
$checks=[Collections.Generic.List[object]]::new()
& $CompilerPath ('/O'+$RunRoot) ('/FRequiredStepFixture') (Join-Path $PSScriptRoot 'tests\RequiredStepFixture.iss') | Set-Content -LiteralPath (Join-Path $RunRoot 'build.log') -Encoding UTF8
if($LASTEXITCODE){throw 'Private fixture compilation failed.'}
$fixture=Join-Path $RunRoot 'RequiredStepFixture.exe'
$privateTemp=Join-Path $RunRoot 'temp';New-Item -ItemType Directory -Path $privateTemp | Out-Null
foreach($phase in $Phases){ foreach($case in $Cases){
 $receipt=Join-Path $RunRoot ($phase+'-'+$case+'.txt');$log=Join-Path $RunRoot ($phase+'-'+$case+'.log')
 $arguments='/VERYSILENT /SUPPRESSMSGBOXES /SP- /NORESTART /NOCLOSEAPPLICATIONS /NORESTARTAPPLICATIONS /NOICONS /TESTPHASE='+$phase+' /TESTMODE='+$case+' /RESULTFILE="'+$receipt+'" /LOG="'+$log+'"'
 $watch=[Diagnostics.Stopwatch]::StartNew()
 $previousTemp=$env:TEMP;$previousTmp=$env:TMP
 try{$env:TEMP=$privateTemp;$env:TMP=$privateTemp;$process=Start-Process -FilePath $fixture -ArgumentList $arguments -WindowStyle Hidden -PassThru}
 finally{$env:TEMP=$previousTemp;$env:TMP=$previousTmp}
 $finished=$process.WaitForExit(15000)
 if(-not $finished){
  # This is only the private fixture process tree started immediately above.
  & taskkill.exe /PID $process.Id /T /F | Out-Null
  if(-not $process.WaitForExit(5000)){throw 'Private fixture did not terminate after its deadline.'}
 }
 $watch.Stop();$process.Refresh();$code=$process.ExitCode
 $actual=if(Test-Path -LiteralPath $receipt){[IO.File]::ReadAllText($receipt)}else{''}
 $expected=if($case -eq 'success'){'true'}else{'false'}
 $launchObserved=Test-Path -LiteralPath ($receipt+'.launch')
 $expectedCode=if($case -eq 'success'){0}elseif($phase -eq 'prepare'){7}else{20}
 $passed=$finished -and $actual -eq $expected -and ($code -eq $expectedCode) -and ($launchObserved -eq ($case -eq 'success'))
 $checks.Add([pscustomobject]@{phase=$phase;case=$case;passed=$passed;timedOut=(-not $finished);exitCode=$code;launchObserved=$launchObserved;receipt=$actual;elapsedMs=$watch.ElapsedMilliseconds})
 $process.Dispose()
}
}
$result=[ordered]@{passed=(@($checks|Where-Object{-not $_.passed}).Count -eq 0);checks=@($checks);scope='Compiled shared production required-step function; no payload, registration, shortcuts, elevation, uninstall entry or personal installation.'}
$result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $RunRoot 'results.json') -Encoding UTF8
$result | ConvertTo-Json -Depth 6
if(-not $result.passed){exit 1}
