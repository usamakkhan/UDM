param([switch]$CapturePresentationUiTestsOnly,[switch]$Test,[switch]$CoreOnly,[switch]$UseInstalledToolchain,[switch]$AppOnly,[switch]$TestsOnly,[switch]$ZipUiTestsOnly,[switch]$ToolbarUiTestsOnly,[switch]$QueueExportUiTestsOnly,[switch]$CaptureExclusionUiTestsOnly,[string]$OutputRoot,[string]$ToolchainRoot)
$ErrorActionPreference='Stop'
if($CapturePresentationUiTestsOnly -and ($Test -or $CoreOnly -or $AppOnly -or $TestsOnly -or $ZipUiTestsOnly -or $ToolbarUiTestsOnly -or $QueueExportUiTestsOnly -or $CaptureExclusionUiTestsOnly)){throw 'Use -CapturePresentationUiTestsOnly separately.'}
if($CaptureExclusionUiTestsOnly -and ($Test -or $CoreOnly -or $AppOnly -or $TestsOnly -or $ZipUiTestsOnly -or $ToolbarUiTestsOnly -or $QueueExportUiTestsOnly)){throw 'Use -CaptureExclusionUiTestsOnly separately.'}
if($QueueExportUiTestsOnly -and ($Test -or $CoreOnly -or $AppOnly -or $TestsOnly -or $ZipUiTestsOnly -or $ToolbarUiTestsOnly)){throw 'Use -QueueExportUiTestsOnly separately.'}
if($ToolbarUiTestsOnly -and ($Test -or $CoreOnly -or $AppOnly -or $TestsOnly -or $ZipUiTestsOnly)){throw 'Use -ToolbarUiTestsOnly separately for the native toolbar harness.'}
if($ZipUiTestsOnly -and ($Test -or $CoreOnly -or $AppOnly -or $TestsOnly)){throw 'Use -ZipUiTestsOnly separately for the isolated dialog component harness.'}
if($AppOnly -and ($Test -or $TestsOnly)){throw 'Use -AppOnly for UI compilation, or -TestsOnly -Test for the native suite; do not combine them.'}
$udmRoot=Split-Path $PSScriptRoot -Parent
$udmCache=if($ToolchainRoot){$ToolchainRoot}else{Join-Path $udmRoot '.media-cache\driver-toolchain'}
$udmVc='Contents\VC\Tools\MSVC\14.44.35207'
$udmBin=Join-Path $udmCache ('Microsoft.VC.14.44.17.14.Tools.HostX64.TargetX64.base\'+$udmVc+'\bin\Hostx64\x64')
$udmSdk=Join-Path $udmCache 'microsoft.windows.sdk.cpp\c'
$udmSdkLib=Join-Path $udmCache 'microsoft.windows.sdk.cpp.x64\c'
$udmIncludes=@(
 (Join-Path $udmCache ('Microsoft.VC.14.44.17.14.CRT.Headers.base\'+$udmVc+'\include')),
 (Join-Path $udmCache ('Microsoft.VC.14.44.17.14.ATL.Headers.base\'+$udmVc+'\atlmfc\include')),
 (Join-Path $udmCache ('Microsoft.VC.14.44.17.14.MFC.Headers.base\'+$udmVc+'\atlmfc\include')),
 (Join-Path $udmSdk 'Include\10.0.26100.0\ucrt'),(Join-Path $udmSdk 'Include\10.0.26100.0\um'),(Join-Path $udmSdk 'Include\10.0.26100.0\shared')
)
$udmLibraries=@(
 (Join-Path $udmCache ('Microsoft.VC.14.44.17.14.CRT.x64.Desktop.base\'+$udmVc+'\lib\x64')),
 (Join-Path $udmCache ('Microsoft.VC.14.44.17.14.CRT.x64.Store.base\'+$udmVc+'\lib\x64')),
 (Join-Path $udmCache ('Microsoft.VC.14.44.17.14.ATL.X64.base\'+$udmVc+'\atlmfc\lib\x64')),
 (Join-Path $udmCache ('Microsoft.VC.14.44.17.14.MFC.X64.base\'+$udmVc+'\atlmfc\lib\x64')),
 (Join-Path $udmSdkLib 'ucrt\x64'),(Join-Path $udmSdkLib 'um\x64')
)
$udmSavedInclude=$env:INCLUDE
$udmSavedLib=$env:LIB
$udmCompiler=Join-Path $udmBin 'cl.exe'
$udmResourceCompiler=Join-Path $udmSdk 'bin\10.0.26100.0\x64\rc.exe'
if($UseInstalledToolchain -or !(Test-Path -LiteralPath $udmCompiler)){
 $udmCl=Get-Command cl.exe -ErrorAction SilentlyContinue
 $udmRc=Get-Command rc.exe -ErrorAction SilentlyContinue
 if(!$udmCl -or !$udmRc -or !$env:INCLUDE -or !$env:LIB){throw 'Open an x64 Native Tools PowerShell for Visual Studio 2022 with Desktop C++, MFC/ATL and a Windows SDK installed, then run build.ps1 again.'}
 $udmCompiler=$udmCl.Source
 $udmResourceCompiler=$udmRc.Source
 if($env:VSCMD_ARG_TGT_ARCH -and $env:VSCMD_ARG_TGT_ARCH -ne 'x64'){throw 'Select the x64 Visual Studio developer environment.'}
}else{
 foreach($udmPath in @($udmIncludes)+@($udmLibraries)){if(!(Test-Path -LiteralPath $udmPath)){throw ('Missing cached compiler dependency: '+$udmPath+'. Use -UseInstalledToolchain in a Visual Studio x64 developer environment.')}}
 $env:INCLUDE=$udmIncludes -join ';'
 $env:LIB=$udmLibraries -join ';'
}
Push-Location -LiteralPath $udmRoot
try {
$udmOut=Join-Path $PSScriptRoot 'out'
$udmRelease=Join-Path $udmRoot 'release-native'
if($OutputRoot){
 if(![IO.Path]::IsPathRooted($OutputRoot)){throw 'OutputRoot must be an absolute path.'}
 $udmOut=Join-Path $OutputRoot 'out'
 $udmRelease=Join-Path $OutputRoot 'release-native'
}
New-Item -ItemType Directory -Force -Path $udmOut,$udmRelease | Out-Null
$env:VCTIP_NOOPTIN='1'
$env:VSCMD_SKIP_SENDTELEMETRY='1'
$udmCommon=@('/nologo','/bigobj','/std:c++17','/EHsc','/MT','/O2','/W4','/utf-8','/permissive-','/DUNICODE','/D_UNICODE','/DNOMINMAX','/D_WIN32_WINNT=0x0A00','/DWINVER=0x0A00','/D_CRT_SECURE_NO_WARNINGS','/Zc:__cplusplus','/Zi',('/Fd'+(Join-Path $udmOut 'native.pdb')))
$udmCurl=Join-Path $PSScriptRoot 'vendor\curl'
if(!(Test-Path -LiteralPath (Join-Path $udmCurl 'lib\libcurl.lib'))){throw 'Missing native/vendor/curl static library. See vendor/curl/README.md for the reproducible build.'}
$udmCommon+=@('/DCURL_STATICLIB',('/I'+(Join-Path $udmCurl 'include')))
$udmCore=@('Core','PacProxy','Queue','FileWorkflows','Duplicates','Transfer','Grabber','GrabberLinks','Streaming','Adaptive','YouTubePlayer','Bridge','Network','SocksProxy','OfflineSite','DialUp','Ftp','DownloadPreview','ZipPreview','QueueWake','Scanner','CaptureExclusions','CaptureAdmission')
if($CoreOnly){$udmCore=@('Core','PacProxy','Transfer','Grabber','GrabberLinks','SocksProxy','OfflineSite','DialUp','Ftp','DownloadPreview','ZipPreview','QueueWake','Scanner')}
foreach($udmName in $udmCore){
 $udmSource=Join-Path $PSScriptRoot ($udmName+'.cpp')
 $udmObject=Join-Path $udmOut ($udmName+'.obj')
 $udmHeaderDate=@('Core.hpp','PacProxy.hpp','CompletionPolicy.hpp','GuiModels.hpp','SpeedMeter.hpp','BrowserSettings.hpp','StreamProgress.hpp','MediaStorage.hpp','YouTubePlayer.hpp','BrowserRequest.hpp','OfflineSite.hpp','OfflineArchive.hpp','BrowserProxy.hpp','BrowserSession.hpp','GrabberAuth.hpp','GrabberBrowserSession.hpp','GrabberProject.hpp','GrabberFilters.hpp','CaptureReceipts.hpp','CapturePresentation.hpp','CaptureTransactions.hpp','CaptureExclusions.hpp','ProxyPolicy.hpp','SiteLogins.hpp','DialUp.hpp','DownloadPreview.hpp','QueueWake.hpp','Scanner.hpp','StreamCache.hpp') | ForEach-Object {(Get-Item (Join-Path $PSScriptRoot $_)).LastWriteTimeUtc} | Sort-Object -Descending | Select-Object -First 1
 if($udmName -in @('Core','Adaptive','Bridge')){$udmHeaderDate=@($udmHeaderDate,(Get-Item (Join-Path $PSScriptRoot 'OptionsModel.hpp')).LastWriteTimeUtc)|Sort-Object -Descending|Select-Object -First 1}
 if($udmName -in @('Core','Transfer','Grabber','GrabberLinks','Duplicates','OfflineSite','FileWorkflows')){foreach($udmHeader in @('GrabberDestinations.hpp','GrabberAuth.hpp','GrabberLinks.hpp','GrabberProject.hpp','GrabberFilters.hpp','PublicSuffix.hpp','OptionsModel.hpp','OfflineSite.hpp')){$udmHeaderDate=@($udmHeaderDate,(Get-Item (Join-Path $PSScriptRoot $udmHeader)).LastWriteTimeUtc)|Sort-Object -Descending|Select-Object -First 1}}
 if($udmName -eq 'Transfer'){$udmHeaderDate=@($udmHeaderDate,(Get-Item (Join-Path $PSScriptRoot 'BrowserCookies.hpp')).LastWriteTimeUtc,(Get-Item (Join-Path $PSScriptRoot 'CurlHttp.hpp')).LastWriteTimeUtc)|Sort-Object -Descending|Select-Object -First 1}
 if($udmName -eq 'Core'){$udmHeaderDate=@($udmHeaderDate,(Get-Item (Join-Path $PSScriptRoot 'ToolbarModel.hpp')).LastWriteTimeUtc)|Sort-Object -Descending|Select-Object -First 1}
 if($udmName -in @('Core','Bridge')){$udmHeaderDate=@($udmHeaderDate,(Get-Item (Join-Path $PSScriptRoot 'DirectMediaRefresh.hpp')).LastWriteTimeUtc)|Sort-Object -Descending|Select-Object -First 1}
 if($udmName -in @('Adaptive','Bridge')){$udmHeaderDate=@($udmHeaderDate,(Get-Item (Join-Path $PSScriptRoot 'AdaptiveCapture.hpp')).LastWriteTimeUtc)|Sort-Object -Descending|Select-Object -First 1}
 if($udmName -eq 'Bridge'){$udmHeaderDate=@($udmHeaderDate,(Get-Item (Join-Path $PSScriptRoot 'BrowserIdentity.hpp')).LastWriteTimeUtc)|Sort-Object -Descending|Select-Object -First 1}
 if($udmName -in @('Core','Transfer','SocksProxy','Ftp')){$udmHeaderDate=@($udmHeaderDate,(Get-Item (Join-Path $PSScriptRoot 'SocksProxy.hpp')).LastWriteTimeUtc)|Sort-Object -Descending|Select-Object -First 1}
 if($udmName -in @('Core','OfflineSite')){$udmHeaderDate=@($udmHeaderDate,(Get-Item (Join-Path $PSScriptRoot 'OfflineSite.hpp')).LastWriteTimeUtc)|Sort-Object -Descending|Select-Object -First 1}
 if($udmName -in @('Transfer','Ftp')){$udmHeaderDate=@($udmHeaderDate,(Get-Item (Join-Path $PSScriptRoot 'Ftp.hpp')).LastWriteTimeUtc)|Sort-Object -Descending|Select-Object -First 1}
 if($udmName -in @('Ftp','ZipPreview')){$udmHeaderDate=@($udmHeaderDate,(Get-Item (Join-Path $PSScriptRoot 'ZipPreview.hpp')).LastWriteTimeUtc,(Get-Item (Join-Path $PSScriptRoot 'Ftp.hpp')).LastWriteTimeUtc)|Sort-Object -Descending|Select-Object -First 1}
 if($udmName -eq 'Network'){$udmHeaderDate=@($udmHeaderDate,(Get-Item (Join-Path $udmRoot 'drivers\signed-network\BrokerProtocol.hpp')).LastWriteTimeUtc)|Sort-Object -Descending|Select-Object -First 1}
 if($udmName -eq 'Adaptive'){$udmHeaderDate=@($udmHeaderDate,(Get-Item (Join-Path $PSScriptRoot 'WebVtt.hpp')).LastWriteTimeUtc,(Get-Item (Join-Path $PSScriptRoot 'AdaptiveResources.hpp')).LastWriteTimeUtc,(Get-Item (Join-Path $PSScriptRoot 'LiveHls.hpp')).LastWriteTimeUtc,(Get-Item (Join-Path $PSScriptRoot 'HlsPlaylist.hpp')).LastWriteTimeUtc,(Get-Item (Join-Path $PSScriptRoot 'HlsRecording.hpp')).LastWriteTimeUtc)|Sort-Object -Descending|Select-Object -First 1}
 if($udmName -eq 'Streaming'){$udmHeaderDate=@($udmHeaderDate,(Get-Item (Join-Path $PSScriptRoot 'Streaming.hpp')).LastWriteTimeUtc)|Sort-Object -Descending|Select-Object -First 1}
 if((Test-Path -LiteralPath $udmObject) -and (Get-Item $udmObject).LastWriteTimeUtc -gt (Get-Item $udmSource).LastWriteTimeUtc -and (Get-Item $udmObject).LastWriteTimeUtc -gt $udmHeaderDate){continue}
 & $udmCompiler @udmCommon /c $udmSource ('/Fo'+$udmObject)
 if($LASTEXITCODE){throw ('C++ compilation failed: '+$udmName)}
}
if($CoreOnly){return}
$udmObjects=$udmCore|ForEach-Object {Join-Path $udmOut ($_+'.obj')}
$udmSystem=@((Join-Path $udmCurl 'lib\libcurl.lib'),'secur32.lib','normaliz.lib','winhttp.lib','wininet.lib','crypt32.lib','bcrypt.lib','shell32.lib','shlwapi.lib','ole32.lib','oleaut32.lib','advapi32.lib','user32.lib','gdi32.lib','comctl32.lib','ws2_32.lib','iphlpapi.lib','uuid.lib','winmm.lib','uxtheme.lib','powrprof.lib','rasapi32.lib','rasdlg.lib')
& $udmResourceCompiler /nologo ('/fo'+(Join-Path $udmOut 'App.res')) (Join-Path $PSScriptRoot 'App.rc')
if($LASTEXITCODE){throw 'Resource compilation failed'}
$udmTargets=@(@('App','UDM','WINDOWS'),@('HostMain','Udm.NativeHost','CONSOLE'),@('MonitorMain','Udm.Monitor','CONSOLE'),@('Tests','Udm.NativeTests','CONSOLE'))
if($AppOnly){$udmTargets=,@('App','UDM','WINDOWS')}
if($TestsOnly){$udmTargets=,@('Tests','Udm.NativeTests','CONSOLE')}
if($ZipUiTestsOnly){$udmTargets=,@('ZipPreviewUiTests','Udm.ZipPreviewUiTests','WINDOWS')}
if($ToolbarUiTestsOnly){$udmTargets=,@('ToolbarUiTests','Udm.ToolbarUiTests','WINDOWS')}
if($QueueExportUiTestsOnly){$udmTargets=,@('QueueExportUiTests','Udm.QueueExportUiTests','WINDOWS')}
if($CaptureExclusionUiTestsOnly){$udmTargets=,@('CaptureExclusionUiTests','Udm.CaptureExclusionUiTests','WINDOWS')}
if($CapturePresentationUiTestsOnly){$udmTargets=,@('CapturePresentationUiTests','Udm.CapturePresentationUiTests','WINDOWS')}
foreach($udmTarget in $udmTargets){
 $udmEntry=@();if($udmTarget[0] -in @('App','ZipPreviewUiTests','ToolbarUiTests','QueueExportUiTests','CaptureExclusionUiTests','CapturePresentationUiTests')){$udmEntry=@('/ENTRY:wWinMainCRTStartup')}
 & $udmCompiler @udmCommon (Join-Path $PSScriptRoot ($udmTarget[0]+'.cpp')) @udmObjects (Join-Path $udmOut 'App.res') ('/Fo'+(Join-Path $udmOut ($udmTarget[0]+'.obj'))) ('/Fe'+(Join-Path $udmRelease ($udmTarget[1]+'.exe'))) /link @udmSystem @udmEntry ('/SUBSYSTEM:'+$udmTarget[2]) /DYNAMICBASE /NXCOMPAT /HIGHENTROPYVA /DEBUG:FULL /INCREMENTAL:NO /MANIFEST:NO
 if($LASTEXITCODE){throw ('C++ link failed: '+$udmTarget[1])}
}
Copy-Item -LiteralPath (Join-Path $udmCurl 'LICENSE.txt') -Destination (Join-Path $udmRelease 'curl-LICENSE.txt')
Copy-Item -LiteralPath (Join-Path $udmRoot 'assets') -Destination $udmRelease -Recurse -Force
if($Test){
 $udmTestTools=Join-Path $udmRelease 'tools'
 foreach($udmMedia in @('ffmpeg.exe','ffprobe.exe','FFmpeg-LICENSE.txt')){
  $udmSourceMedia=Join-Path $udmRoot ('release\tools\'+$udmMedia)
  if((Test-Path -LiteralPath $udmSourceMedia) -and !(Test-Path -LiteralPath (Join-Path $udmTestTools $udmMedia))){
   New-Item -ItemType Directory -Force -Path $udmTestTools | Out-Null
   Copy-Item -LiteralPath $udmSourceMedia -Destination $udmTestTools
  }
 }
}
if($Test){& (Join-Path $udmRelease 'Udm.NativeTests.exe');if($LASTEXITCODE){throw 'Native tests failed'}}
Write-Output ('Native C++/MFC binaries: '+$udmRelease)
} finally {
 Pop-Location
 $env:INCLUDE=$udmSavedInclude
 $env:LIB=$udmSavedLib
}
