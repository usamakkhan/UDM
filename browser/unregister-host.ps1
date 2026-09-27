$ErrorActionPreference = 'Stop'
foreach ($vendor in @('Google\Chrome','Microsoft\Edge','Chromium','BraveSoftware\Brave-Browser','Vivaldi','Mozilla')) {
    $key = "HKCU:\Software\$vendor\NativeMessagingHosts\com.udm.download_manager"
    if (Test-Path -LiteralPath $key) { Remove-Item -LiteralPath $key }
}
Write-Output 'UDM native messaging registration removed. Download files and history were retained.'
