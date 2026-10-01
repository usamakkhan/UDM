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
const pdf=fs.readFileSync(path.join(fixtures,'blank.pdf')),zip=fs.readFileSync(path.join(fixtures,'archive.zip')),html=fs.readFileSync(path.join(fixtures,'error.html'));
const all=[
 {id:'pdf-bin',type:'application/pdf',name:'report.bin',bytes:pdf,capture:true,new:true},
 {id:'zip-bin',type:'application/x-zip-compressed',name:'archive.bin',bytes:zip,capture:true,new:true},
 {id:'pdf-dotless',type:'application/pdf',name:'extensionless',bytes:pdf,capture:true},
 {id:'export.php',type:'application/pdf',bytes:pdf,capture:true},
 {id:'unicode',type:'application/octet-stream',disposition:"attachment; filename*=UTF-8''r%C3%A9sum%C3%A9.pdf",bytes:pdf,capture:true},
 {id:'legacy',type:'application/octet-stream',name:'old.udmfixture',bytes:zip,capture:true},
 {id:'plain-name',type:'application/pdf',name:'keep.txt',bytes:pdf,capture:false},
 {id:'web-error',type:'text/html',name:'error.pdf',bytes:html,capture:false},
 {id:'unknown',type:'application/octet-stream',name:'unknown.bin',bytes:zip,capture:false},
 {id:'local-denial',type:'application/zip',name:'local.bin',bytes:zip,capture:false,local:['pdf']},
 {id:'desktop-denial',type:'text/csv',name:'desktop.bin',bytes:pdf,capture:false,local:['csv']},
 {id:'excluded',type:'application/pdf',name:'excluded.bin',bytes:pdf,capture:false,excluded:true},
 {id:'post',type:'application/pdf',name:'form.bin',bytes:pdf,capture:true,post:true}
];
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
 server=createServer((req,res)=>{
  if(req.url==='/'){res.setHeader('Content-Type','text/html; charset=utf-8');res.end('<!doctype html><title>UDM recognition acceptance</title>'+cases.map(c=>c.post?'<form action="/'+c.id+'" method="post"><input name="value" value="a + b"><button id="'+c.id+'">Download '+c.id+'</button></form>':'<a id="'+c.id+'" href="/'+c.id+'">Download '+c.id+'</a>').join('<br>'));return;}
  const c=cases.find(c=>'/'+c.id===req.url);if(!c){res.writeHead(404);res.end();return;}
  const chunks=[];req.on('data',b=>chunks.push(b));req.on('end',()=>{
   const body=Buffer.concat(chunks);let socketOwners=[];
   if(process.env.UDM_TRACE_SOCKET_OWNERS==='1')try{
    const rows=execFileSync('netstat.exe',['-ano','-p','tcp'],{encoding:'utf8',windowsHide:true,timeout:5000}).split(/\r?\n/).map(line=>line.trim().split(/\s+/));
    socketOwners=rows.filter(row=>row[0]==='TCP'&&row[1].endsWith(':'+req.socket.remotePort)&&row[2].endsWith(':'+req.socket.localPort)).map(row=>{
     const pid=Number(row.at(-1));const text=execFileSync('tasklist.exe',['/FI','PID eq '+pid,'/FO','CSV','/NH'],{encoding:'utf8',windowsHide:true,timeout:5000});return {pid,name:/^"([^"]+)"/m.exec(text)?.[1]||'',state:row[3]};
    });
   }catch(error){socketOwners=[{error:error.message}];}
   requests.push({clientPort:req.socket.remotePort,serverPort:req.socket.localPort,socketOwners,route:req.url,method:req.method,range:req.headers.range||'',body:body.toString(),type:req.headers['content-type']||'',agent:req.headers['user-agent']||''});
   let start=0,end=c.bytes.length-1,status=200;const range=/^bytes=(\d+)-(\d*)$/.exec(req.headers.range||'');
   if(range){start=Number(range[1]);if(range[2])end=Math.min(end,Number(range[2]));status=206;}
   const headers={'Content-Type':c.type,'Content-Disposition':c.disposition||('attachment'+(c.name?'; filename="'+c.name+'"':'')),'Content-Length':end-start+1,'Accept-Ranges':'bytes','ETag':'"fixture-'+c.id+'"'};
   if(status===206)headers['Content-Range']='bytes '+start+'-'+end+'/'+c.bytes.length;
   res.writeHead(status,headers);if(req.method==='HEAD'){res.end();return;}
   let at=start;const timer=setInterval(()=>{const next=Math.min(at+32768,end+1);res.write(c.bytes.subarray(at,next));at=next;if(at>end){clearInterval(timer);res.end();}},20);res.on('close',()=>clearInterval(timer));
  });
 });await new Promise(r=>server.listen(0,bind,r));origin=(tlsDir?'https://':'http://')+host+':'+server.address().port;
 context=await chromium.launchPersistentContext(path.join(root,'profile'),{...(process.env.UDM_EDGE_EXECUTABLE?{executablePath:process.env.UDM_EDGE_EXECUTABLE}:{channel:'msedge'}),headless:true,acceptDownloads:true,ignoreDefaultArgs:['--disable-extensions'],env:{...process.env,UDM_INSTANCE_TAG:tag},args:['--enable-unsafe-extension-debugging']});
 const cdp=await context.browser().newBrowserCDPSession();await cdp.send('Browser.setDownloadBehavior',{behavior:'allow',downloadPath:path.join(root,'browser-downloads'),eventsEnabled:true});await cdp.send('Extensions.loadUnpacked',{path:ext});
 worker=context.serviceWorkers()[0]||await context.waitForEvent('serviceworker');await until(async()=>{try{return(await worker.evaluate(()=>nativeRequest({action:'ping'}))).ok;}catch{return false;}});
 const identity=await worker.evaluate(()=>nativeRequest({action:'diagnostics'}));assert.equal(path.resolve(identity.dataDirectory).toLowerCase(),path.dirname(stateFile).toLowerCase());assert.equal(identity.version,baseline?'0.74.0':'0.78.0');
 await worker.evaluate(()=>{globalThis.__recognitionTrace=[];chrome.downloads.onCreated.addListener(item=>__recognitionTrace.push({event:'created',id:item.id,url:item.url,filename:item.filename,mime:item.mime}));const original=requestContext.resolveDownload;requestContext.resolveDownload=async(...args)=>{const value=await original(...args);__recognitionTrace.push({event:'context',item:args[0],response:value?.response,mime:value?.mime,method:value?.method});return value;};});
 page=context.pages()[0]||await context.newPage();
 page.on('download',download=>browserEvents.push({event:'download',url:download.url(),filename:download.suggestedFilename()}));
 page.on('requestfailed',request=>browserEvents.push({event:'requestfailed',url:request.url(),error:request.failure()?.errorText}));
 page.on('response',response=>{if(response.url().startsWith(origin+'/'))browserEvents.push({event:'response',url:response.url(),status:response.status(),headers:response.headers()});});
 const network=await context.newCDPSession(page);await network.send('Network.enable');
 network.on('Network.requestWillBeSent',event=>{if(event.request.url.startsWith(origin+'/'))browserEvents.push({event:'network-request',url:event.request.url,type:event.type,initiator:event.initiator.type});});
 pass('Native '+identity.version+' connection uses an isolated test catalog');
 await page.goto(origin+'/');
 const downloads=url=>worker.evaluate(async url=>(await chrome.downloads.search({})).filter(item=>item.url===url||item.finalUrl===url),url);
 for(const c of cases){
  await worker.evaluate(async options=>{const {settings}=await chrome.storage.local.get('settings');await chrome.storage.local.set({settings:{...settings,capture:true,extensions:options.local||['pdf','zip','udmfixture'],excluded:options.excluded?[options.host]:[],cookies:false}});},{...c,host});
  // These links/forms return attachments, not documents. Ownership assertions
  // below wait for the actual download; a navigation wait can add 30s per click.
  const url=origin+'/'+c.id,start=performance.now();await page.locator('[id="'+c.id+'"]').click({noWaitAfter:true});
  console.log('CLICK '+c.id+' '+Math.round(performance.now()-start)+'ms');
  const created=await until(async()=>{const [item]=await downloads(url);return item;});
  const capture=c.capture&&!baseline;
  if(capture){
   const job=await until(()=>{const j=jobs().find(j=>j.Url===url);if(j?.Status==='Failed')throw Error(j.Error);return j?.Status==='Complete'&&j;});
   const filename=path.join(job.Folder,job.FileName);assert.equal(hash(fs.readFileSync(filename)),hash(c.bytes));
   const browserItem=await until(async()=>{const [item]=await downloads(url);return item?.state==='interrupted'&&item.error==='USER_CANCELED'&&item;});
   if(c.new)assert.equal(path.extname(browserItem.filename),'.bin');
   if(c.id==='unicode')assert.equal(job.FileName,'résumé.pdf');
   pass(c.id+' captures the file with matching output bytes',{bytes:c.bytes.length,browserFilename:path.basename(browserItem.filename),nativeFilename:job.FileName,mime:browserItem.mime,elapsedMs:Math.round(performance.now()-start)});
   pass(c.id+' finishes with Edge canceled and native ownership',{state:browserItem.state});
   if(c.post){const seen=requests.filter(r=>r.route==='/'+c.id);assert.equal(seen.length,2);assert(seen.every(r=>r.method==='POST'&&!r.range));assert.equal(seen[0].body,seen[1].body);assert.equal(seen[0].type,seen[1].type);pass('MIME recognition preserves the POST body without GET/HEAD probes');}
  }else{
   const browserItem=await until(async()=>{const [item]=await downloads(url);return item?.state==='complete'&&item;});
   assert(!jobs().some(j=>j.Url===url));assert.equal(hash(fs.readFileSync(browserItem.filename)),hash(c.bytes));assert.equal(requests.filter(r=>r.route==='/'+c.id).length,1);
   pass((baseline?'0.47.1 baseline ':'')+c.id+' remains complete in Edge with one original request',{bytes:c.bytes.length,filename:path.basename(browserItem.filename),mime:browserItem.mime,elapsedMs:Math.round(performance.now()-start)});
  }
 }
 const connection=await worker.evaluate(()=>nativeClient.diagnostics());assert.equal(connection.connections,1);assert.equal(connection.persistent,1);pass('All handoffs share one persistent native connection');
})().catch(e=>{checks.push({passed:false,error:e.stack});console.error(e);process.exitCode=1;}).finally(async()=>{
 if(worker)try{fs.writeFileSync(path.join(root,'trace.json'),JSON.stringify(await worker.evaluate(async()=>({trace:__recognitionTrace,lastError:(await chrome.storage.local.get('lastError')).lastError,downloads:(await chrome.downloads.search({})).map(x=>({url:x.url,filename:x.filename,mime:x.mime,state:x.state,error:x.error}))})),null,2));}catch{}
 fs.mkdirSync(root,{recursive:true});fs.writeFileSync(path.join(root,'requests.json'),JSON.stringify(requests,null,2));fs.writeFileSync(path.join(root,'browser-events.json'),JSON.stringify(browserEvents,null,2));fs.writeFileSync(path.join(root,'jobs.json'),JSON.stringify(jobs().map(j=>({Status:j.Status,Error:j.Error,Url:j.Url,FileName:j.FileName,Size:j.Size})),null,2));
 if(context)await context.close();if(desktop)desktop.kill();if(server){server.closeAllConnections();await new Promise(r=>server.close(r));}
 for(const key of nativeKeys)try{execFileSync('reg.exe',['delete',key,'/f'],{windowsHide:true,stdio:'ignore'});}catch{process.exitCode=1;}
 fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({checks,baseline,bind,tls:!!tlsDir,requestCount:requests.length,registryRemoved:!nativeKeys.some(key=>{try{execFileSync('reg.exe',['query',key],{windowsHide:true,stdio:'ignore'});return true;}catch{return false;}})},null,2));
});

