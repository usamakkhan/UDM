param(
 [string]$PackageRoot='C:\UdmTestPackage',
 [string]$OutputRoot='C:\UdmTestResults',
 [switch]$AllowHostDiagnostic,
 [string]$HostWorkRoot
)
$ErrorActionPreference='Stop'
if(!$AllowHostDiagnostic -and $env:USERNAME -ne 'WDAGUtilityAccount'){throw 'This runner is for Windows Sandbox. Host qualification requires explicit AllowHostDiagnostic and a fresh HostWorkRoot.'}
if($AllowHostDiagnostic -and !$HostWorkRoot){throw 'Host diagnostics require an explicit fresh work folder.'}
$package=[IO.Path]::GetFullPath($PackageRoot);$output=[IO.Path]::GetFullPath($OutputRoot)
$manifest=Get-Content -LiteralPath (Join-Path $package 'package.json') -Raw | ConvertFrom-Json
if($manifest.schema -ne 1 -or [string]::IsNullOrWhiteSpace($manifest.packageId)){throw 'Invalid package.'}
$marker=(Get-Content -LiteralPath (Join-Path $output '.udm-sandbox-output') -Raw).Trim()
if($marker -ne $manifest.packageId){throw 'Output mapping belongs to another package.'}
foreach($row in $manifest.files){
 if([IO.Path]::GetFileName($row.name) -ne $row.name -or (Get-FileHash -LiteralPath (Join-Path $package $row.name) -Algorithm SHA256).Hash -ne $row.sha256){throw 'Package hash mismatch.'}
}
$work=if($AllowHostDiagnostic){[IO.Path]::GetFullPath($HostWorkRoot)}else{Join-Path $env:LOCALAPPDATA ('UdmSandboxSmoke-'+[guid]::NewGuid().ToString('N'))}
if(Test-Path -LiteralPath $work){throw 'Private work folder must be new.'}
if($work.StartsWith($package+'\',[StringComparison]::OrdinalIgnoreCase) -or $work.StartsWith($output+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Private app state must stay outside mapped folders.'}
$resultFolder=Join-Path $output ([guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work,$resultFolder,(Join-Path $work 'app'),(Join-Path $work 'files'),(Join-Path $work 'catalog') | Out-Null
$result=[ordered]@{passed=$false;packageId=$manifest.packageId;nativeVersion=$manifest.nativeVersion;executionEnvironment=$(if($AllowHostDiagnostic){'host-runner-qualification'}else{'windows-sandbox'});sandboxAcceptance=$false;scope='Portable CLI 4 MiB download, hash, catalog and normal process exit';installedAppChanged=$false;browserTested=$false;idmCompared=$false;error=$null}
$origin=$null;$process=$null
try {
 Add-Type -TypeDefinition @'
using System;
using System.Net;
using System.Net.Sockets;
using System.IO;
using System.Text;
using System.Threading;
namespace UdmSandbox {
 public sealed class Origin : IDisposable {
  readonly TcpListener listener; readonly Thread worker; readonly byte[] body;
  volatile bool stopping; long sent; int requests;
  public int Port { get { return ((IPEndPoint)listener.LocalEndpoint).Port; } }
  public long BytesSent { get { return Interlocked.Read(ref sent); } }
  public int Requests { get { return Interlocked.CompareExchange(ref requests,0,0); } }
  public byte[] Body { get { return body; } }
  public Origin(int size) {
   body=new byte[size]; for(int i=0;i<size;i++)body[i]=(byte)(((long)i*31+7)%251);
   listener=new TcpListener(IPAddress.Loopback,0);listener.Start();
   worker=new Thread(Run);worker.IsBackground=true;worker.Start();
  }
  void Run() {
   while(!stopping)try { using(TcpClient client=listener.AcceptTcpClient()) {
    client.ReceiveTimeout=3000; client.SendTimeout=5000;
    using(NetworkStream stream=client.GetStream()) {
     StringBuilder header=new StringBuilder();int value;
     while(header.Length<16384&&(value=stream.ReadByte())>=0) {header.Append((char)value);if(header.ToString().EndsWith("\r\n\r\n"))break;}
     string line=header.ToString().Split('\n')[0];
     if(!line.StartsWith("GET /sample.bin "))throw new IOException("Unexpected request");
     Interlocked.Increment(ref requests);
     byte[] response=Encoding.ASCII.GetBytes("HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: "+body.Length+"\r\nAccept-Ranges: none\r\nConnection: close\r\n\r\n");
     stream.Write(response,0,response.Length);
     for(int at=0;at<body.Length;) {int take=Math.Min(65536,body.Length-at);stream.Write(body,at,take);Interlocked.Add(ref sent,take);at+=take;}
    }
   }} catch(Exception) {if(stopping)return;}
  }
  public void Dispose() {stopping=true;listener.Stop();worker.Join(5000);}
 }
}
'@
 $origin=New-Object UdmSandbox.Origin ([int]$manifest.expectedBytes)
 $hash=[Security.Cryptography.SHA256]::Create()
 try {$expected=([BitConverter]::ToString($hash.ComputeHash($origin.Body))).Replace('-','')} finally {$hash.Dispose()}
 $app=Join-Path $work 'app\UDM.exe';Copy-Item -LiteralPath (Join-Path $package 'UDM.exe') -Destination $app
 $settings=@{Connections=1;Parallel=1;DownloadFolder=(Join-Path $work 'files');CategoryFolders=$false;SuppressProgressDialog=$true;SuppressCompletionDialog=$true;Sound=$false;ClipboardMonitor=$false;PrefetchFileInfo=$false;StopOnWebPage=$false;ShowTipsOnStartup=$false}
 @{Schema=1;Settings=$settings;Downloads=@();Queues=@()} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $work 'catalog\state.json') -Encoding UTF8
 $arguments=@('--instance-tag',('sandbox-'+[guid]::NewGuid().ToString('N').Substring(0,20)),'--data-dir',(Join-Path $work 'catalog'),'/d',('http://127.0.0.1:'+$origin.Port+'/sample.bin'),'/p',(Join-Path $work 'files'),'/f','sample.bin','/n','/q')
 $quoted=@($arguments | ForEach-Object {if($_.Contains('"') -or $_.EndsWith('\')){throw 'Unsupported quoted argument.'};'"'+$_+'"'}) -join ' '
 $process=Start-Process -FilePath $app -ArgumentList $quoted -WorkingDirectory (Split-Path $app -Parent) -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $work 'stdout.txt') -RedirectStandardError (Join-Path $work 'stderr.txt')
 if(!$process.WaitForExit(60000)){throw ('Owned test process still running; inspect PID '+$process.Id+' before retrying.')}
 $process.WaitForExit();$process.Refresh()
 if($process.ExitCode -ne 0){throw ('UDM exited with code '+$process.ExitCode)}
 $payload=Join-Path $work 'files\sample.bin';$actual=(Get-FileHash -LiteralPath $payload -Algorithm SHA256).Hash
 $catalog=Get-Content -LiteralPath (Join-Path $work 'catalog\state.json') -Raw | ConvertFrom-Json
 $row=@($catalog.Downloads)
 if((Get-Item -LiteralPath $payload).Length -ne $manifest.expectedBytes -or $actual -ne $expected -or $row.Count -ne 1 -or $row[0].Status -ne 'Complete' -or $row[0].Sha256 -ne $expected){throw 'Completed bytes, hash or catalog mismatch.'}
 if($origin.Requests -ne 1 -or $origin.BytesSent -ne $manifest.expectedBytes){throw 'Unexpected origin request/body totals.'}
 $result.bytes=$manifest.expectedBytes;$result.sha256=$actual;$result.catalogComplete=$true;$result.normalProcessExit=$true;$result.requests=$origin.Requests;$result.originBytes=$origin.BytesSent
 if((Get-Item -LiteralPath $payload).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Unexpected payload link.'}
 Remove-Item -LiteralPath $payload
 $result.payloadRemoved=$true;$result.passed=$true;$result.sandboxAcceptance=!$AllowHostDiagnostic
} catch {$result.error=$_.Exception.Message.Replace($work,'[private-work]').Replace($package,'[package]').Replace($output,'[output]')}
finally {
 if($null -ne $origin){$origin.Dispose()}
 $result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $resultFolder 'results.json') -Encoding UTF8
}
$result | ConvertTo-Json -Depth 6
if(!$result.passed){exit 1}
