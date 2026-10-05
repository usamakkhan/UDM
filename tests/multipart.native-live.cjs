'use strict';
const {chromium}=require('playwright'),http=require('node:http'),fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),assert=require('node:assert/strict'),{spawn,execFileSync}=require('node:child_process');
const project=process.env.UDM_PROJECT||path.resolve(__dirname,'..'),root=path.resolve(process.argv[2]),tag='multipart'+Date.now(),checks=[],requests=[];
const hostName='com.udm.multipartfixture'+Date.now(),nativeKeys=[],stateFile=path.join(root,'state/state.json');
const expected=process.env.UDM_EXPECT_MULTIPART!=='0',release=process.env.UDM_TEST_RELEASE||'D:\\UDM\\release';
let context,worker,desktop,server,page,origin;
const wait=ms=>new Promise(r=>setTimeout(r,ms)),hash=b=>crypto.createHash('sha256').update(b).digest('hex');
function jobs(){try{return JSON.parse(fs.readFileSync(stateFile)).Downloads;}catch{return [];}}
async function until(fn,timeout=25000){const end=Date.now()+timeout;while(Date.now()<end){const r=await fn();if(r)return r;await wait(100);}throw Error('Multipart fixture timeout');}
const pass=(name,details={})=>{checks.push({passed:true,name,...details});console.log('PASS '+name);};
const payload=Buffer.alloc(4*1024*1024+31);for(let i=0;i<payload.length;i++)payload[i]=(i*19+5)%251;
(async()=>{
 assert(!fs.existsSync(root),'Use a fresh fixture folder');fs.mkdirSync(path.dirname(stateFile),{recursive:true});fs.mkdirSync(path.join(root,'browser-downloads'));
 const ext=path.join(root,'extension');fs.cpSync(path.join(project,'browser/chromium'),ext,{recursive:true});
 const manifest=JSON.parse(fs.readFileSync(path.join(ext,'manifest.json')));
 const extensionId=crypto.createHash('sha256').update(Buffer.from(manifest.key,'base64')).digest('hex').slice(0,32).replace(/[0-9a-f]/g,c=>String.fromCharCode(97+parseInt(c,16)));
 const hostManifest=path.join(root,'native-host.json');fs.writeFileSync(hostManifest,JSON.stringify({name:hostName,description:'Isolated UDM multipart fixture',path:path.join(release,'Udm.NativeHost.exe'),type:'stdio',allowed_origins:['chrome-extension://'+extensionId+'/']}));
 const background=path.join(ext,'background.js');fs.writeFileSync(background,fs.readFileSync(background,'utf8').replace("'com.udm.download_manager'",JSON.stringify(hostName)));
 const key='HKCU\\Software\\Microsoft\\Edge\\NativeMessagingHosts\\'+hostName;
 let exists=false;try{execFileSync('reg.exe',['query',key],{windowsHide:true,stdio:'ignore'});exists=true;}catch{}assert(!exists);
 execFileSync('reg.exe',['add',key,'/ve','/t','REG_SZ','/d',hostManifest,'/f'],{windowsHide:true,stdio:'ignore'});nativeKeys.push(key);
 fs.writeFileSync(stateFile,JSON.stringify({Schema:1,Settings:{DownloadFolder:path.join(root,'downloads'),CategoryFolders:false,ProxyMode:'Connect directly',SkipBrowserFileInfo:true,PrefetchFileInfo:true,SuppressProgressDialog:true,SuppressCompletionDialog:true,Sound:false,CaptureExtensions:'udmform',DuplicatePolicy:'Numbered',Connections:8},Queues:[{Name:'Main queue',Enabled:true,Parallel:2}],Downloads:[],Projects:[]}));
 desktop=spawn(path.join(release,'UDM.exe'),['--background','--data-dir',path.dirname(stateFile),'--instance-tag',tag],{windowsHide:true,stdio:'ignore'});
 const form=(id,fields,attrs='')=>'<form id="form-'+id+'" method="post" enctype="multipart/form-data" action="/'+id+'.udmform" '+attrs+'>'+fields+'<button id="'+id+'">Submit '+id+'</button></form>';
 server=http.createServer((req,res)=>{
  if(req.method==='GET'&&req.url==='/'){
   res.setHeader('Content-Type','text/html; charset=utf-8');
   res.end('<!doctype html><meta charset="utf-8"><title>UDM multipart acceptance</title>'+
    form('ordered','<input name="z" value="first"><input name="a" value="middle"><input name="z" value="last"><input name="unicode" value="café ✓">')+
    form('escaped','<input name="quote&quot;&#13;&#10;field" value="line"><textarea name="lines">one\ntwo\nthree</textarea>')+
    form('percent','<input name="literal+%22%C3%A9%25" value="plus+percent%22"><input name="raw%literal" value="unchanged">')+
    form('modified','<input name="original" value="yes">')+
    form('override','<input name="report" value="override"><button id="actual-override" formaction="/overridden.udmform" name="choice" value="chosen">Override</button>')+
    form('redirect','<input name="redirect" value="same body">')+
    form('quoted','<textarea name="quotes">'+('&#34;'.repeat(350000))+'</textarea>')+
    form('large','<textarea name="bulk">'+('x'.repeat(200000))+'</textarea>')+
    form('files','<input name="label" value="keep"><input name="file" type="file" id="file">')+
    form('oversized','<textarea name="huge">'+('x'.repeat(1048576))+'</textarea>')+
    '<script>document.querySelector("#form-modified").addEventListener("formdata",e=>{e.formData.delete("original");e.formData.append("z","one");e.formData.append("a","middle");e.formData.append("z","two");});</script>');return;
  }
  if(req.method!=='POST'){res.writeHead(404);res.end();return;}
  const chunks=[];req.on('data',b=>chunks.push(b));req.on('end',()=>{const body=Buffer.concat(chunks);requests.push({route:req.url,body,type:req.headers['content-type'],range:req.headers.range});
   if(req.url==='/redirect.udmform'){res.writeHead(307,{Location:'/redirected.udmform','Content-Length':0});res.end();return;}
   res.writeHead(200,{'Content-Type':'application/octet-stream','Content-Disposition':'attachment; filename="'+req.url.slice(1)+'"','Content-Length':payload.length});
   let at=0;const timer=setInterval(()=>{const end=Math.min(at+32768,payload.length);res.write(payload.subarray(at,end));at=end;if(at===payload.length){clearInterval(timer);res.end();}},25);res.on('close',()=>clearInterval(timer));
  });
 });await new Promise(r=>server.listen(0,'127.0.0.1',r));origin='http://127.0.0.1:'+server.address().port;
 context=await chromium.launchPersistentContext(path.join(root,'profile'),{channel:'msedge',headless:true,ignoreDefaultArgs:['--disable-extensions'],env:{...process.env,UDM_INSTANCE_TAG:tag},args:['--enable-unsafe-extension-debugging']});
 const cdp=await context.browser().newBrowserCDPSession();await cdp.send('Browser.setDownloadBehavior',{behavior:'allow',downloadPath:path.join(root,'browser-downloads'),eventsEnabled:true});await cdp.send('Extensions.loadUnpacked',{path:ext});
 worker=context.serviceWorkers()[0]||await context.waitForEvent('serviceworker');
 await until(async()=>{try{return(await worker.evaluate(()=>nativeRequest({action:'ping'}))).ok;}catch{return false;}});
 const identity=await worker.evaluate(()=>nativeRequest({action:'diagnostics'}));assert.equal(path.resolve(identity.dataDirectory).toLowerCase(),path.dirname(stateFile).toLowerCase());assert.equal(identity.version,'0.69.0');
 await worker.evaluate(async()=>{const {settings}=await chrome.storage.local.get('settings');await chrome.storage.local.set({settings:{...settings,capture:true,extensions:['udmform'],cookies:false}});});
 await worker.evaluate(()=>{globalThis.__multipartTrace=[];const original=requestContext.captureForm;requestContext.captureForm=(message,sender)=>{const result=original(message,sender);__multipartTrace.push({phase:'form',time:Date.now(),accepted:result,fields:message.fields?.map(([k,v])=>[k,v.slice(0,80)])});return result;};chrome.webRequest.onBeforeRequest.addListener(e=>{if(e.method==='POST')__multipartTrace.push({phase:'observed',url:e.url,fields:e.requestBody?.formData?Object.fromEntries(Object.entries(e.requestBody.formData).map(([k,v])=>[k,v.map(x=>x.slice(0,80))])):null});},{urls:['http://127.0.0.1/*']},['requestBody']);const resolve=requestContext.resolveDownload;requestContext.resolveDownload=async(...args)=>{const start=Date.now(),value=await resolve(...args);__multipartTrace.push({phase:'resolve',ms:Date.now()-start,captured:!!value?.request,pending:value?.pending});return value;};});
 page=context.pages()[0]||await context.newPage();await page.goto(origin+'/');
 const downloads=route=>worker.evaluate(async url=>(await chrome.downloads.search({})).filter(item=>item.url===url||item.finalUrl===url),origin+route);
 for(const [button,route] of [['ordered','ordered'],['escaped','escaped'],['percent','percent'],['modified','modified'],['actual-override','overridden'],['redirect','redirected'],['quoted','quoted'],['large','large']]){
  console.log('SUBMIT '+route);await page.locator('#'+button).click();
  if(expected){
   const job=await until(()=>{const j=jobs().find(j=>j.Url===origin+'/'+route+'.udmform');if(j?.Status==='Failed')throw Error(j.Error);return j?.Status==='Complete'&&j;});
   assert.equal(hash(fs.readFileSync(path.join(job.Folder,job.FileName))),hash(payload));assert(job.ProtectedRequest);assert.equal(job.RangeSupported,false);
   const seen=requests.filter(r=>r.route==='/'+route+'.udmform');assert.equal(seen.length,2);assert.deepEqual(seen[1].body,seen[0].body);assert.equal(seen[1].type,seen[0].type);assert(seen.every(r=>!r.range));
   if(route==='redirected'){const original=requests.filter(r=>r.route==='/redirect.udmform');assert.equal(original.length,1);assert.deepEqual(original[0].body,seen[0].body);}
   pass(route+' multipart preserves every browser request byte and publishes identical output',{requestBytes:seen[0].body.length,sha256:hash(seen[0].body)});
   await until(async()=>{const [item]=await downloads('/'+route+'.udmform');return item?.state==='interrupted'&&item.error==='USER_CANCELED';});pass(route+' browser response canceled only after native ownership');
  }else{
   await until(async()=>{const [item]=await downloads('/'+route+'.udmform');return item?.state==='complete';});assert.equal(jobs().length,0);assert.equal(requests.filter(r=>r.route==='/'+route+'.udmform').length,1);pass('Baseline leaves '+route+' multipart response in Edge');
  }
 }
 for(const route of ['files','oversized']){
  if(route==='files')await page.locator('#file').setInputFiles({name:'fixture.txt',mimeType:'text/plain',buffer:Buffer.from('known upload fixture')});
  console.log('SUBMIT '+route);await page.locator('#'+route).click();await until(async()=>{const [item]=await downloads('/'+route+'.udmform');return item?.state==='complete';});
  assert(!jobs().some(j=>j.Url===origin+'/'+route+'.udmform'));assert.equal(requests.filter(r=>r.route==='/'+route+'.udmform').length,1);assert.equal(hash(fs.readFileSync(path.join(root,'browser-downloads',route+'.udmform'))),hash(payload));pass(route+' completes in Edge without an incomplete native replay');
 }
 const storage=JSON.stringify(await worker.evaluate(async()=>({local:await chrome.storage.local.get(null),session:await chrome.storage.session.get(null)}))),state=fs.readFileSync(stateFile,'utf8');
 for(const request of requests){assert(!storage.includes(request.body.toString('base64')));assert(!state.includes(request.body.toString('base64')));}
 pass('Multipart body bytes never enter extension storage or plaintext native history');
 const connection=await worker.evaluate(()=>nativeClient.diagnostics());assert.equal(connection.connections,1);assert.equal(connection.persistent,1);pass('All form captures use one persistent native connection');
})().catch(async e=>{checks.push({passed:false,error:e.stack});console.error(e);if(worker)try{const diagnostics=await worker.evaluate(async()=>({trace:globalThis.__multipartTrace,context:requestContext.diagnostics(),lastError:(await chrome.storage.local.get('lastError')).lastError,downloads:(await chrome.downloads.search({})).map(x=>({url:x.url,state:x.state,error:x.error}))}));fs.writeFileSync(path.join(root,'failure.json'),JSON.stringify({diagnostics,jobs:jobs().map(j=>({Status:j.Status,Error:j.Error,Url:j.Url})),requests:requests.map(r=>({route:r.route,bytes:r.body.length,body:r.body.length<1000?r.body.toString():undefined}))},null,2));}catch{}process.exitCode=1;}).finally(async()=>{
 if(context)await context.close();if(desktop)desktop.kill();if(server){server.closeAllConnections();await new Promise(r=>server.close(r));}
 for(const key of nativeKeys)try{execFileSync('reg.exe',['delete',key,'/f'],{windowsHide:true,stdio:'ignore'});}catch{process.exitCode=1;}
 fs.mkdirSync(root,{recursive:true});fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({checks,requestCount:requests.length,expectedSupport:expected,registryRemoved:!nativeKeys.some(key=>{try{execFileSync('reg.exe',['query',key],{windowsHide:true,stdio:'ignore'});return true;}catch{return false;}})},null,2));
});
