param([string]$ReleaseDirectory,[string]$BrowserDirectory,[string]$OutputDirectory,[string]$Version = '1.0.4',[string]$CompilerPath,
 # Code signing is the user's certificate: a thumbprint in the certificate store, a PFX file (password
 # by parameter or UDM_SIGN_PFX_PASSWORD) or an Azure Trusted Signing metadata file. Without one, the
 # setup is built unsigned as before. The timestamp authority defaults to the credential's usual one.
 [string]$SignThumbprint,[string]$SignPfxPath,[securestring]$SignPfxPassword,[string]$TrustedSigningMetadata,[string]$TrustedSigningDlib,[string]$TimestampUrl,[string]$SignToolPath,
 [switch]$SignDryRun,  # report the signtool, certificate and files that would be signed, then stop
 [switch]$NoTimestamp)
$ErrorActionPreference = 'Stop'
$root=Split-Path $PSScriptRoot -Parent
$release=if($ReleaseDirectory){[IO.Path]::GetFullPath($ReleaseDirectory)}else{Join-Path $root 'release'}
$browser=if($BrowserDirectory){[IO.Path]::GetFullPath($BrowserDirectory)}else{Join-Path $root 'browser'}
$output=if($OutputDirectory){[IO.Path]::GetFullPath($OutputDirectory)}else{Join-Path $root 'installer-out'}
# Dot-sourcing sign.ps1 runs its param block in this scope, which resets the parameters the two scripts
# share by name (TrustedSigningMetadata, TrustedSigningDlib, TimestampUrl, SignToolPath, NoTimestamp) to
# sign.ps1's defaults. Keep this script's values across the dot-source.
$signOptions=@{Metadata=$TrustedSigningMetadata;Dlib=$TrustedSigningDlib;TimestampUrl=$TimestampUrl;SignToolPath=$SignToolPath;NoTimestamp=[bool]$NoTimestamp}
. (Join-Path $PSScriptRoot 'sign.ps1') -AsModule
$TrustedSigningMetadata=$signOptions.Metadata;$TrustedSigningDlib=$signOptions.Dlib;$TimestampUrl=$signOptions.TimestampUrl;$SignToolPath=$signOptions.SignToolPath;$NoTimestamp=$signOptions.NoTimestamp
$signing=$null
if($SignThumbprint -or $SignPfxPath -or $TrustedSigningMetadata -or $SignDryRun){
    $signing=@{Tool=(Get-UdmSignToolPath $SignToolPath);Credential=(Get-UdmSigningCertificate -Thumbprint $SignThumbprint -PfxPath $SignPfxPath -PfxPassword $SignPfxPassword -TrustedSigningMetadata $TrustedSigningMetadata -TrustedSigningDlib $TrustedSigningDlib)}
    if(-not $TimestampUrl){$TimestampUrl=$signing.Credential.TimestampUrl}
    Write-Output ('signtool:    '+$signing.Tool)
    Write-Output ('certificate: '+(Get-UdmCredentialDescription $signing.Credential))
    Write-Output ('timestamp:   '+$(if($NoTimestamp){'none'}else{$TimestampUrl}))
    if($SignDryRun){
        Get-UdmSignSet -Release $release | ForEach-Object { Write-Output (('{0,-28} {1}' -f $_.Name,$(if($_.Present){(Test-UdmCredentialSignature -Path $_.Path -Credential $signing.Credential).Status}else{'not built'}))) }
        Write-Output 'setup:                       signed by Inno Setup (SignTool=udm, SignedUninstaller=yes)'
        Write-Output 'dry run: nothing was signed or built'
        return
    }
}
if ($Version -notmatch '^\d+\.\d+(\.\d+)?$') { throw 'Use a numeric release version.' }
$root = Split-Path $PSScriptRoot -Parent
$compiler = @(
    $CompilerPath,
    (Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6\ISCC.exe'),
    (Join-Path $env:LOCALAPPDATA 'Programs\Inno Setup 6\ISCC.exe')
) | Where-Object { $_ -and (Test-Path -LiteralPath $_) } | Select-Object -First 1
if (-not $compiler) { throw 'Install Inno Setup 6 before building the setup executable.' }
$application = Join-Path $release 'UDM.exe'
if (!(Test-Path -LiteralPath $application) -or (Get-Item -LiteralPath $application).VersionInfo.ProductVersion -ne $Version) { throw 'Rebuild UDM with the requested version before packaging it.' }
foreach ($file in @('Udm.NativeHost.exe','Udm.Monitor.exe','Udm.SetupHelper.exe','Udm.Automation.exe')) {
    $program = Join-Path $release $file
    if (!(Test-Path -LiteralPath $program) -or (Get-Item -LiteralPath $program).VersionInfo.ProductVersion -ne $Version) {
        throw ('Rebuild matching native companion before packaging: ' + $file)
    }
}
$network = Join-Path $release 'network'
foreach ($file in @('Udm.Network.exe','runtime\WinDivert.dll','runtime\WinDivert64.sys','runtime\WinDivert-LICENSE.txt','WinDivert-2.2.2-Source.zip')) {
    if (-not (Test-Path -LiteralPath (Join-Path $network $file))) { throw ('Missing signed network package: '+$file) }
}
$status = & (Join-Path $network 'Udm.Network.exe') --status | ConvertFrom-Json
if ($LASTEXITCODE -ne 0 -or -not $status.RuntimeVerified -or $status.DesktopBrokerProtocol -ne 1) { throw 'Signed network runtime or desktop protocol verification failed.' }
$browserVersion=(Get-Content -LiteralPath (Join-Path $browser 'chromium\manifest.json') -Raw | ConvertFrom-Json).version
if ($browserVersion -notmatch '^\d+\.\d+\.\d+$') { throw 'The browser manifest must declare a numeric release version.' }
$outputName='UDM-' + $Version + '-Browser-' + $browserVersion + '-Setup-x64'
$firefoxVersion=(Get-Content -LiteralPath (Join-Path $browser 'firefox\manifest.json') -Raw | ConvertFrom-Json).version
if ($firefoxVersion -ne $browserVersion) { throw 'Chromium and Firefox packages must use the same release version.' }
$innoArguments=@(('/DReleaseRoot='+$release),('/DBrowserRoot='+$browser),('/O'+$output),('/DAppVersion=' + $Version),('/DBrowserVersion=' + $browserVersion),('/F' + $outputName))
if($signing){
    # UDM's own executables first (never the vendor's WinDivert files), then Inno signs the setup and uninstaller.
    $set=Get-UdmSignSet -Release $release | Where-Object Present
    Invoke-UdmSign -Files $set.Path -Credential $signing.Credential -SignTool $signing.Tool -TimestampUrl $TimestampUrl -NoTimestamp:$NoTimestamp | Format-Table -AutoSize | Out-String -Width 200 | Write-Output
    $innoArguments+=@('/DSignSetup',('/S'+(Get-UdmInnoSignToolDefinition -Credential $signing.Credential -SignTool $signing.Tool -TimestampUrl $TimestampUrl -NoTimestamp:$NoTimestamp)))
}
& $compiler @innoArguments (Join-Path $PSScriptRoot 'UDM-Setup.iss')
if ($LASTEXITCODE -ne 0) { throw 'Inno Setup compilation failed.' }
if($signing){
    $setupSignature=Test-UdmCredentialSignature -Path (Join-Path $output ($outputName + '.exe')) -Credential $signing.Credential
    if(-not $setupSignature.Matches){throw ('The setup is not signed by the expected certificate ('+$setupSignature.Status+', issuer '+$setupSignature.Issuer+').')}
}
Get-Item -LiteralPath (Join-Path $output ($outputName + '.exe')) | Select-Object -ExpandProperty FullName
