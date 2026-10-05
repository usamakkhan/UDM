'use strict';
const {chromium}=require('playwright'),http=require('node:http'),fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),assert=require('node:assert/strict'),{spawn,execFileSync}=require('node:child_process');
const root=path.resolve(process.argv[2]),project=process.env.UDM_PROJECT||path.resolve(__dirname,'..'),release=process.env.UDM_TEST_RELEASE||'D:/UDM/release',baseline=process.env.UDM_FILES_BASELINE==='1';
const tag='fileforms'+Date.now(),hostName='com.udm.filefixture'+Date.now(),registry='HKCU\\Software\\Microsoft\\Edge\\NativeMessagingHosts\\'+hostName;
const stateFile=path.join(root,'state/state.json'),checks=[],requests=[],diagnostics=[],payload=Buffer.alloc(2*1024*1024+19,71),upload=Buffer.from(Array.from({length:16384},(_,i)=>i%256));
let context,worker,page,server,desktop,origin,registered=false;
const hash=b=>crypto.createHash('sha256').update(b).digest('hex'),wait=ms=>new Promise(r=>setTimeout(r,ms));
const jobs=()=>{try{return JSON.parse(fs.readFileSync(stateFile)).Downloads;}catch{return [];}};
async function until(fn){const end=Date.now()+30000;while(Date.now()<end){const result=await fn();if(result)return result;await wait(80);}throw Error('File form fixture timeout');}
const pass=(name,detail={})=>{checks.push({passed:true,name,...detail});console.log('PASS '+name);};
const cases=[
 {id:'binary',fields:'<input name="label" value="before"><input type="file" name="upload" id="input-binary"><input name="label" value="after">',file:true},
 {id:'percentfile',fields:'<input type="file" name="upload" id="input-percentfile">',file:true,filename:'literal%22%GG+é.bin'},
 {id:'near-limit',fields:'<input type="file" name="upload" id="input-near-limit">',file:true,near:true},
 {id:'disk',fields:'<input type="file" name="upload" id="input-disk">',file:true,disk:true},
 {id:'multiple',fields:'<input name="z" value="first"><input type="file" name="upload" id="input-multiple" multiple><input name="z" value="last">',file:true,multiple:true},
 {id:'empty',fields:'<input type="file" name="upload">'},
 {id:'blob',fields:'<input name="label" value="blob">'},
 {id:'quoted',fields:'<input name="label" value="quoted">'},
 {id:'textescapes',fields:'<input name="literal%22%GG" value="percent">'},
 {id:'redirect',fields:'<input type="file" name="upload" id="input-redirect">',file:true,route:'redirected'},
 {id:'mutated',fields:'<input type="file" name="upload" id="input-mutated">',file:true,keep:true},
 {id:'oversized',fields:'<input type="file" name="upload" id="input-oversized">',file:true,large:true,keep:true},
 {id:'nonutf8',fields:'<input name="label" value="café"><input type="file" name="upload" id="input-nonutf8">',file:true,keep:true,attrs:'accept-charset="windows-1252"'}
];
(async()=>{
 assert(!fs.existsSync(root),'Use a fresh isolated output directory');fs.mkdirSync(path.dirname(stateFile),{recursive:true});fs.mkdirSync(path.join(root,'browser-downloads'));
 const ext=path.join(root,'extension');fs.cpSync(path.join(project,'browser/chromium'),ext,{recursive:true});
 const manifest=JSON.parse(fs.readFileSync(path.join(ext,'manifest.json')));assert.equal(manifest.version,baseline?'0.47.1':'0.53.1');
 const id=crypto.createHash('sha256').update(Buffer.from(manifest.key,'base64')).digest('hex').slice(0,32).replace(/[0-9a-f]/g,c=>String.fromCharCode(97+parseInt(c,16)));
 const hostFile=path.join(root,'native-host.json');fs.writeFileSync(hostFile,JSON.stringify({name:hostName,description:'Isolated UDM file-form acceptance',path:path.join(release,'Udm.NativeHost.exe'),type:'stdio',allowed_origins:['chrome-extension://'+id+'/']}));
 const bg=path.join(ext,'background.js');fs.writeFileSync(bg,fs.readFileSync(bg,'utf8').replace("'com.udm.download_manager'",JSON.stringify(hostName)));
 let exists=false;try{execFileSync('reg.exe',['query',registry],{windowsHide:true,stdio:'ignore'});exists=true;}catch{}assert(!exists);
 execFileSync('reg.exe',['add',registry,'/ve','/t','REG_SZ','/d',hostFile,'/f'],{windowsHide:true,stdio:'ignore'});registered=true;
 fs.writeFileSync(stateFile,JSON.stringify({Schema:1,Settings:{DownloadFolder:path.join(root,'downloads'),CategoryFolders:false,ProxyMode:'Connect directly',SkipBrowserFileInfo:true,PrefetchFileInfo:true,SuppressProgressDialog:true,SuppressCompletionDialog:true,Sound:false,CaptureExtensions:'udmform',DuplicatePolicy:'Numbered',Connections:8},Queues:[{Name:'Main queue',Enabled:true,Parallel:2}],Downloads:[],Projects:[]}));
 desktop=spawn(path.join(release,'UDM.exe'),['--background','--data-dir',path.dirname(stateFile),'--instance-tag',tag],{windowsHide:true,stdio:'ignore'});
 server=http.createServer((req,res)=>{
  if(req.url==='/'){
   res.setHeader('Content-Type','text/html; charset=utf-8');
   res.end('<!doctype html><meta charset="utf-8"><title>UDM File form acceptance</title>'+cases.map(c=>'<form id="form-'+c.id+'" action="/'+c.id+'.udmform" method="post" enctype="multipart/form-data" '+(c.attrs||'')+'>'+c.fields+'<button id="'+c.id+'">Download '+c.id+'</button></form>').join('<hr>')+`<script>
    document.querySelector('#form-blob').addEventListener('formdata',e=>e.formData.append('blob',new Blob([new Uint8Array([0,128,255,13,10,1])],{type:'application/octet-stream'})));
    document.querySelector('#form-quoted').addEventListener('formdata',e=>e.formData.append('quoted',new File([new Uint8Array([255,0,10])],'quote"\\rfile.bin',{type:'application/octet-stream'})));
    document.querySelector('#form-textescapes').addEventListener('formdata',e=>e.formData.append('quote"\\rname','one\\ntwo'));
    document.querySelector('#form-mutated').addEventListener('formdata',e=>{const prior=e.formData.get('upload');e.formData.set('upload',new File([new Uint8Array(prior.size).fill(77)],prior.name,{type:prior.type}));});
   </script>`);return;
  }
  if(req.method!=='POST'){res.writeHead(404);res.end();return;}
  const chunks=[];req.on('data',b=>chunks.push(b));req.on('end',()=>{
   const body=Buffer.concat(chunks);requests.push({route:req.url,body,type:req.headers['content-type'],range:req.headers.range,method:req.method});
   if(req.url==='/redirect.udmform'){res.writeHead(307,{Location:'/redirected.udmform','Content-Length':0});res.end();return;}
   res.writeHead(200,{'Content-Type':'application/octet-stream','Content-Disposition':'attachment; filename="'+req.url.slice(1)+'"','Content-Length':payload.length});
   let at=0;const timer=setInterval(()=>{const end=Math.min(at+32768,payload.length);res.write(payload.subarray(at,end));at=end;if(at===payload.length){clearInterval(timer);res.end();}},25);res.on('close',()=>clearInterval(timer));
  });
 });await new Promise(r=>server.listen(0,'127.0.0.1',r));origin='http://127.0.0.1:'+server.address().port;
 context=await chromium.launchPersistentContext(path.join(root,'profile'),{channel:'msedge',headless:true,ignoreDefaultArgs:['--disable-extensions'],env:{...process.env,UDM_INSTANCE_TAG:tag},args:['--enable-unsafe-extension-debugging']});
 const cdp=await context.browser().newBrowserCDPSession();await cdp.send('Browser.setDownloadBehavior',{behavior:'allow',downloadPath:path.join(root,'browser-downloads'),eventsEnabled:true});await cdp.send('Extensions.loadUnpacked',{path:ext});
 worker=context.serviceWorkers()[0]||await context.waitForEvent('serviceworker');await until(async()=>{try{return(await worker.evaluate(()=>nativeRequest({action:'ping'}))).ok;}catch{return false;}});
 const identity=await worker.evaluate(()=>nativeRequest({action:'diagnostics'}));assert.equal(identity.version,'0.78.0');assert.equal(path.resolve(identity.dataDirectory).toLowerCase(),path.dirname(stateFile).toLowerCase());pass('Native candidate uses only the isolated test catalog'); const policy=await worker.evaluate(()=>nativeRequest({action:'preferences'}));assert.equal(policy.captureTransaction,1);pass('Native candidate advertises prepared browser handoff');
 await worker.evaluate(async fault=>{
  const {settings}=await chrome.storage.local.get('settings');await chrome.storage.local.set({settings:{...settings,capture:true,extensions:['udmform'],cookies:false}});
  globalThis.__fileForms=[];let injected=false;const preparedUrls=new Map();const nativeSend=nativeClient.request.bind(nativeClient);nativeClient.request=async message=>{if(message.action==='capture-prepare')preparedUrls.set(message.captureToken,message.download.url);if(message.action==='capture-commit'){const items=await chrome.downloads.search({});__fileForms.push({event:'commit',url:preparedUrls.get(message.captureToken),browser:items.map(i=>({url:i.url,finalUrl:i.finalUrl,state:i.state,error:i.error}))});}const result=await nativeSend(message);if(!injected&&message.action===fault){injected=true;__fileForms.push({event:'lost-reply',action:fault});throw Error('Fixture: lost native acknowledgement');}return result;};const original=requestContext.captureForm;requestContext.captureForm=(message,sender)=>{const result=original(message,sender);__fileForms.push({event:'form',accepted:result,files:message.fields.filter(x=>typeof x[1]==='object').length,prior:!!message.priorFields});return result;};
  chrome.webRequest.onBeforeRequest.addListener(e=>{if(e.method==='POST')__fileForms.push({event:'request',url:e.url,body:e.requestBody?{keys:Object.keys(e.requestBody),raw:e.requestBody.raw?.map(p=>({file:!!p.file,bytes:p.bytes?.byteLength})),fields:e.requestBody.formData?Object.fromEntries(Object.entries(e.requestBody.formData).map(([key,values])=>[key,values.map(value=>({length:value.length,prefix:value.slice(0,80),codes:Array.from(value.slice(0,24),c=>c.codePointAt(0))}))])):undefined}:null});},{urls:['http://127.0.0.1/*']},['requestBody']);
 },process.env.UDM_TRANSACTION_FAULT||'');
 page=context.pages()[0]||await context.newPage();await page.goto(origin+'/');
 const downloads=route=>worker.evaluate(async url=>(await chrome.downloads.search({})).filter(item=>item.url===url||item.finalUrl===url),origin+'/'+route+'.udmform');
 for(const c of cases.filter(c=>!process.env.UDM_FILE_CASES||process.env.UDM_FILE_CASES.split(',').includes(c.id))){
  if(c.file){
   const values=[{name:c.filename||'binary.bin',mimeType:'application/octet-stream',buffer:c.large?Buffer.alloc(1048577,99):c.near?Buffer.from(Array.from({length:983040},(_,i)=>i%256)):upload}];
   if(c.multiple)values.push({name:'second-é.bin',mimeType:'application/octet-stream',buffer:Buffer.from([8,9,0,255])});
   if(c.disk){const file=path.join(root,'on-disk.bin');fs.writeFileSync(file,upload);await page.locator('#input-'+c.id).setInputFiles(file);}
   else await page.locator('#input-'+c.id).setInputFiles(c.multiple?values:values[0]);
  }
  const route=c.route||c.id,start=performance.now();console.log('SUBMIT '+c.id);await page.locator('#'+c.id).click();
  if(!baseline&&!c.keep){
   const job=await until(()=>{const j=jobs().find(j=>j.Url===origin+'/'+route+'.udmform');if(j?.Status==='Failed')throw Error(j.Error);return j?.Status==='Complete'&&j;});
   assert.equal(hash(fs.readFileSync(path.join(job.Folder,job.FileName))),hash(payload));assert(job.ProtectedRequest);assert.equal(job.RangeSupported,false);
   const seen=requests.filter(r=>r.route==='/'+route+'.udmform');assert.equal(seen.length,2);assert.deepEqual(seen[1].body,seen[0].body);assert.equal(seen[1].type,seen[0].type);assert(seen.every(r=>!r.range&&r.method==='POST'));
   if(c.file)assert(seen[0].body.includes(upload));if(c.id==='empty')assert(seen[0].body.includes(Buffer.from('filename=""')));
   if(c.id==='blob')assert(seen[0].body.includes(Buffer.from([0,128,255,13,10,1])));
   if(c.id==='quoted')assert(seen[0].body.includes(Buffer.from('filename="quote%22%0Dfile.bin"')));
   if(c.id==='textescapes')assert(seen[0].body.includes(Buffer.from('name="quote%22%0D%0Aname"\r\n\r\none\r\ntwo')));
   if(c.id==='redirect'){const first=requests.filter(r=>r.route==='/redirect.udmform');assert.equal(first.length,1);assert.deepEqual(first[0].body,seen[0].body);}
   pass(c.id+' transfers every original request byte and publishes matching output',{requestBytes:seen[0].body.length,requestSha256:hash(seen[0].body),elapsedMs:Math.round(performance.now()-start)});
   await until(async()=>{const [item]=await downloads(route);return item?.state==='interrupted'&&item.error==='USER_CANCELED';});pass(c.id+' has one native output and a canceled Edge response');
  }else{
   const item=await until(async()=>{const [item]=await downloads(route);return item?.state==='complete'&&item;});
   assert(!jobs().some(j=>j.Url===origin+'/'+route+'.udmform'));assert.equal(requests.filter(r=>r.route==='/'+route+'.udmform').length,1);assert.equal(hash(fs.readFileSync(item.filename)),hash(payload));
   pass((baseline?'Installed baseline: ':'')+c.id+' completes in Edge with no native replay');
  }
 }
 if(process.env.UDM_TRANSACTION_FAULT){assert.equal(await worker.evaluate(()=>__fileForms.filter(e=>e.event==='lost-reply').length),1);pass('Injected lost native acknowledgement recovers without a duplicate output');}
 const commits=await worker.evaluate(()=>__fileForms.filter(e=>e.event==='commit'));assert(commits.length>0);for(const entry of commits)assert(entry.browser.some(i=>(i.url===entry.url||i.finalUrl===entry.url)&&i.state==='interrupted'&&i.error==='USER_CANCELED'));pass('Actual Edge cancellation is observed before each native commit');
 const storage=JSON.stringify(await worker.evaluate(async()=>({local:await chrome.storage.local.get(null),session:await chrome.storage.session.get(null)}))),state=fs.readFileSync(stateFile,'utf8');
 assert(!storage.includes(upload.toString('base64')));assert(!state.includes(upload.toString('base64')));
 for(const request of requests){assert(!storage.includes(request.body.toString('base64')));assert(!state.includes(request.body.toString('base64')));}pass('File payloads do not enter extension storage or plaintext native history');
 const connection=await worker.evaluate(()=>nativeClient.diagnostics());assert.equal(connection.connections,1);assert.equal(connection.persistent,1);pass('All transfers share one persistent native connection');
})().catch(error=>{checks.push({passed:false,error:error.stack});console.error(error);process.exitCode=1;}).finally(async()=>{
 if(worker)try{diagnostics.push(await worker.evaluate(async()=>({trace:globalThis.__fileForms,lastError:(await chrome.storage.local.get('lastError')).lastError,context:requestContext.diagnostics(),downloads:(await chrome.downloads.search({})).map(i=>({url:i.url,filename:i.filename,state:i.state,error:i.error}))})));}catch{}
 fs.mkdirSync(root,{recursive:true});fs.writeFileSync(path.join(root,'diagnostics.json'),JSON.stringify(diagnostics,null,2));fs.writeFileSync(path.join(root,'requests.json'),JSON.stringify(requests.map(r=>({route:r.route,bytes:r.body.length,sha256:hash(r.body),type:r.type,range:r.range})),null,2));
 fs.writeFileSync(path.join(root,'jobs.json'),JSON.stringify(jobs().map(j=>({Status:j.Status,Error:j.Error,Url:j.Url,FileName:j.FileName})),null,2));
 if(context)await context.close();if(desktop)desktop.kill();if(server){server.closeAllConnections();await new Promise(r=>server.close(r));}
 if(registered)execFileSync('reg.exe',['delete',registry,'/f'],{windowsHide:true,stdio:'ignore'});
 let remains=false;try{execFileSync('reg.exe',['query',registry],{windowsHide:true,stdio:'ignore'});remains=true;}catch{}
 fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({baseline,checks,requestCount:requests.length,registryRemoved:!remains},null,2));
});
