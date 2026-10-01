param(
    [ValidatePattern('^[a-p]{32}$')][string]$ExtensionId,
    [ValidateSet('Chromium','Firefox','Both')][string]$Browser = 'Chromium',
    [string]$HostExecutable = (Join-Path $PSScriptRoot '..\release\Udm.NativeHost.exe')
)
$ErrorActionPreference = 'Stop'
if ($Browser -ne 'Firefox' -and -not $ExtensionId) { throw 'Supply the 32-character extension ID shown in chrome://extensions or edge://extensions.' }
$hostPath = (Resolve-Path -LiteralPath $HostExecutable).Path
$manifestDir = Join-Path $env:LOCALAPPDATA 'UDM\native-messaging'
New-Item -ItemType Directory -Force -Path $manifestDir | Out-Null
if ($Browser -ne 'Firefox') {
    $manifestPath = Join-Path $manifestDir 'chromium.json'
    @{name='com.udm.download_manager';description='UDM browser download helper';path=$hostPath;type='stdio';allowed_origins=@("chrome-extension://$ExtensionId/")} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $manifestPath -Encoding ascii
    foreach ($vendor in @('Google\Chrome','Microsoft\Edge','Chromium','BraveSoftware\Brave-Browser','Vivaldi')) {
        $key = "HKCU:\Software\$vendor\NativeMessagingHosts\com.udm.download_manager"
        New-Item -Path $key -Force | Out-Null
        Set-Item -LiteralPath $key -Value $manifestPath
    }
}
if ($Browser -ne 'Chromium') {
    $manifestPath = Join-Path $manifestDir 'firefox.json'
    @{name='com.udm.download_manager';description='UDM browser download helper';path=$hostPath;type='stdio';allowed_extensions=@('udm@local.example')} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $manifestPath -Encoding ascii
    $key = 'HKCU:\Software\Mozilla\NativeMessagingHosts\com.udm.download_manager'
    New-Item -Path $key -Force | Out-Null
    Set-Item -LiteralPath $key -Value $manifestPath
}
Write-Output 'UDM native messaging registered for the current Windows user. Open the extension and check the desktop connection.'
