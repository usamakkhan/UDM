param([string]$Evidence='D:\UDM\benchmarks\idm-complete-20260927')
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$dumpbin=Join-Path $repo '.media-cache\driver-toolchain\Microsoft.VC.14.44.17.14.Tools.HostX64.TargetX64.base\Contents\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\dumpbin.exe'
$inventory=Get-Content -LiteralPath "$Evidence\inventory.json" -Raw | ConvertFrom-Json
$destination=Join-Path $Evidence 'dumpbin-disasm'
New-Item -ItemType Directory -Force -Path $destination | Out-Null
$results=@()
foreach($obj in $inventory.objects | Where-Object kind -EQ 'PE'){
    $output=Join-Path $destination ($obj.sha256.Substring(0,12)+'__'+$obj.name+'.txt')
    & $dumpbin /NOLOGO /DISASM $obj.snapshot > $output 2>&1
    $code=$LASTEXITCODE
    $results += [pscustomobject]@{name=$obj.name;sha256=$obj.sha256;exitCode=$code;output=$output;outputBytes=(Get-Item -LiteralPath $output).Length;outputSha256=(Get-FileHash -LiteralPath $output -Algorithm SHA256).Hash.ToLowerInvariant();scope='Independent static section disassembly; not proof of code/data distinction or runtime reachability'}
}
$results | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath "$destination\summary.json" -Encoding utf8
[pscustomobject]@{files=$results.Count;failures=@($results | Where-Object exitCode -NE 0).Count;outputBytes=($results | Measure-Object outputBytes -Sum).Sum} | ConvertTo-Json
if($results | Where-Object exitCode -NE 0){throw 'A DUMPBIN disassembly failed; inspect summary.json'}
