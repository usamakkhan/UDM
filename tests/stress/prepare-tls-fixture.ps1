#requires -Version 7.0
$ErrorActionPreference='Stop'
$udmTlsRoot=Join-Path (Split-Path (Split-Path $PSScriptRoot -Parent) -Parent) 'benchmarks\stress-2026-09-20\tls-fixture'
New-Item -ItemType Directory -Path $udmTlsRoot -Force | Out-Null
$udmKey=[Security.Cryptography.RSA]::Create(2048)
try {
 $udmRequest=[Security.Cryptography.X509Certificates.CertificateRequest]::new('CN=localhost',$udmKey,[Security.Cryptography.HashAlgorithmName]::SHA256,[Security.Cryptography.RSASignaturePadding]::Pkcs1)
 $udmSan=[Security.Cryptography.X509Certificates.SubjectAlternativeNameBuilder]::new()
 $udmSan.AddDnsName('localhost')
 $udmSan.AddIpAddress([Net.IPAddress]::Loopback)
 $udmRequest.CertificateExtensions.Add($udmSan.Build())
 $udmCert=$udmRequest.CreateSelfSigned([DateTimeOffset]::UtcNow.AddMinutes(-5),[DateTimeOffset]::UtcNow.AddDays(1))
 try {
  [IO.File]::WriteAllText((Join-Path $udmTlsRoot 'cert.pem'),$udmCert.ExportCertificatePem())
  [IO.File]::WriteAllText((Join-Path $udmTlsRoot 'key.pem'),$udmKey.ExportPkcs8PrivateKeyPem())
 } finally {$udmCert.Dispose()}
} finally {$udmKey.Dispose()}
Write-Output 'Created an untrusted, one-day loopback fixture certificate. No certificate store or trust setting changed.'