'use strict';
const {chromium}=require('playwright'),http=require('node:http'),fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),assert=require('node:assert/strict'),{spawn,execFileSync}=require('node:child_process');
const project=process.env.UDM_PROJECT||path.resolve(__dirname,'..'),root=path.resolve(process.argv[2]),fixtures=path.resolve(process.argv[3]),release=process.env.UDM_TEST_RELEASE||'D:/UDM/release',baseline=process.env.UDM_RECOGNITION_BASELINE==='1';
const tag='recognition'+Date.now(),hostName='com.udm.recognitionfixture'+Date.now(),nativeKeys=[],checks=[],requests=[],browserEvents=[],stateFile=path.join(root,'state/state.json');
const tlsDir=process.env.UDM_TEST_TLS_DIR,bind=process.env.UDM_FIXTURE_IP||'127.0.0.1',host=tlsDir?'localhost':bind.includes(':')?'['+bind+']':bind;
const createServer=tlsDir?handler=>require('node:https').createServer({key:fs.readFileSync(path.join(tlsDir,'key.pem')),cert:fs.readFileSync(path.join(tlsDir,'cert.pem'))},handler):http.createServer;
let server,desktop,context,worker,page,origin;
const pause=ms=>new Promise(r=>setTimeout(r,ms)),hash=b=>crypto.createHash('sha256').update(b).digest('hex');
async function until(fn,timeout=30000){const end=Date.now()+timeout;while(Date.now()<end){const value=await fn();if(value)return value;await pause(80);}throw Error('Recognition fixture timeout');}
function jobs(){try{return JSON.parse(fs.readFileSync(stateFile)).Downloads;}catch{return [];}}
function pass(name,detail={}){checks.push({passed:true,name,...detail});console.log('PASS '+name);}
const pdf=fs.readFileSync(path.join(fixtures,'reference.pdf')),zip=fs.readFileSync(path.join(fixtures,'reference.zip')),reference=JSON.parse(fs.readFileSync(path.join(__dirname,'w3-files.json')));
const all=Array.from({length:6},(_,i)=>({id:'zip-owner-'+i,name:'archive-'+i+'.bin',bytes:zip,capture:true,file:'zip'}));
for(const c of all)c.url=reference.find(f=>f.id===c.file).finalUrl+'?udm-acceptance='+tag+'-'+c.id;

