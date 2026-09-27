'use strict';
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict'),http=require('node:http'),net=require('node:net'),crypto=require('node:crypto'),{spawn}=require('node:child_process');
const project=process.env.UDM_PROJECT||path.resolve(__dirname,'..'),root=path.resolve(process.argv[2]),tag='fxpost'+crypto.randomBytes(6).toString('hex'),results=[],requests=[];
let driver,desktop,server,session,base,origin,pending=null,answer=null;
const state=path.join(root,'state/state.json'),pause=ms=>new Promise(r=>setTimeout(r,ms)),hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const payload=Buffer.alloc(4*1024*1024+31);for(let i=0;i<payload.length;i++)payload[i]=(i*19+5)%251;
async function until(fn,ms=45000){const deadline=Date.now()+ms;while(Date.now()<deadline){const r=await fn();if(r)return r;await pause(100);}throw Error('Firefox POST condition timed out');}
async function request(method,route,data){const r=await fetch(base+route,{method,headers:{'Content-Type':'application/json'},body:data?JSON.stringify(data):undefined,signal:AbortSignal.timeout(50000)}),j=await r.json();if(j.value?.error)throw Error(j.value.error+': '+j.value.message);return j.value;}
const cmd=(route,data)=>request('POST','/session/'+session+route,data);
const pass=(name,detail={})=>{results.push({passed:true,name,...detail});console.log('PASS '+name);};
function jobs(){try{return JSON.parse(fs.readFileSync(state)).Downloads;}catch{return [];}}
async function bg(message){answer=null;pending=message;const r=await until(()=>answer);if(r.error)throw Error(r.error);return r;}
async function click(selector){const e=await cmd('/element',{using:'css selector',value:selector});await cmd('/element/'+e['element-6066-11e4-a52e-4f735466cecf']+'/click',{});}
async function freePort(){const s=net.createServer();await new Promise(r=>s.listen(0,'127.0.0.1',r));const p=s.address().port;await new Promise(r=>s.close(r));return p;}
(async()=>{
 assert(!fs.existsSync(state),'Use a new fixture directory');fs.mkdirSync(path.dirname(state),{recursive:true});fs.mkdirSync(path.join(root,'browser-downloads'),{recursive:true});
 fs.writeFileSync(state,JSON.stringify({Schema:1,Settings:{DownloadFolder:path.join(root,'downloads'),CategoryFolders:false,ProxyMode:'Connect directly',SkipBrowserFileInfo:true,PrefetchFileInfo:true,SuppressProgressDialog:true,SuppressCompletionDialog:true,Sound:false,CaptureExtensions:'udmform',DuplicatePolicy:'Numbered',Connections:8},Queues:[{Name:'Main queue',Enabled:true,Parallel:2}],Downloads:[],Projects:[]}));
 server=http.createServer((req,res)=>{
  if(req.url==='/command'){res.setHeader('Content-Type','application/json');res.end(JSON.stringify(pending));pending=null;return;}
  if(req.url==='/answer'){let value='';req.on('data',c=>value+=c);req.on('end',()=>{answer=JSON.parse(value);res.end('{}');});return;}
  if(req.method==='GET'&&req.url==='/'){res.setHeader('Content-Type','text/html; charset=utf-8');res.end('<!doctype html><title>UDM Firefox form tests</title><form method="post" action="/normal.udmform"><input name="report" value="monthly"><input name="tag" value="one two"><input name="tag" value="✓"><button id="normal">Normal form</button></form><form method="post" action="/medium.udmform"><textarea name="medium">'+('x'.repeat(65530))+'</textarea><button id="medium">Medium form</button></form><form method="post" action="/large.udmform"><textarea name="large">'+('x'.repeat(1048570))+'</textarea><button id="large">Large form</button></form><form method="post" action="/oversized.udmform"><textarea name="large">'+('x'.repeat(1048571))+'</textarea><button id="oversized">Oversized form</button></form><form method="post" enctype="multipart/form-data" action="/upload.udmform"><input id="file" name="file" type="file"><button id="upload">Upload form</button></form>');return;}
  if(req.method!=='POST'){res.writeHead(404);res.end();return;}
  const chunks=[];req.on('data',c=>chunks.push(c));req.on('end',()=>{const body=Buffer.concat(chunks);fs.appendFileSync(path.join(root,'request-proof.jsonl'),JSON.stringify({path:req.url,bytes:body.length,sha256:hash(body)})+'\n');requests.push({path:req.url,method:req.method,body,type:req.headers['content-type'],range:req.headers.range});res.writeHead(200,{'Content-Type':'application/octet-stream','Content-Disposition':'attachment; filename="'+req.url.slice(1)+'"','Content-Length':payload.length});let at=0;const timer=setInterval(()=>{const end=Math.min(at+65536,payload.length);res.write(payload.subarray(at,end));at=end;if(at===payload.length){clearInterval(timer);res.end();}},50);res.on('close',()=>clearInterval(timer));});
 });await new Promise(r=>server.listen(0,'127.0.0.1',r));origin='http://127.0.0.1:'+server.address().port;
 const ext=path.join(root,'extension');fs.cpSync(path.join(project,'browser/firefox'),ext,{recursive:true});
 const observer=[
 'setInterval(async()=>{try{const m=await(await fetch('+JSON.stringify(origin+'/command')+')).json();if(!m)return;let result;',
 "if(m.action==='configure'){const {settings}=await api.storage.local.get('settings');await api.storage.local.set({settings:{...settings,capture:true,extensions:['udmform'],cookies:false}});result={ok:true};}",
 "else if(m.action==='connection')result=nativeClient.diagnostics();",
 "else if(m.action==='debug')result={context:requestContext.diagnostics(),events:globalThis.__udmPostEvents||[],downloads:await api.downloads.search({}),storage:await api.storage.local.get(null)};",
 "else if(m.action==='downloads')result=await api.downloads.search({url:m.url});",
 "else if(m.action==='storage')result={local:await api.storage.local.get(null),session:api.storage.session?await api.storage.session.get(null):{}};",
 'else result=await nativeRequest(m);await fetch('+JSON.stringify(origin+'/answer')+",{method:'POST',body:JSON.stringify(result)});",
 '}catch(e){await fetch('+JSON.stringify(origin+'/answer')+",{method:'POST',body:JSON.stringify({error:e.message})}).catch(()=>{});}},100);"
 ].join('\n');
 fs.appendFileSync(path.join(ext,'background.js'),'\n'+observer,'utf8');

 desktop=spawn(process.env.UDM_APP_EXE||path.join(project,'release/UDM.exe'),['--background','--data-dir',path.dirname(state),'--instance-tag',tag],{windowsHide:true,stdio:'ignore'});
 const profile=path.join(root,'profile');fs.mkdirSync(profile);const port=await freePort();base='http://127.0.0.1:'+port;
 driver=spawn(process.env.UDM_GECKODRIVER,['--port',String(port),'--profile-root',root],{windowsHide:true,env:{...process.env,UDM_INSTANCE_TAG:tag},stdio:['ignore','pipe','pipe']});const log=fs.createWriteStream(path.join(root,'driver.log'));driver.stdout.pipe(log);driver.stderr.pipe(log);
 await until(async()=>{try{return await request('GET','/status');}catch{return false;}});
 const created=await request('POST','/session',{capabilities:{alwaysMatch:{'moz:firefoxOptions':{binary:process.env.UDM_FIREFOX,args:['-headless','-profile',profile],prefs:{'browser.shell.checkDefaultBrowser':false,'extensions.webextensions.warnings-as-errors':false,'browser.download.folderList':2,'browser.download.dir':path.join(root,'browser-downloads'),'browser.download.useDownloadDir':true,'browser.helperApps.neverAsk.saveToDisk':'application/octet-stream','browser.download.alwaysOpenPanel':false}},'acceptInsecureCerts':false}}});session=created.sessionId;
 assert.equal(await cmd('/moz/addon/install',{path:ext,temporary:true}),'udm@local.example');
 const identity=await bg({action:'diagnostics'});assert.equal(identity.version,'0.40.0');assert.equal(path.resolve(identity.dataDirectory).toLowerCase(),path.dirname(state).toLowerCase());assert.equal((await bg({action:'preferences'})).postBodyLimit,1048576);await bg({action:'configure'});pass('Firefox 0.29.0 reaches the isolated desktop and reads its 1 MiB limit',{browser:created.capabilities.browserVersion});
 await cmd('/url',{url:origin+'/'});
 for(const kind of ['normal','medium']){
  await click('#'+kind);const job=await until(()=>{const j=jobs().find(x=>x.Url===origin+'/'+kind+'.udmform');if(j?.Status==='Failed')throw Error(j.Error);return j?.Status==='Complete'&&j;});
  assert.equal(hash(fs.readFileSync(path.join(job.Folder,job.FileName))),hash(payload));assert(job.ProtectedRequest);assert.equal(job.RangeSupported,false);pass(kind+' form produces an exact native download with encrypted request storage');
  const seen=requests.filter(r=>r.path==='/'+kind+'.udmform');assert.equal(seen.length,2);assert.equal(seen[1].body.length,seen[0].body.length);assert.equal(hash(seen[1].body),hash(seen[0].body));assert(seen.every(r=>r.method==='POST'&&!r.range));
  if(kind==='medium')assert.equal(seen[0].body.length,65537);else assert.deepEqual(new URLSearchParams(seen[1].body.toString()).getAll('tag'),['one two','✓']);
  pass(kind+' form preserves all request bytes and fields with one native POST');
  await until(async()=>{const rows=await bg({action:'downloads',url:origin+'/'+kind+'.udmform'});return rows[0]?.state==='interrupted'&&!rows[0]?.paused&&rows[0]?.error==='USER_CANCELED';});pass(kind+' browser transfer is canceled after native acknowledgement');
 }
 const uploadFile=path.join(root,'fixture-upload.txt');fs.writeFileSync(uploadFile,'known local fixture');const fileElement=await cmd('/element',{using:'css selector',value:'#file'});await cmd('/element/'+fileElement['element-6066-11e4-a52e-4f735466cecf']+'/value',{text:uploadFile});
 for(const kind of ['large','oversized','upload']){
  await click('#'+kind);await until(async()=>{const rows=await bg({action:'downloads',url:origin+'/'+kind+'.udmform'});return rows[0]?.state==='complete';});
  assert(!jobs().some(j=>j.Url===origin+'/'+kind+'.udmform'));assert.equal(hash(fs.readFileSync(path.join(root,'browser-downloads',kind+'.udmform'))),hash(payload));pass(kind+' form completes in Firefox without an incorrect native download');
 }
 const uploads=requests.filter(r=>r.path==='/upload.udmform');assert.equal(uploads.length,1);assert(uploads[0].body.includes(Buffer.from('known local fixture')));pass('Multipart file contents stay intact in the browser with no native replay');
 const storage=JSON.stringify(await bg({action:'storage'})),catalog=fs.readFileSync(state,'utf8');
 for(const r of requests.filter(r=>r.path==='/normal.udmform'||r.path==='/medium.udmform')){assert(!storage.includes(r.body.toString('base64')));assert(!catalog.includes(r.body.toString('base64')));}pass('Request bodies are absent from extension storage and plaintext native history');
 const connection=await bg({action:'connection'});assert.equal(connection.connections,1);assert.equal(connection.persistent,1);pass('Firefox keeps one persistent native connection across large form handoffs');
})().catch(async e=>{results.push({passed:false,error:e.stack});console.error(e);if(session)try{fs.writeFileSync(path.join(root,'failure-diagnostics.json'),JSON.stringify({browser:await bg({action:'debug'}),requests:requests.map(r=>({path:r.path,method:r.method,bytes:r.body.length}))},null,2));}catch{}process.exitCode=1;}).finally(async()=>{if(session)try{await request('DELETE','/session/'+session);}catch{}if(driver)driver.kill();if(desktop)desktop.kill();if(server){server.closeAllConnections();await new Promise(r=>server.close(r));}fs.mkdirSync(root,{recursive:true});fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({results,requestCount:requests.length},null,2));});

