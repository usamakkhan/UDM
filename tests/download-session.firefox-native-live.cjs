'use strict';
const fs=require('node:fs'),path=require('node:path'),http=require('node:http'),assert=require('node:assert/strict'),crypto=require('node:crypto'),{spawn,execFileSync}=require('node:child_process');
const project=path.resolve(__dirname,'..'),output=path.resolve(process.argv[2]),channel='firefox',tag='downloadsession'+crypto.randomBytes(5).toString('hex'),results=[],seen=[];
const app=process.env.UDM_APP_EXE,host=process.env.UDM_HOST_EXE,hostName='com.udm.sessionfixture'+Date.now(),registry='HKCU\\Software\\'+'Mozilla'+'\\NativeMessagingHosts\\'+hostName;
const data=path.join(output,'state'),catalog=path.join(data,'state.json'),payload=Buffer.alloc(4*1024*1024+71,83),small=Buffer.from('public attachment with correctly scoped cookies');
let server,context,desktop,registered=false,origin,worker,driver,session,base,pending=null,answer=null;const sockets=new Set();
const wait=ms=>new Promise(r=>setTimeout(r,ms)),hash=b=>crypto.createHash('sha256').update(b).digest('hex');
function pass(name){results.push({name,passed:true});console.log('PASS '+name);}
function jobs(){try{return JSON.parse(fs.readFileSync(catalog)).Downloads;}catch{return [];}}
async function until(fn,timeout=60000){const end=Date.now()+timeout;while(Date.now()<end){const result=await fn();if(result)return result;await wait(100);}throw Error('Download session fixture timed out');}
async function request(method,route,data){const r=await fetch(base+route,{method,headers:{'Content-Type':'application/json'},body:data?JSON.stringify(data):undefined,signal:AbortSignal.timeout(50000)}),j=await r.json();if(j.value?.error)throw Error(j.value.error+': '+j.value.message);return j.value;}
const cmd=(route,data)=>request('POST','/session/'+session+route,data);
async function bg(message){answer=null;pending=message;const result=await until(()=>answer);if(result.error)throw Error(result.error);return result;}
async function click(selector){const e=await cmd('/element',{using:'css selector',value:selector});await cmd('/element/'+e['element-6066-11e4-a52e-4f735466cecf']+'/click',{});}
async function script(code){return cmd('/execute/sync',{script:code,args:[]});}
async function freePort(){const listener=require('node:net').createServer();await new Promise(r=>listener.listen(0,'127.0.0.1',r));const port=listener.address().port;await new Promise(r=>listener.close(r));return port;}
async function serve(req,res){
 if(req.url==='/command'){res.setHeader('Content-Type','application/json');res.end(JSON.stringify(pending));pending=null;return;}
 if(req.url==='/answer'){let body='';for await(const chunk of req)body+=chunk;answer=JSON.parse(body);res.end('ok');return;}

 const route=new URL(req.url,origin).pathname,cookie=req.headers.cookie||'',range=/^bytes=(\d+)-(\d*)$/.exec(req.headers.range||'');seen.push({route,method:req.method,range:!!range,privateCookie:cookie.includes('session='),rotated:cookie.includes('session=native-rotated'),root:cookie.includes('root=fixture-root'),elsewhere:cookie.includes('elsewhere=')});
 if(route==='/login'){res.writeHead(200,{'Content-Type':'text/html','Set-Cookie':['session=browser-original; HttpOnly; Path=/private; SameSite=Lax','root=fixture-root; HttpOnly; Path=/; SameSite=Strict','elsewhere=unrelated; HttpOnly; Path=/elsewhere']});res.end('<!doctype html><title>UDM download session fixture</title><a href="/private/report.udmsession">Download signed-in report</a><a href="/private/redirect.udmsession">Download redirected report</a>');return;}
 if(route.startsWith('/private/')&&!cookie.includes('session=browser-original')&&!cookie.includes('session=native-rotated')){res.writeHead(401);res.end('session required');return;}
 if(route==='/private/redirect.udmsession'){res.writeHead(302,{Location:'/public/result.udmsession'});res.end();return;}
 if(route==='/public/result.udmsession'&&cookie.includes('session=')){res.writeHead(400);res.end('private-path cookie leaked');return;}
 const bytes=route==='/private/report.udmsession'?payload:route==='/public/result.udmsession'?small:null;if(!bytes){res.writeHead(404);res.end();return;}
 const native=req.method==='HEAD'||!!range,probe=req.method==='HEAD'||range&&range[1]==='0'&&range[2]==='0';
 if(route==='/private/report.udmsession'&&probe)res.setHeader('Set-Cookie','session=native-rotated; HttpOnly; Path=/private');
 if(route==='/private/report.udmsession'&&range&&!probe&&!cookie.includes('session=native-rotated')){res.writeHead(401);res.end('replacement session required');return;}
 const first=range?+range[1]:0,last=Math.min(range&&range[2]?+range[2]:bytes.length-1,bytes.length-1);
 res.writeHead(range?206:200,{'Content-Type':'application/octet-stream','Content-Disposition':'attachment; filename="'+route.split('/').pop()+'"','Content-Length':last-first+1,'Accept-Ranges':'bytes',ETag:'"session-fixture-'+bytes.length+'"',...(range?{'Content-Range':`bytes ${first}-${last}/${bytes.length}`}:{})});
 if(req.method==='HEAD'){res.end();return;}
 if(native){res.end(bytes.subarray(first,last+1));return;}
 let at=first;const timer=setInterval(()=>{const next=Math.min(at+32768,last+1);res.write(bytes.subarray(at,next));at=next;if(at>last){clearInterval(timer);res.end();}},30);res.on('close',()=>clearInterval(timer));
}
(async()=>{
 assert(app&&host);assert(!fs.existsSync(output),'Use a fresh test output directory');fs.mkdirSync(data,{recursive:true});
 fs.writeFileSync(catalog,JSON.stringify({Schema:1,Settings:{DownloadFolder:path.join(output,'downloads'),CategoryFolders:false,ProxyMode:'Connect directly',SkipBrowserFileInfo:true,PrefetchFileInfo:false,SuppressProgressDialog:true,SuppressCompletionDialog:true,Sound:false,CaptureExtensions:'udmsession',DuplicatePolicy:'Numbered',Connections:8,Retries:0},Queues:[{Name:'Main queue',Enabled:true,Parallel:2}],Downloads:[],Projects:[]}));
 server=http.createServer((req,res)=>{serve(req,res).catch(e=>res.destroy(e));});server.on('connection',s=>{sockets.add(s);s.on('close',()=>sockets.delete(s));});await new Promise(r=>server.listen(0,'127.0.0.1',r));origin='http://127.0.0.1:'+server.address().port;
 desktop=spawn(app,['--background','--data-dir',data,'--instance-tag',tag],{windowsHide:true,stdio:'ignore'});
 const extension=path.join(output,'extension');fs.cpSync(path.join(project,'browser/firefox'),extension,{recursive:true});const background=path.join(extension,'background.js');fs.writeFileSync(background,fs.readFileSync(background,'utf8').replace("'com.udm.download_manager'",JSON.stringify(hostName)));
 const mf=path.join(extension,'manifest.json'),manifest=JSON.parse(fs.readFileSync(mf));manifest.permissions.push('cookies');manifest.optional_permissions=manifest.optional_permissions.filter(x=>x!=='cookies');fs.writeFileSync(mf,JSON.stringify(manifest));
 const id=fs.readFileSync(path.join(project,'browser/extension-id.txt'),'utf8').trim(),hostManifest=path.join(output,'host.json');fs.writeFileSync(hostManifest,JSON.stringify({name:hostName,description:'Isolated UDM download session acceptance',path:host,type:'stdio',allowed_extensions:['udm@local.example']}));
 let exists=false;try{execFileSync('reg.exe',['query',registry],{windowsHide:true,stdio:'ignore'});exists=true;}catch{}assert(!exists);execFileSync('reg.exe',['add',registry,'/ve','/t','REG_SZ','/d',hostManifest,'/f'],{windowsHide:true,stdio:'ignore'});registered=true;

 const observer=[
 'setInterval(async()=>{try{const m=await(await fetch('+JSON.stringify(origin+'/command')+')).json();if(!m)return;let result;',
 "if(m.action==='settings'){await api.storage.local.set({settings:{capture:true,cookies:true,extensions:['udmsession']}});result={ok:true};}",
 "else if(m.action==='downloads')result=await api.downloads.search({url:m.url});",
 "else if(m.action==='storage')result={local:await api.storage.local.get(null),session:await api.storage.session.get(null)};",
 "else if(m.action==='handoff'){const tabs=(await api.tabs.query({})).filter(t=>t.url===m.url+'/login');if(tabs.length!==1)throw Error('Fixture tab missing: '+JSON.stringify((await api.tabs.query({})).map(t=>({id:t.id,url:t.url}))));const tab=tabs[0];result=await handoff({url:m.url+'/private/redirect.udmsession',filename:'redirect.udmsession',referrer:tab.url,tabId:tab.id,frameId:0});}",
 "else result=await nativeRequest(m);",
 'await fetch('+JSON.stringify(origin+'/answer')+",{method:'POST',body:JSON.stringify(result)});}catch(e){await fetch("+JSON.stringify(origin+'/answer')+",{method:'POST',body:JSON.stringify({error:e.message})});}},100);"
 ].join('\n');fs.appendFileSync(background,'\n'+observer);
 const port=await freePort();base='http://127.0.0.1:'+port;
 driver=spawn(process.env.UDM_GECKODRIVER,['--port',String(port),'--profile-root',output],{windowsHide:true,env:{...process.env,UDM_INSTANCE_TAG:tag},stdio:['ignore','pipe','pipe']});const log=fs.createWriteStream(path.join(output,'driver.log'));driver.stdout.pipe(log);driver.stderr.pipe(log);
 await until(async()=>{try{return await request('GET','/status');}catch{return false;}});
 const profile=path.join(output,'profile');fs.mkdirSync(profile);
 const created=await request('POST','/session',{capabilities:{alwaysMatch:{'moz:firefoxOptions':{binary:process.env.UDM_FIREFOX,args:['-headless','-profile',profile],prefs:{'browser.shell.checkDefaultBrowser':false,'extensions.webextensions.warnings-as-errors':false,'browser.download.alwaysOpenPanel':false,'browser.download.folderList':2,'browser.download.dir':path.join(output,'browser-downloads'),'browser.helperApps.neverAsk.saveToDisk':'application/octet-stream'}},acceptInsecureCerts:false}}});session=created.sessionId;
 context={close:async()=>{if(session){await request('DELETE','/session/'+session);session=null;}}};
 assert.equal(await cmd('/moz/addon/install',{path:extension,temporary:true}),'udm@local.example');
 worker={evaluate:async(fn,arg)=>{const code=fn.toString();if(code.includes("action:'diagnostics'"))return bg({action:'diagnostics'});if(code.includes('storage.local.set'))return bg({action:'settings'});if(code.includes('downloads.search'))return bg({action:'downloads',url:arg});if(code.includes('return handoff'))return bg({action:'handoff',url:arg});if(code.includes('storage.local.get'))return bg({action:'storage'});throw Error('Unknown fixture command');}};
 const identity=await worker.evaluate(()=>nativeRequest({action:'diagnostics'}));assert.equal(path.resolve(identity.dataDirectory).toLowerCase(),data.toLowerCase());pass('The real native host connects to the isolated app catalog');
 await worker.evaluate(async()=>{await chrome.storage.local.set({settings:{capture:true,cookies:true,extensions:['udmsession']}});});
 await cmd('/url',{url:origin+'/login'});assert(!await script("return document.cookie.includes('browser-original')"));pass('Website login cookies are HttpOnly and unavailable to page scripts');
 await click('a[href="/private/report.udmsession"]');
 const job=await until(()=>{const j=jobs().find(x=>x.Url===origin+'/private/report.udmsession');if(j?.Status==='Failed')throw Error(j.Error);return j?.Status==='Complete'&&j;});
 assert.equal(hash(fs.readFileSync(path.join(job.Folder,job.FileName))),hash(payload));pass('Automatic browser capture completes a byte-exact authenticated download');
 assert(job.ProtectedBrowserSession);assert(seen.some(x=>x.route==='/private/report.udmsession'&&x.range&&!x.rotated));assert(seen.filter(x=>x.route==='/private/report.udmsession'&&x.range&&x.rotated).length>1);assert(job.Segments.length>1);pass('The native metadata probe rotates the session before parallel range workers start');
 assert(!seen.some(x=>x.elsewhere));pass('The handoff excludes cookies for unrelated paths');
 await until(async()=>{const [d]=await worker.evaluate(url=>chrome.downloads.search({url}),origin+'/private/report.udmsession');return d?.state==='interrupted'&&d.error==='USER_CANCELED';});pass('The browser cancels its transfer only after native acceptance');
 // Explicit link capture preserves the original redirect URL; the real browser
 // first establishes the cookie attributes, then UDM follows the redirect.
 await cmd('/url',{url:origin+'/login'});
 const reply=await worker.evaluate(async url=>{const [tab]=await chrome.tabs.query({url:url+'/login'});return handoff({url:url+'/private/redirect.udmsession',filename:'redirect.udmsession',referrer:tab.url,tabId:tab.id,frameId:0});},origin);assert(reply.ok);
 const redirected=await until(()=>{const j=jobs().find(x=>x.Url===origin+'/private/redirect.udmsession');if(j?.Status==='Failed')throw Error(j.Error);return j?.Status==='Complete'&&j;});assert.equal(fs.readFileSync(path.join(redirected.Folder,redirected.FileName)).toString(),small.toString());assert(seen.some(x=>x.route==='/public/result.udmsession'&&x.root));assert(!seen.some(x=>x.route==='/public/result.udmsession'&&x.privateCookie));pass('A same-origin redirect keeps root cookies and removes private-path cookies');
 const text=fs.readFileSync(catalog,'utf8'),storage=await worker.evaluate(async()=>({local:await chrome.storage.local.get(null),session:await chrome.storage.session.get(null)}));for(const secret of ['browser-original','native-rotated','fixture-root']){assert(!text.includes(secret));assert(!JSON.stringify(storage).includes(secret));}pass('Original and updated cookies are encrypted on disk and absent from extension storage');
 assert.equal(jobs().length,2);pass('Captured downloads create exactly two native records without duplicate browser ownership');
})().catch(async e=>{console.error(e);results.push({passed:false,error:e.stack});if(worker)try{fs.writeFileSync(path.join(output,'failure.json'),JSON.stringify(await worker.evaluate(async()=>({error:(await chrome.storage.local.get('lastError')).lastError,downloads:(await chrome.downloads.search({})).map(x=>({state:x.state,error:x.error,url:x.url}))})),null,2));}catch{}process.exitCode=1;}).finally(async()=>{if(context)await context.close().catch(()=>{});if(driver)driver.kill();if(desktop&&desktop.exitCode===null){desktop.kill();await new Promise(r=>{desktop.once('close',r);setTimeout(r,5000).unref();});}for(const socket of sockets)socket.destroy();if(server)server.close();if(registered)execFileSync('reg.exe',['delete',registry,'/f'],{windowsHide:true,stdio:'ignore'});fs.mkdirSync(output,{recursive:true});fs.writeFileSync(path.join(output,'acceptance.json'),JSON.stringify({browser:channel,scope:'Real browser automatic download capture and native app on localhost; synthetic cookies and isolated profiles only.',results,requests:seen},null,2));console.log(results.filter(x=>x.passed).length+' passed, '+results.filter(x=>!x.passed).length+' failed');});
