param([string]$Version='0.6.1',[switch]$TestSigned)
$ErrorActionPreference='Stop'
if($Version -notmatch '^\d+\.\d+(\.\d+)?$'){throw 'Use a numeric version.'}
$root=Split-Path $PSScriptRoot -Parent
$built=Join-Path $PSScriptRoot $(if($TestSigned){'out\x64-test-signed'}else{'out\x64'})
$evidence=Get-Content (Join-Path $built 'build-evidence.json') -Raw | ConvertFrom-Json
if(!$evidence.analyzed -or !$evidence.infVerified){throw 'Run build-driver.ps1 -Analyze first.'}
$signing=$null
if($TestSigned){
    $signing=Get-Content (Join-Path $built 'signing-evidence.json') -Raw | ConvertFrom-Json
    if(!$signing.cmsSignaturesVerified -or $signing.productionSigned){throw 'Expected development test-signing evidence.'}
    if((Get-FileHash (Join-Path $built 'UdmWfp.sys')).Hash -ne $signing.driverSha256 -or (Get-FileHash (Join-Path $built 'UdmWfp.cat')).Hash -ne $signing.catalogSha256){throw 'Signed package differs from verification evidence.'}
}elseif((Get-FileHash (Join-Path $built 'UdmWfp.sys')).Hash -ne $evidence.driverSha256){throw 'Driver does not match build evidence.'}
if((Get-FileHash (Join-Path $PSScriptRoot 'UdmMonitor.c')).Hash -ne $evidence.sourceSha256){throw 'Source changed after driver build.'}
if((Get-FileHash (Join-Path $PSScriptRoot 'UdmWfpProtocol.h')).Hash -ne $evidence.protocolSha256){throw 'Protocol changed after driver build.'}
if(!$TestSigned -and (Get-AuthenticodeSignature (Join-Path $built 'UdmWfp.sys')).Status -ne 'NotSigned'){throw 'Select -TestSigned for a signed development package.'}
$kind=if($TestSigned){'test-signed-development'}else{'unsigned-development'}
$staging=Join-Path $root ('package\driver-stage-'+[Guid]::NewGuid().ToString('N'))
$target=Join-Path $staging ('UDM-WFP-'+$Version+'-x64-'+$kind)
New-Item -ItemType Directory -Path $target -Force | Out-Null
foreach($name in @('UdmWfp.sys','UdmWfp.pdb','UdmWfp.inf','build-evidence.json')){Copy-Item -LiteralPath (Join-Path $built $name) -Destination $target}
foreach($name in @('README.md','VM-TEST-PLAN.md','UdmMonitor.c','UdmWfpProtocol.h','LiveSmoke.cs')){Copy-Item -LiteralPath (Join-Path $PSScriptRoot $name) -Destination $target}
if($TestSigned){foreach($name in @('UdmWfp.cat','UDM-development-public.cer','signing-evidence.json')){Copy-Item -LiteralPath (Join-Path $built $name) -Destination $target}}
Copy-Item -LiteralPath (Join-Path $root 'release\Udm.Monitor.exe') -Destination $target
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'out\tests\Udm.DriverSmoke.exe') -Destination $target
"$kind. Not kernel-tested or Microsoft production-signed. Read README and VM-TEST-PLAN. No private key is included. Ordinary UDM setup does not install this driver or change Windows security settings." | Set-Content (Join-Path $target 'START-HERE.txt')
$archive=Join-Path $root ('UDM-WFP-'+$Version+'-x64-'+$kind+'.zip')
Compress-Archive -LiteralPath $target -DestinationPath $archive -Force
Write-Output $archive
