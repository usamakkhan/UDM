param([Parameter(Mandatory)][string]$CurlArchive,[Parameter(Mandatory)][string]$CMakePath,[Parameter(Mandatory)][string]$BuildRoot,[string]$ToolchainRoot)
$ErrorActionPreference='Stop'
$udmHash='f9ec970e52124e494606209e6bc0e985c623b428796641742f60c5b56931aaee'
if((Get-FileHash -LiteralPath $CurlArchive -Algorithm SHA256).Hash.ToLowerInvariant() -ne $udmHash){throw 'Expected the pinned curl-8.22.0.zip source archive.'}
if(![IO.Path]::IsPathRooted($BuildRoot) -or (Test-Path -LiteralPath $BuildRoot)){throw 'Choose a new absolute BuildRoot directory.'}
$udmProject=Split-Path $PSScriptRoot -Parent
if(!$ToolchainRoot){$ToolchainRoot=Join-Path $udmProject '.media-cache\driver-toolchain'}
$udmVc='Contents\VC\Tools\MSVC\14.44.35207'
$udmBin=Join-Path $ToolchainRoot ('Microsoft.VC.14.44.17.14.Tools.HostX64.TargetX64.base\'+$udmVc+'\bin\Hostx64\x64')
$udmSdk=Join-Path $ToolchainRoot 'microsoft.windows.sdk.cpp\c'
$udmSdkLib=Join-Path $ToolchainRoot 'microsoft.windows.sdk.cpp.x64\c'
$udmInclude=@((Join-Path $ToolchainRoot ('Microsoft.VC.14.44.17.14.CRT.Headers.base\'+$udmVc+'\include')),(Join-Path $udmSdk 'Include\10.0.26100.0\ucrt'),(Join-Path $udmSdk 'Include\10.0.26100.0\um'),(Join-Path $udmSdk 'Include\10.0.26100.0\shared'))
$udmLib=@((Join-Path $ToolchainRoot ('Microsoft.VC.14.44.17.14.CRT.x64.Desktop.base\'+$udmVc+'\lib\x64')),(Join-Path $udmSdkLib 'ucrt\x64'),(Join-Path $udmSdkLib 'um\x64'))
foreach($udmPath in @($udmInclude)+@($udmLib)+@($udmBin,$CMakePath)){if(!(Test-Path -LiteralPath $udmPath)){throw ('Missing build dependency: '+$udmPath)}}
$udmSavedPath=$env:PATH;$udmSavedInclude=$env:INCLUDE;$udmSavedLib=$env:LIB
try {
 $env:PATH=$udmBin+';'+(Join-Path $udmSdk 'bin\10.0.26100.0\x64')+';'+$env:PATH
 $env:INCLUDE=$udmInclude -join ';';$env:LIB=$udmLib -join ';';$env:VCTIP_NOOPTIN='1';$env:VSCMD_SKIP_SENDTELEMETRY='1'
 New-Item -ItemType Directory -Path $BuildRoot | Out-Null
 Expand-Archive -LiteralPath $CurlArchive -DestinationPath $BuildRoot
 $udmSource=Join-Path $BuildRoot 'curl-8.22.0';$udmBuild=Join-Path $BuildRoot 'out';$udmInstall=Join-Path $PSScriptRoot 'vendor\curl'
 $udmFlags=@('-G','NMake Makefiles','-S',$udmSource,'-B',$udmBuild,'-DCMAKE_BUILD_TYPE=Release',('-DCMAKE_INSTALL_PREFIX='+$udmInstall),
  '-DBUILD_SHARED_LIBS=OFF','-DBUILD_STATIC_LIBS=ON','-DBUILD_CURL_EXE=OFF','-DBUILD_TESTING=OFF','-DBUILD_LIBCURL_DOCS=OFF','-DBUILD_MISC_DOCS=OFF',
  '-DCURL_STATIC_CRT=ON','-DCURL_USE_SCHANNEL=ON','-DCURL_USE_OPENSSL=OFF','-DCURL_ZLIB=OFF','-DCURL_BROTLI=OFF','-DCURL_ZSTD=OFF','-DCURL_USE_LIBPSL=OFF','-DUSE_NGHTTP2=OFF','-DUSE_LIBIDN2=OFF','-DENABLE_THREADED_RESOLVER=ON','-DENABLE_ARES=OFF')
 foreach($udmFeature in @('ALTSVC','AWS','DICT','DOH','FILE','FTP','GOPHER','HSTS','IMAP','LDAP','LDAPS','MQTT','NETRC','POP3','RTSP','SMTP','TELNET','TFTP','WEBSOCKETS')){$udmFlags+=('-DCURL_DISABLE_'+$udmFeature+'=ON')}
 & $CMakePath @udmFlags;if($LASTEXITCODE){throw 'curl configuration failed'}
 & $CMakePath --build $udmBuild;if($LASTEXITCODE){throw 'curl compilation failed'}
 & $CMakePath --install $udmBuild;if($LASTEXITCODE){throw 'curl installation failed'}
 Copy-Item -LiteralPath (Join-Path $udmSource 'COPYING') -Destination (Join-Path $udmInstall 'LICENSE.txt')
 Get-FileHash -LiteralPath (Join-Path $udmInstall 'lib\libcurl.lib') -Algorithm SHA256
} finally {$env:PATH=$udmSavedPath;$env:INCLUDE=$udmSavedInclude;$env:LIB=$udmSavedLib}
