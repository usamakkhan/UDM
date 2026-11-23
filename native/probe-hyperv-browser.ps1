param([switch]$InspectOnly,[ValidateSet(9222,9224)][int]$Port=9222)
# Run inside the owned guest through PowerShell Direct, never against a personal browser.
$ErrorActionPreference='Stop'
if($env:COMPUTERNAME -ne 'UDM-LAB'){throw 'This probe belongs only to UDM-LAB.'}
$versionRequest=[Net.HttpWebRequest]::Create(('http://127.0.0.1:'+ $Port +'/json/version'))
$versionRequest.Proxy=$null
$versionRequest.Timeout=10000
$versionResponse=$versionRequest.GetResponse()
try {
 $versionReader=New-Object IO.StreamReader($versionResponse.GetResponseStream())
 try {$version=$versionReader.ReadToEnd() | ConvertFrom-Json}finally{$versionReader.Dispose()}
} finally {$versionResponse.Dispose()}
$script:cdp=new-object Net.WebSockets.ClientWebSocket
$script:cdp.Options.Proxy=$null
$script:sequence=0
$script:deadline=New-Object Threading.CancellationTokenSource
$script:deadline.CancelAfter(90000)
function Send-LabCdp([string]$Method,$Parameters=@{},[string]$SessionId=''){
 $script:sequence++
 $id=$script:sequence
 $message=@{id=$id;method=$Method;params=$Parameters}
 if($SessionId){$message.sessionId=$SessionId}
 $data=[Text.Encoding]::UTF8.GetBytes(($message | ConvertTo-Json -Depth 15 -Compress))
 $script:cdp.SendAsync([ArraySegment[byte]]::new($data),[Net.WebSockets.WebSocketMessageType]::Text,$true,$script:deadline.Token).GetAwaiter().GetResult() | Out-Null
 while($true){
  $stream=New-Object IO.MemoryStream
  try {
   do {
    $buffer=New-Object byte[] 65536
    $received=$script:cdp.ReceiveAsync([ArraySegment[byte]]::new($buffer),$script:deadline.Token).GetAwaiter().GetResult()
    if($received.MessageType -eq [Net.WebSockets.WebSocketMessageType]::Close){throw 'Browser closed its control connection.'}
    $stream.Write($buffer,0,$received.Count)
    if($stream.Length -gt 2MB){throw 'Browser control response exceeded the bounded size.'}
   } while(!$received.EndOfMessage)
   $response=[Text.Encoding]::UTF8.GetString($stream.ToArray()) | ConvertFrom-Json
  } finally {$stream.Dispose()}
  if($response.id -eq $id){
   if($response.error){throw ($Method+': '+$response.error.message)}
   return $response.result
  }
 }
}
try {
 $script:cdp.ConnectAsync([uri]$version.webSocketDebuggerUrl,$script:deadline.Token).GetAwaiter().GetResult() | Out-Null
 $targets=Send-LabCdp 'Target.getTargets'
 $page=@($targets.targetInfos | Where-Object {$_.type -eq 'page' -and $_.url -eq 'file:///C:/UDM-Lab/fixtures/control.html'})
 if($page.Count -ne 1){throw 'Expected exactly one owned browser fixture tab.'}
 $attached=Send-LabCdp 'Target.attachToTarget' @{targetId=$page[0].targetId;flatten=$true}
 if($InspectOnly){
  $inspection=Send-LabCdp 'Runtime.evaluate' @{expression="({title:document.title,url:location.href,readyState:document.readyState,text:document.body?.innerText?.slice(0,1500),html:document.documentElement?.outerHTML?.slice(0,1500)})";returnByValue=$true} $attached.sessionId
  $inspection | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath 'C:\UDM-Lab\records\browser-fixture-state.json' -Encoding UTF8
  $inspection | ConvertTo-Json -Depth 6
  return
 }
 Send-LabCdp 'Runtime.runIfWaitingForDebugger' @{} $attached.sessionId | Out-Null
 Send-LabCdp 'Page.enable' @{} $attached.sessionId | Out-Null
 $navigation=Send-LabCdp 'Page.navigate' @{url='file:///C:/UDM-Lab/fixtures/control.html'} $attached.sessionId
 if($navigation.errorText){throw ('Owned fixture navigation failed: '+$navigation.errorText)}
 $pageDeadline=[DateTime]::UtcNow.AddSeconds(20)
 do {
  $pageReady=Send-LabCdp 'Runtime.evaluate' @{expression="!!document.querySelector('#verify')&&!!document.querySelector('#result')";returnByValue=$true} $attached.sessionId
  if($pageReady.result.value -eq $true){break}
  Start-Sleep -Milliseconds 200
 } while([DateTime]::UtcNow -lt $pageDeadline)
 if($pageReady.result.value -ne $true){
  $fixtureState=Send-LabCdp 'Runtime.evaluate' @{expression="({title:document.title,url:location.href,readyState:document.readyState,text:document.body?.innerText?.slice(0,1500)})";returnByValue=$true} $attached.sessionId
  $fixtureState | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath 'C:\UDM-Lab\records\browser-fixture-failure.json' -Encoding UTF8
  throw ('Owned browser fixture did not finish loading: '+($fixtureState | ConvertTo-Json -Depth 6 -Compress))
 }
 $pageCheck=Send-LabCdp 'Runtime.evaluate' @{expression="document.querySelector('#verify').click();({title:document.title,verified:document.body.dataset.verified==='true',message:document.querySelector('#result').textContent})";returnByValue=$true} $attached.sessionId
 if(!$pageCheck.result.value.verified){throw ('Browser input check failed: '+($pageCheck | ConvertTo-Json -Depth 6 -Compress))}
 # A Manifest V3 worker may legitimately be asleep. Open our installed diagnostics
 # page and message the background worker instead of assuming it is always running.
 $diagnostic=Send-LabCdp 'Target.createTarget' @{url='chrome-extension://kahfappnpjdcboccpnhinkcobcdgbdpl/diagnostics.html'}
 $worker=Send-LabCdp 'Target.attachToTarget' @{targetId=$diagnostic.targetId;flatten=$true}
 $readyDeadline=[DateTime]::UtcNow.AddSeconds(20)
 do {
  $ready=Send-LabCdp 'Runtime.evaluate' @{expression="typeof chrome!=='undefined'&&chrome.runtime?chrome.runtime.id:null";returnByValue=$true} $worker.sessionId
  if($ready.result.value -eq 'kahfappnpjdcboccpnhinkcobcdgbdpl'){break}
  Start-Sleep -Milliseconds 200
 } while([DateTime]::UtcNow -lt $readyDeadline)
 if($ready.result.value -ne 'kahfappnpjdcboccpnhinkcobcdgbdpl'){throw 'Installed UDM extension page did not become ready.'}
 $hello=Send-LabCdp 'Runtime.evaluate' @{expression="new Promise(resolve=>chrome.runtime.sendNativeMessage('com.udm.download_manager',{action:'hello',protocol:1,requestId:881},reply=>resolve(chrome.runtime.lastError?{ok:false,error:chrome.runtime.lastError.message}:reply)))";awaitPromise=$true;returnByValue=$true} $worker.sessionId
 if(!$hello.result.value.ok){throw ('UDM native handshake failed: '+($hello.result.value | ConvertTo-Json -Compress))}
 $ping=Send-LabCdp 'Runtime.evaluate' @{expression="new Promise(resolve=>chrome.runtime.sendMessage({action:'ping'},reply=>resolve(chrome.runtime.lastError?{ok:false,error:chrome.runtime.lastError.message}:reply)))";awaitPromise=$true;returnByValue=$true} $worker.sessionId
 if(!$ping.result.value.ok){throw ('UDM background-to-desktop check failed: '+($ping.result.value | ConvertTo-Json -Compress))}
 $manifest=Send-LabCdp 'Runtime.evaluate' @{expression='({version:chrome.runtime.getManifest().version,id:chrome.runtime.id})';returnByValue=$true} $worker.sessionId
 if($manifest.result.value.version -ne '0.73.52'){throw 'Unexpected live extension version.'}
 $record=[ordered]@{passed=$true;browserVersion=$version.Browser;controlPort=$Port;headless=($version.'User-Agent' -match 'HeadlessChrome');loopbackControl=$true;controlTransportUsesDirectLoopback=$true;ownedFixtureInputVerified=$true;page=$pageCheck.result.value;extension=$manifest.result.value;nativeHello=$hello.result.value;backgroundToDesktopPing=$ping.result.value;personalProfilesUsed=$false;completedUtc=[DateTime]::UtcNow.ToString('o')}
 $recordPath=if($Port -eq 9224){'C:\UDM-Lab\records\browser-control-functional.json'}else{'C:\UDM-Lab\records\browser-control.json'}
 $record | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $recordPath -Encoding UTF8
 $record | ConvertTo-Json -Depth 8
} finally {
 if($diagnostic -and $script:cdp.State -eq [Net.WebSockets.WebSocketState]::Open){try{Send-LabCdp 'Target.closeTarget' @{targetId=$diagnostic.targetId} | Out-Null}catch{}}
 $script:cdp.Dispose();$script:deadline.Dispose()
}
