param(
 [Parameter(Mandatory=$true)][string]$BuildRoot,
 [Parameter(Mandatory=$true)][string]$RunRoot,
 [string]$Python='python',
 [string]$Ffmpeg,
 [switch]$UseInstalledToolchain,
 [string]$ToolchainRoot
)
$ErrorActionPreference='Stop'
if(![IO.Path]::IsPathRooted($BuildRoot) -or ![IO.Path]::IsPathRooted($RunRoot)){throw 'BuildRoot and RunRoot must be absolute paths.'}
$run=[IO.Path]::GetFullPath($RunRoot)
if(Test-Path -LiteralPath $run){throw 'Use a new RunRoot; existing results will not be overwritten.'}
$pythonCommand=Get-Command $Python -ErrorAction Stop
if($Ffmpeg){$ffmpegCommand=Get-Command $Ffmpeg -ErrorAction Stop}
New-Item -ItemType Directory -Path $run | Out-Null
$summary=[ordered]@{passed=$false;nativeChecks=0;videoIncluded=[bool]$Ffmpeg;fullMediaDecodePassed=$false;browserPlaybackVerified=$false}
try {
 $build=@{OutputRoot=$BuildRoot;BackendTestsOnly=$true;BackendTestNames=@('RenderedOfflineTests');UseInstalledToolchain=$UseInstalledToolchain}
 if($ToolchainRoot){$build.ToolchainRoot=$ToolchainRoot}
 & (Join-Path $PSScriptRoot 'build.ps1') @build *> (Join-Path $run 'build.log')
 $arguments=@((Join-Path (Split-Path $PSScriptRoot -Parent) 'tests\rendered-offline-native.py'),(Join-Path $BuildRoot 'release-native\Udm.RenderedOfflineTests.exe'),(Join-Path $run 'data'))
 if($Ffmpeg){
  $clip=Join-Path $run 'clip.mp4'
  & $ffmpegCommand.Source -hide_banner -loglevel error -f lavfi -i 'testsrc2=size=160x90:rate=10:duration=2' -f lavfi -i 'sine=frequency=440:sample_rate=44100:duration=2' -c:v libx264 -pix_fmt yuv420p -c:a aac -movflags +faststart $clip *> (Join-Path $run 'media-generation.log')
  if($LASTEXITCODE){throw 'Media fixture generation failed.'}
  $arguments+=$clip
 }
 & $pythonCommand.Source @arguments *> (Join-Path $run 'archive-test.log')
 if($LASTEXITCODE){throw 'Rendered archive integration failed; inspect archive-test.log.'}
 $detail=Get-Content -LiteralPath (Join-Path $run 'data\results.json') -Raw|ConvertFrom-Json
 if(!$detail.passed -or !$detail.startPageNotRequested -or !$detail.localLinksRewritten){throw 'Incomplete archive acceptance evidence.'}
 $summary.nativeChecks=$detail.checks
 if($Ffmpeg){
  $videos=@(Get-ChildItem -LiteralPath (Join-Path $run 'data\extracted') -Filter '*.mp4')
  if($videos.Count -ne 1){throw 'Expected exactly one archived test video.'}
  & $ffmpegCommand.Source -v error -i $videos[0].FullName -f null - *> (Join-Path $run 'media-decode.log')
  if($LASTEXITCODE){throw 'Archived media decoding failed.'}
  $summary.fullMediaDecodePassed=$true
 }
 $summary.passed=$true
}catch{
 $summary.error=$_.Exception.Message
 throw
}finally{
 $summary|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $run 'summary.json') -Encoding utf8
}
Write-Output "Rendered archive checks passed. Evidence: $run"
Write-Output 'Browser playback is a separate check: serve data/extracted with tests/offline-shadow-browser-fixture.cjs.'
