param([string]$Evidence='D:\UDM\benchmarks\idm-complete-20260927')
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$inventory=Get-Content -LiteralPath "$Evidence\inventory.json" -Raw | ConvertFrom-Json
$records=@()
foreach($obj in $inventory.objects | Where-Object {$_.kind -in @('PE','signed catalog')}){
    $p=$obj.snapshot
    $sig=Get-AuthenticodeSignature -LiteralPath $p
    $item=Get-Item -LiteralPath $p
    $records += [pscustomobject]@{sha256=$obj.sha256;name=$obj.name;signatureStatus=[string]$sig.Status;signatureMessage=$sig.StatusMessage;signer=$sig.SignerCertificate.Subject;signerThumbprint=$sig.SignerCertificate.Thumbprint;timestampSigner=$sig.TimeStamperCertificate.Subject;fileVersion=$item.VersionInfo.FileVersion;productVersion=$item.VersionInfo.ProductVersion;description=$item.VersionInfo.FileDescription;company=$item.VersionInfo.CompanyName;originalFilename=$item.VersionInfo.OriginalFilename}
}
$records | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath "$Evidence\signatures.json" -Encoding utf8
$streams=@();$streamErrors=@()
foreach($root in $inventory.roots){
    foreach($file in Get-ChildItem -LiteralPath $root -Force -Recurse -File){
        try{
            foreach($stream in Get-Item -LiteralPath $file.FullName -Stream * -ErrorAction Stop){
                if($stream.Stream -eq ':$DATA'){continue}
                $streams += [pscustomobject]@{path=$file.FullName;stream=$stream.Stream;bytes=$stream.Length}
            }
        }catch{$streamErrors += [pscustomobject]@{path=$file.FullName;error=$_.Exception.Message}}
    }
}
@{streams=$streams;errors=$streamErrors} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath "$Evidence\alternate-streams.json" -Encoding utf8
$dumpbin=Join-Path $repo '.media-cache\driver-toolchain\Microsoft.VC.14.44.17.14.Tools.HostX64.TargetX64.base\Contents\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\dumpbin.exe'
New-Item -ItemType Directory -Force -Path "$Evidence\dumpbin","$Evidence\catalogs" | Out-Null
$toolRuns=@()
foreach($f in Get-ChildItem -LiteralPath "$Evidence\native-inputs" -File){
    & $dumpbin /headers /imports /exports /loadconfig /tls $f.FullName *> "$Evidence\dumpbin\$($f.Name).txt"
    $toolRuns += [pscustomobject]@{tool='DUMPBIN';input=$f.Name;exitCode=$LASTEXITCODE}
}
foreach($obj in $inventory.objects | Where-Object kind -eq 'signed catalog'){
    & certutil.exe -dump $obj.snapshot *> "$Evidence\catalogs\$($obj.name).txt"
    $toolRuns += [pscustomobject]@{tool='certutil';input=$obj.name;exitCode=$LASTEXITCODE}
}
$toolRuns | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath "$Evidence\tool-runs.json" -Encoding utf8
Write-Output ("Signature records: "+$records.Count+"; named streams: "+$streams.Count+"; DUMPBIN/catalog runs: "+$toolRuns.Count)
