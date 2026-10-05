'use strict';
// Real Firefox/Win32 transfers. Fault modes alter delivery timing or send one
// invalid prepare request; browser and native responses are never fabricated.
const http=require('node:http'),net=require('node:net'),fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),assert=require('node:assert/strict'),{spawn,execFileSync}=require('node:child_process');
const root=path.resolve(process.argv[2]),project=process.env.UDM_PROJECT||path.resolve(__dirname,'..'),release=process.env.UDM_TEST_RELEASE||'D:/UDM/release';
const mode=process.env.UDM_HANDOFF_MODE||'accept',baseline=process.env.UDM_HANDOFF_BASELINE==='1';
assert(['accept','denied','unavailable'].includes(mode));
const tag='fxrunning'+Date.now(),hostName='com.udm.runningfixture'+Date.now(),registry='HKCU\\Software\\Mozilla\\NativeMessagingHosts\\'+hostName;
const stateFile=path.join(root,'state/state.json'),checks=[],requests=[],diagnostics=[],previousEvents=[],payload=Buffer.alloc(2*1024*1024+19,71);
let server,desktop,origin,driver,session,base,browserVersion,observerReady=false,registered=false,pending=null,commandSerial=0;const answers=new Map();
const hash=b=>crypto.createHash('sha256').update(b).digest('hex'),wait=ms=>new Promise(r=>setTimeout(r,ms));
const jobs=()=>{try{return JSON.parse(fs.readFileSync(stateFile)).Downloads;}catch{return [];}};
async function until(fn){const end=Date.now()+35000;while(Date.now()<end){const result=await fn();if(result)return result;await wait(80);}throw Error('Automatic handoff fixture timeout');}
const pass=(name,detail={})=>{checks.push({passed:true,name,...detail});console.log('PASS '+name);};
async function request(method,route,data){const response=await fetch(base+route,{method,headers:{'Content-Type':'application/json'},body:data?JSON.stringify(data):undefined,signal:AbortSignal.timeout(45000)});const json=await response.json();if(json.value?.error)throw Error(json.value.error+': '+json.value.message);return json.value;}
const cmd=(route,data)=>request('POST','/session/'+session+route,data);
async function bg(message){assert(!pending);const id=++commandSerial;pending={id,message};const response=await until(()=>answers.get(id));answers.delete(id);if(response.error)throw Error(response.error);return response.result;}
async function click(selector){const value=await cmd('/element',{using:'css selector',value:selector});await cmd('/element/'+value['element-6066-11e4-a52e-4f735466cecf']+'/click',{});}
async function freePort(){const socket=net.createServer();await new Promise(r=>socket.listen(0,'127.0.0.1',r));const port=socket.address().port;await new Promise(r=>socket.close(r));return port;}
function fixtureObserver(origin){
 const events=[],urls=new Map(),generation=crypto.randomUUID();let busy=false,scenario='',faulted=false;
 const snapshot=async()=> (await api.downloads.search({})).map(i=>({id:i.id,url:i.url,state:i.state,error:i.error,paused:i.paused,canResume:i.canResume,bytesReceived:i.bytesReceived,totalBytes:i.totalBytes}));
 for(const action of ['pause','resume','cancel']){const original=api.downloads[action].bind(api.downloads);api.downloads[action]=async id=>{events.push({event:action,id,before:await snapshot()});try{return await original(id);}finally{events.push({event:action+'-after',id,after:await snapshot()});}};}
 const send=nativeClient.request.bind(nativeClient);
 nativeClient.request=async message=>{
  const action=message.action;events.push({event:'native-start',action,scenario});
  if(action==='capture-prepare'){
   urls.set(message.captureToken,message.download.url);
   if(scenario==='reject')message={...message,download:{...message.download,url:'file:///invalid-prepare-fixture'}};
   if(scenario==='complete')await new Promise(r=>setTimeout(r,2600));
  }
  if(action==='capture-commit')events.push({event:'commit',url:urls.get(message.captureToken),browser:await snapshot()});
  try{
   const result=await send(message);events.push({event:'native-result',action,ok:result?.ok,status:result?.status,scenario,error:result?.error});
   if((scenario==='restart-prepare'&&action==='capture-prepare')||(scenario==='restart-commit'&&action==='capture-commit')){events.push({event:'restart-checkpoint',action,scenario});await new Promise(()=>{});}
   if(!faulted&&((scenario==='lost-prepare'&&action==='capture-prepare')||(scenario==='lost-commit'&&action==='capture-commit'))){faulted=true;events.push({event:'dropped-reply',action});throw Error('Fixture dropped one real native acknowledgement');}
   return result;
  }catch(error){events.push({event:'native-error',action,error:error.message});throw error;}
 };
 setInterval(async()=>{
  if(busy)return;busy=true;let command;
  try{
   command=await(await fetch(origin+'/command')).json();if(!command)return;
   const m=command.message;let result;
   if(m.action==='configure'){const {settings}=await api.storage.local.get('settings');await api.storage.local.set({settings:{...settings,capture:true,extensions:['udmform'],cookies:false}});result={ok:true};}
   else if(m.action==='scenario'){scenario=m.value;faulted=false;result={ok:true};}
   else if(m.action==='debug')result={generation,events,downloads:await api.downloads.search({}),local:await api.storage.local.get(null)};
   else if(m.action==='reload'){result={ok:true,generation};setTimeout(()=>api.runtime.reload(),100);}
   else if(m.action==='recover')result=await captureRecovery.recover();
   else if(m.action==='diagnostics'||m.action==='preferences')result=await nativeRequest(m);
   else throw Error('Unknown fixture operation');
   await fetch(origin+'/answer',{method:'POST',body:JSON.stringify({id:command.id,result})});
  }catch(error){if(command)await fetch(origin+'/answer',{method:'POST',body:JSON.stringify({id:command.id,error:error.message})}).catch(()=>{});}
  finally{busy=false;}
 },75);
}
const cases=(mode==='accept'?[{id:'get',method:'GET'},{id:'post',method:'POST'},{id:'reject-get',method:'GET',scenario:'reject',browser:true},{id:'reject-post',method:'POST',scenario:'reject',browser:true},{id:'lost-prepare',method:'POST',scenario:'lost-prepare'},{id:'lost-commit',method:'POST',scenario:'lost-commit'},{id:'complete',method:'POST',scenario:'complete',browser:true},{id:'restart-prepare',method:'POST',scenario:'restart-prepare',browser:true},{id:'restart-commit',method:'POST',scenario:'restart-commit'}]:[{id:'get',method:'GET',browser:true},{id:'post',method:'POST',browser:true}]).filter(c=>!process.env.UDM_HANDOFF_CASES||process.env.UDM_HANDOFF_CASES.split(',').includes(c.id));
(async()=>{
 assert(!fs.existsSync(root),'Use a fresh isolated output directory');fs.mkdirSync(path.dirname(stateFile),{recursive:true});fs.mkdirSync(path.join(root,'browser-downloads'));
 const ext=path.join(root,'extension');fs.cpSync(path.join(project,'browser/firefox'),ext,{recursive:true});
 const manifest=JSON.parse(fs.readFileSync(path.join(ext,'manifest.json')));assert.equal(manifest.version,baseline?'0.52.2':'0.53.1');
 const hostFile=path.join(root,'native-host.json');fs.writeFileSync(hostFile,JSON.stringify({name:hostName,description:'Isolated UDM automatic Firefox handoff',path:mode==='unavailable'?path.join(root,'missing-host.exe'):path.join(release,'Udm.NativeHost.exe'),type:'stdio',allowed_extensions:['udm@local.example']}));
 let exists=false;try{execFileSync('reg.exe',['query',registry],{windowsHide:true,stdio:'ignore'});exists=true;}catch{}assert(!exists);
 execFileSync('reg.exe',['add',registry,'/ve','/t','REG_SZ','/d',hostFile,'/f'],{windowsHide:true,stdio:'ignore'});registered=true;
 fs.writeFileSync(stateFile,JSON.stringify({Schema:1,Settings:{DownloadFolder:path.join(root,'downloads'),CategoryFolders:false,ProxyMode:'Connect directly',BrowserCaptureEnabled:mode!=='denied',SkipBrowserFileInfo:true,PrefetchFileInfo:false,SuppressProgressDialog:true,SuppressCompletionDialog:true,Sound:false,CaptureExtensions:'udmform',DuplicatePolicy:'Numbered',Connections:1},Queues:[{Name:'Main queue',Enabled:true,Parallel:1}],Downloads:[],Projects:[]}));
 if(mode!=='unavailable')desktop=spawn(path.join(release,'UDM.exe'),['--background','--data-dir',path.dirname(stateFile),'--instance-tag',tag],{windowsHide:true,stdio:'ignore'});
 server=http.createServer((req,res)=>{
  if(req.url==='/command'){res.setHeader('Content-Type','application/json');res.end(JSON.stringify(pending));pending=null;return;}
  if(req.url==='/answer'){let value='';req.on('data',chunk=>value+=chunk);req.on('end',()=>{const answer=JSON.parse(value);answers.set(answer.id,answer);res.end('{}');});return;}
  if(req.url==='/'){res.setHeader('Content-Type','text/html; charset=utf-8');res.end('<!doctype html><meta charset="utf-8"><title>UDM automatic handoff acceptance</title>'+cases.map(c=>c.method==='GET'?'<a id="'+c.id+'" href="/'+c.id+'.udmform">'+c.id+'</a>':'<form action="/'+c.id+'.udmform" method="post"><input name="label" value="original response ✓"><input name="label" value="one+two"><button id="'+c.id+'">'+c.id+'</button></form>').join('<hr>'));return;}
  if(!/^\/[\w-]+\.udmform$/.test(req.url)){res.writeHead(404);res.end();return;}
  const chunks=[];req.on('data',b=>chunks.push(b));req.on('end',()=>{
   const body=Buffer.concat(chunks);requests.push({route:req.url,body,type:req.headers['content-type'],range:req.headers.range,method:req.method});
   // A declined or unavailable handoff must not spend a one-use response twice.
   if(mode!=='accept'&&requests.filter(r=>r.route===req.url).length>1){res.writeHead(410,{'Content-Length':0});res.end();return;}
   res.writeHead(200,{'Content-Type':'application/octet-stream','Content-Disposition':'attachment; filename="'+req.url.slice(1)+'"','Content-Length':payload.length,'Accept-Ranges':'none'});res.flushHeaders();
   if(req.method==='HEAD'){res.end();return;}
   let at=0,timer;const first=setTimeout(()=>{timer=setInterval(()=>{const end=Math.min(at+65536,payload.length);res.write(payload.subarray(at,end));at=end;if(at===payload.length){clearInterval(timer);res.end();}},30);},750);
   res.on('close',()=>{clearTimeout(first);clearInterval(timer);});
  });
 });await new Promise(r=>server.listen(0,'127.0.0.1',r));origin='http://127.0.0.1:'+server.address().port;
 const bgFile=path.join(ext,'background.js');fs.writeFileSync(bgFile,fs.readFileSync(bgFile,'utf8').replace("'com.udm.download_manager'",JSON.stringify(hostName))+'\n('+fixtureObserver.toString()+')('+JSON.stringify(origin)+');\n');
 const profile=path.join(root,'profile');fs.mkdirSync(profile);const port=await freePort();base='http://127.0.0.1:'+port;
 driver=spawn(process.env.UDM_GECKODRIVER,['--port',String(port),'--profile-root',root],{windowsHide:true,env:{...process.env,UDM_INSTANCE_TAG:tag},stdio:['ignore','pipe','pipe']});const log=fs.createWriteStream(path.join(root,'driver.log'));driver.stdout.pipe(log);driver.stderr.pipe(log);
 await until(async()=>{try{return await request('GET','/status');}catch{return false;}});
 const created=await request('POST','/session',{capabilities:{alwaysMatch:{'moz:firefoxOptions':{binary:process.env.UDM_FIREFOX,args:['-headless','-profile',profile],prefs:{'browser.shell.checkDefaultBrowser':false,'extensions.webextensions.warnings-as-errors':false,'browser.download.folderList':2,'browser.download.dir':path.join(root,'browser-downloads'),'browser.download.useDownloadDir':true,'browser.helperApps.neverAsk.saveToDisk':'application/octet-stream','browser.download.alwaysOpenPanel':false}},acceptInsecureCerts:false}}});
 session=created.sessionId;browserVersion=created.capabilities.browserVersion;assert.equal(await cmd('/moz/addon/install',{path:ext,temporary:true}),'udm@local.example');
 await bg({action:'configure'});observerReady=true;
 if(mode!=='unavailable'){const identity=await bg({action:'diagnostics'});assert.equal(identity.version,'0.78.0');assert.equal(path.resolve(identity.dataDirectory).toLowerCase(),path.dirname(stateFile).toLowerCase());assert.equal((await bg({action:'preferences'})).captureAllowed,mode!=='denied');pass('Real native settings and private catalog verified');}
 else{await assert.rejects(bg({action:'preferences'}));pass('Firefox reports the genuinely missing native executable');}
 await cmd('/url',{url:origin+'/'});
 for(const c of cases){
  await bg({action:'scenario',value:c.scenario||''});const url=origin+'/'+c.id+'.udmform',start=performance.now();console.log('SUBMIT '+mode+' '+c.id);await click('#'+c.id);
  if(c.scenario?.startsWith('restart-')){
   const before=await until(async()=>{const d=await bg({action:'debug'});return d.events.some(e=>e.event==='restart-checkpoint'&&e.scenario===c.scenario)&&d;});
   const records=Object.values(before.local.captureOwnershipV1||{});assert.equal(records.length,1);assert.equal(records[0].browserRunning,true);assert.equal(records[0].phase,c.browser?'preparing':'committing');
   if(c.browser)await until(async()=>(await bg({action:'debug'})).downloads.some(i=>i.url===url&&i.state==='complete'));
   diagnostics.push(before);previousEvents.push(...before.events);await bg({action:'reload'});
   await until(async()=>(await bg({action:'debug'})).generation!==before.generation);
   pass(c.id+': actual extension background reload crossed the durable boundary');
  }
  if(c.browser){
   const item=await until(async()=>{const detail=await bg({action:'debug'}),item=detail.downloads.find(i=>i.url===url);if(item?.state==='interrupted'&&!item.canResume){diagnostics.push(detail);throw Error('Original Firefox response cannot resume: '+JSON.stringify({state:item.state,error:item.error,paused:item.paused,canResume:item.canResume,bytesReceived:item.bytesReceived}));}return item?.state==='complete'&&item;});
   assert.equal(hash(fs.readFileSync(item.filename)),hash(payload));assert(!jobs().some(j=>j.Url===url));assert.equal(requests.filter(r=>r.route==='/'+c.id+'.udmform').length,1);
   pass(c.id+': original browser response completes, exact bytes, one request, no native job',{elapsedMs:Math.round(performance.now()-start)});
  }else{
   const job=await until(()=>{const job=jobs().find(j=>j.Url===url);if(job?.Status==='Failed')throw Error(job.Error);return job?.Status==='Complete'&&job;});assert.equal(hash(fs.readFileSync(path.join(job.Folder,job.FileName))),hash(payload));assert.equal(jobs().filter(j=>j.Url===url).length,1);
   const seen=requests.filter(r=>r.route==='/'+c.id+'.udmform');if(c.method==='POST'){assert.equal(seen.length,2);assert.deepEqual(seen[1].body,seen[0].body);assert.equal(seen[1].type,seen[0].type);assert(seen.every(r=>r.method==='POST'&&!r.range));assert(job.ProtectedRequest);}
   const detail=await bg({action:'debug'}),item=detail.downloads.find(i=>i.url===url);assert.equal(item.state,'interrupted');assert.equal(item.error,'USER_CANCELED');const commits=[...previousEvents,...detail.events].filter(e=>e.event==='commit'&&e.url===url);assert.equal(commits.length,1);assert(commits[0].browser.some(i=>i.url===url&&i.state==='interrupted'&&i.error==='USER_CANCELED'));
   pass(c.id+': one native output, exact request and file bytes, browser cancellation before commit',{elapsedMs:Math.round(performance.now()-start)});
  }
  await until(async()=>!Object.keys((await bg({action:'debug'})).local.captureOwnershipV1||{}).length);
 }
 const detail=await bg({action:'debug'}),events=[...previousEvents,...detail.events];diagnostics.push(detail);assert(!events.some(e=>['pause','resume'].includes(e.event)));pass('No destructive Firefox pause or replaying resume was attempted');
 if(cases.some(c=>c.scenario==='lost-prepare')){assert.equal(events.filter(e=>e.event==='dropped-reply').length,cases.filter(c=>c.scenario?.startsWith('lost-')).length);pass('Deliberately lost real native replies recovered');}
 if(cases.some(c=>c.scenario==='reject')){assert(events.some(e=>e.event==='native-result'&&e.action==='capture-prepare'&&e.ok===false));pass('Actual native prepare rejection verified');}
 for(const req of requests)if(req.body.length)assert(!JSON.stringify(detail.local).includes(req.body.toString('base64')));pass('Recovery journal cleared without storing request bodies');
})().catch(error=>{checks.push({passed:false,error:error.stack});console.error(error);process.exitCode=1;}).finally(async()=>{
 if(observerReady)try{diagnostics.push(await bg({action:'debug'}));}catch(error){diagnostics.push({fixtureError:error.message});}
 fs.mkdirSync(root,{recursive:true});fs.writeFileSync(path.join(root,'diagnostics.json'),JSON.stringify(diagnostics,null,2));fs.writeFileSync(path.join(root,'requests.json'),JSON.stringify(requests.map(r=>({route:r.route,method:r.method,bytes:r.body.length,sha256:hash(r.body),type:r.type,range:r.range})),null,2));
 fs.writeFileSync(path.join(root,'jobs.json'),JSON.stringify(jobs().map(j=>({Status:j.Status,Error:j.Error,Url:j.Url,FileName:j.FileName})),null,2));
 if(session)try{await request('DELETE','/session/'+session);}catch{}if(driver)driver.kill();if(desktop)desktop.kill();if(server){server.closeAllConnections();await new Promise(r=>server.close(r));}
 if(registered)execFileSync('reg.exe',['delete',registry,'/f'],{windowsHide:true,stdio:'ignore'});
 let remains=false;try{execFileSync('reg.exe',['query',registry],{windowsHide:true,stdio:'ignore'});remains=true;}catch{}
 fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({baseline,mode,cases:cases.map(c=>c.id),browserVersion,checks,requestCount:requests.length,registry,registryRemoved:!remains},null,2));
});