let cases=baseline?all.filter(x=>x.new).sort((a,b)=>a.id.localeCompare(b.id)).reverse():all;
if(process.env.UDM_RECOGNITION_CASES)cases=cases.filter(c=>process.env.UDM_RECOGNITION_CASES.split(',').includes(c.id));
if(process.env.UDM_RECOGNITION_METHOD==='POST')cases=cases.map(c=>({...c,post:true}));
(async()=>{
 assert(!fs.existsSync(root),'Use a fresh isolated output');fs.mkdirSync(path.dirname(stateFile),{recursive:true});fs.mkdirSync(path.join(root,'browser-downloads'));
 const ext=path.join(root,'extension');fs.cpSync(path.join(project,'browser/chromium'),ext,{recursive:true});
 const manifest=JSON.parse(fs.readFileSync(path.join(ext,'manifest.json')));assert.equal(manifest.version,baseline?'0.47.1':'0.53.1');
 const id=crypto.createHash('sha256').update(Buffer.from(manifest.key,'base64')).digest('hex').slice(0,32).replace(/[0-9a-f]/g,c=>String.fromCharCode(97+parseInt(c,16)));
 const hostFile=path.join(root,'native-host.json');fs.writeFileSync(hostFile,JSON.stringify({name:hostName,description:'Isolated UDM recognition fixture',path:path.join(release,'Udm.NativeHost.exe'),type:'stdio',allowed_origins:['chrome-extension://'+id+'/']}));
 const bg=path.join(ext,'background.js');fs.writeFileSync(bg,fs.readFileSync(bg,'utf8').replace("'com.udm.download_manager'",JSON.stringify(hostName)));
 const key='HKCU\\Software\\Microsoft\\Edge\\NativeMessagingHosts\\'+hostName;let exists=false;try{execFileSync('reg.exe',['query',key],{windowsHide:true,stdio:'ignore'});exists=true;}catch{}assert(!exists);
 execFileSync('reg.exe',['add',key,'/ve','/t','REG_SZ','/d',hostFile,'/f'],{windowsHide:true,stdio:'ignore'});nativeKeys.push(key);
 fs.writeFileSync(stateFile,JSON.stringify({Schema:1,Settings:{DownloadFolder:path.join(root,'downloads'),CategoryFolders:false,ProxyMode:'Connect directly',SkipBrowserFileInfo:true,PrefetchFileInfo:true,SuppressProgressDialog:true,SuppressCompletionDialog:true,Sound:false,CaptureExtensions:'pdf zip udmfixture',DuplicatePolicy:'Numbered',Connections:8},Queues:[{Name:'Main queue',Enabled:true,Parallel:2}],Downloads:[],Projects:[]}));
 desktop=spawn(path.join(release,'UDM.exe'),['--background','--data-dir',path.dirname(stateFile),'--instance-tag',tag],{windowsHide:true,stdio:'ignore'});
 origin='https://www.w3.org';
 context=await chromium.launchPersistentContext(path.join(root,'profile'),{...(process.env.UDM_EDGE_EXECUTABLE?{executablePath:process.env.UDM_EDGE_EXECUTABLE}:{channel:'msedge'}),headless:true,acceptDownloads:true,ignoreDefaultArgs:['--disable-extensions'],env:{...process.env,UDM_INSTANCE_TAG:tag},args:['--enable-unsafe-extension-debugging']});
 const cdp=await context.browser().newBrowserCDPSession();await cdp.send('Browser.setDownloadBehavior',{behavior:'allow',downloadPath:path.join(root,'browser-downloads'),eventsEnabled:true});await cdp.send('Extensions.loadUnpacked',{path:ext});
 worker=context.serviceWorkers()[0]||await context.waitForEvent('serviceworker');await until(async()=>{try{return(await worker.evaluate(()=>nativeRequest({action:'ping'}))).ok;}catch{return false;}});
 const identity=await worker.evaluate(()=>nativeRequest({action:'diagnostics'}));assert.equal(path.resolve(identity.dataDirectory).toLowerCase(),path.dirname(stateFile).toLowerCase());assert.equal(identity.version,'0.78.0');
 await worker.evaluate(()=>{globalThis.__recognitionTrace=[];const submit=captureRecovery.submit;captureRecovery.submit=async(...args)=>{const result=await submit(...args);__recognitionTrace.push({event:'submission',url:args[1].url,browserRetained:result?.browserRetained===true});return result;};chrome.downloads.onCreated.addListener(item=>__recognitionTrace.push({event:'created',id:item.id,url:item.url,filename:item.filename,mime:item.mime}));const original=requestContext.resolveDownload;requestContext.resolveDownload=async(...args)=>{const value=await original(...args);__recognitionTrace.push({event:'context',item:args[0],response:value?.response,mime:value?.mime,method:value?.method});return value;};});
 page=context.pages()[0]||await context.newPage();
 page.on('download',download=>browserEvents.push({event:'download',url:download.url(),filename:download.suggestedFilename()}));
 page.on('requestfailed',request=>browserEvents.push({event:'requestfailed',url:request.url(),error:request.failure()?.errorText}));
 page.on('response',response=>{if(response.url().startsWith(origin+'/'))browserEvents.push({event:'response',url:response.url(),status:response.status(),headers:response.headers()});});
 const network=await context.newCDPSession(page);await network.send('Network.enable');
 network.on('Network.requestWillBeSent',event=>{if(event.request.url.startsWith(origin+'/'))browserEvents.push({event:'network-request',url:event.request.url,type:event.type,initiator:event.initiator.type});});
 pass('Native candidate connection uses an isolated test catalog');assert.equal((await worker.evaluate(()=>nativeRequest({action:'preferences'}))).captureTransaction,1);pass('Desktop advertises prepared ownership protocol');
 await page.goto(origin+'/',{waitUntil:'domcontentloaded'});
 await page.evaluate(cases=>{document.title='UDM isolated public HTTPS acceptance';document.body.replaceChildren();for(const c of cases){const link=document.createElement('a');link.id=c.id;link.href=c.url;link.download=c.name;link.textContent='Download '+c.id;link.style.display='block';document.body.append(link);}},cases.map(({id,url,name})=>({id,url,name})));
 const downloads=url=>worker.evaluate(async url=>(await chrome.downloads.search({})).filter(item=>item.url===url||item.finalUrl===url),url);
 for(const c of cases){
  await worker.evaluate(async options=>{const {settings}=await chrome.storage.local.get('settings');await chrome.storage.local.set({settings:{...settings,capture:true,extensions:options.local||['pdf','zip','udmfixture'],excluded:options.excluded?[options.host]:[],cookies:false}});},{...c,host:'www.w3.org'});
  const url=c.url,start=performance.now();await network.send('Network.emulateNetworkConditions',{offline:false,latency:0,downloadThroughput:Math.min(65536,Math.floor(c.bytes.length/4)),uploadThroughput:65536});await page.locator('[id="'+c.id+'"]').click();
  const created=await until(async()=>{const [item]=await downloads(url);return item;});
  const outcome=await until(async()=>{
   const matching=jobs().filter(j=>j.Url===url),[browserItem]=await downloads(url);
   assert(matching.length<=1,'More than one native job was created');
   if(matching[0]?.Status==='Failed')throw Error(matching[0].Error);
   if(matching[0]?.Status==='Complete'||browserItem?.state==='complete')return {native:matching[0],browser:browserItem};
  });
  await until(async()=>!(await worker.evaluate(async()=>Object.keys((await chrome.storage.local.get('captureOwnershipV1')).captureOwnershipV1||{}))).length);
  const matching=jobs().filter(j=>j.Url===url),[browserItem]=await downloads(url);
  if(browserItem.state==='complete'){
   assert.equal(matching.length,0,'Both browser and native own the completed response');
   assert.equal(hash(fs.readFileSync(browserItem.filename)),hash(c.bytes));
   pass(c.id+' completes once in Edge and creates no native job',{owner:'Edge',elapsedMs:Math.round(performance.now()-start)});
  }else{
   assert.equal(matching.length,1);assert.equal(matching[0].Status,'Complete');assert.equal(browserItem.state,'interrupted');assert.equal(browserItem.error,'USER_CANCELED');
   assert.equal(hash(fs.readFileSync(path.join(matching[0].Folder,matching[0].FileName))),hash(c.bytes));
   pass(c.id+' completes once in UDM with Edge canceled',{owner:'UDM',elapsedMs:Math.round(performance.now()-start)});
  }

 }
 const connection=await worker.evaluate(()=>nativeClient.diagnostics());assert.equal(connection.connections,1);assert.equal(connection.persistent,1);pass('All handoffs share one persistent native connection');
})().catch(e=>{checks.push({passed:false,error:e.stack});console.error(e);process.exitCode=1;}).finally(async()=>{
 if(worker)try{fs.writeFileSync(path.join(root,'trace.json'),JSON.stringify(await worker.evaluate(async()=>({trace:__recognitionTrace,lastError:(await chrome.storage.local.get('lastError')).lastError,downloads:(await chrome.downloads.search({})).map(x=>({url:x.url,filename:x.filename,mime:x.mime,state:x.state,error:x.error}))})),null,2));}catch{}
 fs.mkdirSync(root,{recursive:true});fs.writeFileSync(path.join(root,'requests.json'),JSON.stringify(requests,null,2));fs.writeFileSync(path.join(root,'browser-events.json'),JSON.stringify(browserEvents,null,2));fs.writeFileSync(path.join(root,'jobs.json'),JSON.stringify(jobs().map(j=>({Status:j.Status,Error:j.Error,Url:j.Url,FileName:j.FileName,Size:j.Size})),null,2));
 if(context)await context.close();if(desktop)desktop.kill();if(server){server.closeAllConnections();await new Promise(r=>server.close(r));}
 for(const key of nativeKeys)try{execFileSync('reg.exe',['delete',key,'/f'],{windowsHide:true,stdio:'ignore'});}catch{process.exitCode=1;}
 fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({checks,baseline,bind,tls:true,certificateValidation:true,trustStoreChanged:false,fixture:'Six repeated W3C ZIP ownership races; this does not establish recognition acceptance',requestCount:requests.length,registryRemoved:!nativeKeys.some(key=>{try{execFileSync('reg.exe',['query',key],{windowsHide:true,stdio:'ignore'});return true;}catch{return false;}})},null,2));
});
