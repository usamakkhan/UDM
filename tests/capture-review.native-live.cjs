'use strict';
// Seeded native review + a real canceled Edge response exercises recovery across
// a browser restart. Native decision/UI tests cover how the review was created.
const {chromium}=require('playwright'),http=require('node:http'),fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),assert=require('node:assert/strict'),{spawn,execFileSync}=require('node:child_process');
const root=path.resolve(process.argv[2]),seed=path.resolve(process.argv[3]),project=process.env.UDM_PROJECT||path.resolve(__dirname,'..'),release=process.env.UDM_TEST_RELEASE;
const tag='review'+Date.now(),hostName='com.udm.reviewfixture'+Date.now(),registry='HKCU\\Software\\Microsoft\\Edge\\NativeMessagingHosts\\'+hostName,checks=[];
let context,worker,desktop,server,registered=false,requests=0,token,id,extension;const requestLog=[];
const stateFile=path.join(root,'state/state.json'),state=()=>JSON.parse(fs.readFileSync(stateFile)),wait=ms=>new Promise(r=>setTimeout(r,ms));
async function until(fn){const end=Date.now()+20000;while(Date.now()<end){const result=await fn();if(result)return result;await wait(100);}throw Error('Review recovery fixture timed out');}
function pass(name){checks.push({name,passed:true});console.log('PASS '+name);}
async function browser(load){
 context=await chromium.launchPersistentContext(path.join(root,'profile'),{channel:'msedge',headless:true,ignoreDefaultArgs:['--disable-extensions'],env:{...process.env,UDM_INSTANCE_TAG:tag},args:['--enable-unsafe-extension-debugging']});
 const cdp=await context.browser().newBrowserCDPSession();await cdp.send('Browser.setDownloadBehavior',{behavior:'allow',downloadPath:path.join(root,'browser-downloads'),eventsEnabled:true});if(load)await cdp.send('Extensions.loadUnpacked',{path:extension});
 worker=context.serviceWorkers()[0]||await context.waitForEvent('serviceworker');await until(async()=>{try{return(await worker.evaluate(()=>nativeRequest({action:'ping'}))).ok;}catch{return false;}});
}
(async()=>{
 assert(release&&seed.includes('release-077-build')&&seed.endsWith('state.json'));assert(!fs.existsSync(root));fs.mkdirSync(path.dirname(stateFile),{recursive:true});fs.mkdirSync(path.join(root,'browser-downloads'));
 const source=JSON.parse(fs.readFileSync(seed)),row=Object.entries(source.BrowserCaptures).find(([,r])=>r.status==='review');assert(row);[token]=row;
 fs.writeFileSync(stateFile,JSON.stringify({Schema:1,Settings:{DownloadFolder:path.join(root,'downloads'),CategoryFolders:false,SkipBrowserFileInfo:false,PrefetchFileInfo:false,ClipboardMonitor:false,CloseToTray:false,DropBasket:false,Sound:false},Queues:[{Name:'Main queue',Enabled:false,Parallel:1}],Downloads:[],Projects:[],BrowserCaptures:{[token]:row[1]}}));
 extension=path.join(root,'extension');fs.cpSync(path.join(project,'browser/chromium'),extension,{recursive:true});const manifest=JSON.parse(fs.readFileSync(path.join(extension,'manifest.json')));assert.equal(manifest.version,'0.53.1');
 id=crypto.createHash('sha256').update(Buffer.from(manifest.key,'base64')).digest('hex').slice(0,32).replace(/[0-9a-f]/g,c=>String.fromCharCode(97+parseInt(c,16)));
 const bg=path.join(extension,'background.js');fs.writeFileSync(bg,fs.readFileSync(bg,'utf8').replace("'com.udm.download_manager'",JSON.stringify(hostName)));
 const hostFile=path.join(root,'native-host.json');fs.writeFileSync(hostFile,JSON.stringify({name:hostName,description:'Isolated UDM review recovery acceptance',path:path.join(release,'Udm.NativeHost.exe'),type:'stdio',allowed_origins:['chrome-extension://'+id+'/']}));
 let exists=false;try{execFileSync('reg.exe',['query',registry],{stdio:'ignore',windowsHide:true});exists=true;}catch{}assert(!exists);execFileSync('reg.exe',['add',registry,'/ve','/t','REG_SZ','/d',hostFile,'/f'],{stdio:'ignore',windowsHide:true});registered=true;
 desktop=spawn(path.join(release,'UDM.exe'),['--background','--data-dir',path.dirname(stateFile),'--instance-tag',tag],{stdio:'ignore',windowsHide:true});
 server=http.createServer((req,res)=>{if(req.url==='/'){res.setHeader('Content-Type','text/html');res.end('<a id="download" href="/review.udmreview">Download fixture</a>');return;}if(req.url!=='/review.udmreview'){res.writeHead(404);res.end();return;}++requests;requestLog.push({method:req.method,url:req.url,range:req.headers.range});res.writeHead(200,{'Content-Type':'application/octet-stream','Content-Disposition':'attachment; filename="review.udmreview"','Content-Length':8388608});let bytes=0;const timer=setInterval(()=>{res.write(Buffer.alloc(8192,71));bytes+=8192;if(bytes===8388608){clearInterval(timer);res.end();}},50);res.on('close',()=>clearInterval(timer));});await new Promise(r=>server.listen(0,'127.0.0.1',r));
 await browser(true);const identity=await worker.evaluate(()=>nativeRequest({action:'diagnostics'}));assert.equal(identity.version,'0.78.0');assert.equal(path.resolve(identity.dataDirectory).toLowerCase(),path.dirname(stateFile).toLowerCase());pass('Edge is connected to the isolated native 0.77 catalog');
 const hello=await worker.evaluate(()=>nativeRequest({action:'hello'}));assert(hello.capabilities.includes('capture-review'));pass('Native host advertises captured-link review');
 const origin='http://127.0.0.1:'+server.address().port,url=origin+'/review.udmreview';
 await worker.evaluate(async()=>{const {settings}=await chrome.storage.local.get('settings');await chrome.storage.local.set({settings:{...settings,capture:false}});});
 const page=await context.newPage();await page.goto(origin);await page.click('#download');
 await until(async()=>{const items=await worker.evaluate(()=>chrome.downloads.search({}));return items.some(i=>i.url===url&&i.state==='in_progress');});
 await worker.evaluate(async({url,token})=>{
  const current=(await chrome.downloads.search({})).find(i=>i.url===url);if(!current)throw Error('Missing fixture response');const id=current.id;await chrome.downloads.cancel(id);
  const [item]=await chrome.downloads.search({id});if(item.state!=='interrupted'||item.error!=='USER_CANCELED')throw Error('Browser response did not cancel');
  const fingerprint=Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',new TextEncoder().encode(JSON.stringify([item.id,item.startTime,item.url])))),b=>b.toString(16).padStart(2,'0')).join('');
  await chrome.storage.local.set({reviewRestartMarker:token,captureOwnershipV1:{[token]:{id,token,fingerprint,phase:'committing',protocol:2}}});
 },{url,token});pass('Real Edge response is canceled before the interrupted ownership journal is seeded');
 // CDP-loaded unpacked extensions need reactivation after this isolated browser
 // exits; the profile and extension storage must survive that reactivation.
 await context.close();context=null;await browser(true);
 assert.equal((await worker.evaluate(()=>chrome.storage.local.get('reviewRestartMarker'))).reviewRestartMarker,token);
 await until(async()=>Object.keys((await worker.evaluate(()=>chrome.storage.local.get('captureOwnershipV1'))).captureOwnershipV1||{}).length===0);
 const items=await worker.evaluate(()=>chrome.downloads.search({}));assert.equal(items.length,1);assert.equal(items[0].state,'interrupted');assert.equal(items[0].error,'USER_CANCELED');assert.equal(requests,1);assert.equal(state().Downloads.length,0);assert.equal(state().BrowserCaptures[token].status,'review');pass('Browser restart settles ownership without resuming, replaying or creating a native job');
 await worker.evaluate(()=>chrome.action.openPopup());const cdp=await context.browser().newBrowserCDPSession();
 const target=await until(async()=>{const {targetInfos}=await cdp.send('Target.getTargets');fs.writeFileSync(path.join(root,'popup-targets.json'),JSON.stringify(targetInfos,null,2));return targetInfos.find(t=>t.url==='chrome-extension://'+id+'/popup.html');});
 const {sessionId}=await cdp.send('Target.attachToTarget',{targetId:target.targetId,flatten:false});let nextMessage=0;
 const evaluate=expression=>new Promise((resolve,reject)=>{const messageId=++nextMessage;const timer=setTimeout(()=>{cdp.off('Target.receivedMessageFromTarget',receive);reject(Error('Popup evaluation timeout'));},10000);function receive(event){if(event.sessionId!==sessionId)return;const reply=JSON.parse(event.message);if(reply.id!==messageId)return;clearTimeout(timer);cdp.off('Target.receivedMessageFromTarget',receive);if(reply.error||reply.result?.exceptionDetails)reject(Error(JSON.stringify(reply)));else resolve(reply.result.result.value);}cdp.on('Target.receivedMessageFromTarget',receive);cdp.send('Target.sendMessageToTarget',{sessionId,message:JSON.stringify({id:messageId,method:'Runtime.evaluate',params:{expression,returnByValue:true,awaitPromise:true}})}).catch(reject);});
 await until(()=>evaluate("!!document.getElementById('recover')"));await evaluate("document.getElementById('recover').click();true");
 const status=await until(async()=>{const value=await evaluate("document.getElementById('status').textContent");return value.includes('1 saved links are ready to review in UDM.')&&value;});
 fs.writeFileSync(path.join(root,'popup-status.json'),JSON.stringify({status,targetType:target.type},null,2));pass('Actual Edge popup Recover button opens the saved review through the real host connection');
 assert.equal(requests,1);assert.equal(state().BrowserCaptures[token].request,row[1].request);pass('Manual review preserves the protected request without another network request');
})().catch(e=>{checks.push({passed:false,error:e.stack});console.error(e);process.exitCode=1;}).finally(async()=>{
 if(worker)try{fs.writeFileSync(path.join(root,'browser-diagnostics.json'),JSON.stringify(await worker.evaluate(async()=>({downloads:await chrome.downloads.search({}),storage:await chrome.storage.local.get(null)})),null,2));}catch{}
 if(context)await context.close();if(desktop)desktop.kill();if(server){server.closeAllConnections();await new Promise(r=>server.close(r));}
 if(registered)execFileSync('reg.exe',['delete',registry,'/f'],{stdio:'ignore',windowsHide:true});let remains=false;try{execFileSync('reg.exe',['query',registry],{stdio:'ignore',windowsHide:true});remains=true;}catch{}
 fs.mkdirSync(root,{recursive:true});fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({checks,requests,requestLog,seed,registryRemoved:!remains},null,2));
});
