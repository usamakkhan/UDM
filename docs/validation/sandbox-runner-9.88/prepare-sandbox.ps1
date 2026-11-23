param(
 [Parameter(Mandatory=$true)][string]$BuildManifest,
 [Parameter(Mandatory=$true)][string]$PackageRoot
)
$ErrorActionPreference='Stop'
$package=[IO.Path]::GetFullPath($PackageRoot)
if(Test-Path -LiteralPath $package){throw 'Choose a new PackageRoot; existing results are preserved.'}
$parent=Get-Item -LiteralPath (Split-Path $package -Parent)
while($null -ne $parent){if($parent.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Package ancestors must not redirect.'};$parent=$parent.Parent}
$build=Get-Content -LiteralPath $BuildManifest -Raw | ConvertFrom-Json
if(!$build.passed){throw 'The selected build did not pass.'}
$products=@($build.outputs | Where-Object kind -eq product)
if($products.Count -ne 5 -or @($products | Where-Object name -eq UDM.exe).Count -ne 1){throw 'Expected the five native build products.'}
foreach($row in $products){
 if([IO.Path]::GetFileName($row.name) -ne $row.name -or (Get-FileHash -LiteralPath $row.path -Algorithm SHA256).Hash -ne $row.sha256){throw 'Product name or hash mismatch.'}
}
$id=[guid]::NewGuid().ToString('N')
$inputRoot=Join-Path $package 'input';$outputRoot=Join-Path $package 'results'
New-Item -ItemType Directory -Path $inputRoot,$outputRoot | Out-Null
$files=@()
foreach($row in $products){
 $target=Join-Path $inputRoot $row.name
 Copy-Item -LiteralPath $row.path -Destination $target
 if((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -ne $row.sha256){throw 'Copied product hash mismatch.'}
 $files+= [ordered]@{name=$row.name;sha256=$row.sha256;bytes=(Get-Item -LiteralPath $target).Length}
}
$guest=Join-Path $inputRoot 'sandbox-smoke.ps1'
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'sandbox-smoke.ps1') -Destination $guest
$files+= [ordered]@{name='sandbox-smoke.ps1';sha256=(Get-FileHash -LiteralPath $guest -Algorithm SHA256).Hash;bytes=(Get-Item -LiteralPath $guest).Length}
$version=[Diagnostics.FileVersionInfo]::GetVersionInfo((Join-Path $inputRoot 'UDM.exe')).FileVersion
$manifest=[ordered]@{schema=1;packageId=$id;nativeVersion=$version;files=$files;expectedBytes=4194304;scope='Portable native CLI smoke; no installer, browser or IDM acceptance'}
$manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $inputRoot 'package.json') -Encoding UTF8
$id | Set-Content -LiteralPath (Join-Path $outputRoot '.udm-sandbox-output') -Encoding ASCII
$escape={param($value)[Security.SecurityElement]::Escape($value)}
$mappedInput=& $escape $inputRoot;$mappedOutput=& $escape $outputRoot
$xml=@"
<Configuration>
  <MemoryInMB>4096</MemoryInMB>
  <Networking>Disable</Networking>
  <ClipboardRedirection>Disable</ClipboardRedirection>
  <AudioInput>Disable</AudioInput>
  <VideoInput>Disable</VideoInput>
  <PrinterRedirection>Disable</PrinterRedirection>
  <MappedFolders>
    <MappedFolder><HostFolder>$mappedInput</HostFolder><SandboxFolder>C:\UdmTestPackage</SandboxFolder><ReadOnly>true</ReadOnly></MappedFolder>
    <MappedFolder><HostFolder>$mappedOutput</HostFolder><SandboxFolder>C:\UdmTestResults</SandboxFolder><ReadOnly>false</ReadOnly></MappedFolder>
  </MappedFolders>
  <LogonCommand><Command>powershell.exe -NoProfile -ExecutionPolicy Bypass -File C:\UdmTestPackage\sandbox-smoke.ps1</Command></LogonCommand>
</Configuration>
"@
$config=Join-Path $package 'UDM.wsb'
$xml | Set-Content -LiteralPath $config -Encoding UTF8
[xml]$check=Get-Content -LiteralPath $config -Raw
if(@($check.Configuration.MappedFolders.MappedFolder).Count -ne 2){throw 'Invalid Sandbox mapping.'}
[ordered]@{prepared=$true;packageId=$id;nativeVersion=$version;configuration=$config;results=$outputRoot;guestExecuted=$false;windowsFeaturesChanged=$false;installerCreated=$false} | ConvertTo-Json
