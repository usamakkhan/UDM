'use strict';
const fs=require('node:fs'),path=require('node:path'),http=require('node:http'),assert=require('node:assert/strict'),crypto=require('node:crypto'),{spawn,execFileSync}=require('node:child_process');
const project=path.resolve(__dirname,'..'),output=path.resolve(process.argv[2]),channel='firefox',tag='grabber'+crypto.randomBytes(5).toString('hex'),results=[],seen=[];
const app=process.env.UDM_APP_EXE,host=process.env.UDM_HOST_EXE,tests=process.env.UDM_TEST_EXE,hostName='com.udm.grabberfixture'+Date.now();
const registry='HKCU\\Software\\'+'Mozilla'+'\\NativeMessagingHosts\\'+hostName;
const dataDir=path.join(output,'state'),stateFile=path.join(dataDir,'state.json'),payload=Buffer.alloc(2*1024*1024+31,67),publicPayload=Buffer.from('public result without private-path cookies');
let server,context,desktop,registered=false,origin,driver,session,base,pending=null,answer=null;const sockets=new Set();let phase='browser',currentSecret='fixture-private-secret';const imagePayload=Buffer.from('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+a8p0AAAAASUVORK5CYII=','base64');
const wait=ms=>new Promise(r=>setTimeout(r,ms));
async function until(fn,timeout=25000){const end=Date.now()+timeout;while(Date.now()<end){const value=await fn();if(value)return value;await wait(100);}throw Error('Grabber session fixture timed out');}
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
function pass(name){results.push({name,passed:true});console.log('PASS '+name);}
function state(){try{return JSON.parse(fs.readFileSync(stateFile));}catch{return null;}}
async function stopDesktop(){if(desktop&&desktop.exitCode===null){desktop.kill();await new Promise(resolve=>{desktop.once('close',resolve);setTimeout(resolve,5000).unref();});}desktop=null;}
async function spec(operation,values={}){const file=path.join(output,'input.json');fs.writeFileSync(file,JSON.stringify({operation,root:dataDir,...values}));await require('node:util').promisify(require('node:child_process').execFile)(tests,['--grabber-session-spec',file],{windowsHide:true,timeout:90000});return JSON.parse(fs.readFileSync(path.join(output,'result.json')));}

async function request(method,route,data){const r=await fetch(base+route,{method,headers:{'Content-Type':'application/json'},body:data?JSON.stringify(data):undefined,signal:AbortSignal.timeout(50000)}),j=await r.json();if(j.value?.error)throw Error(j.value.error+': '+j.value.message);return j.value;}
const cmd=(route,data)=>request('POST','/session/'+session+route,data);
async function bg(message){answer=null;pending=message;const result=await until(()=>answer);if(result.error)throw Error(result.error);return result;}
async function element(selector){const value=await cmd('/element',{using:'css selector',value:selector});return value['element-6066-11e4-a52e-4f735466cecf'];}
async function click(selector){await cmd('/element/'+await element(selector)+'/click',{});}
async function script(code){return cmd('/execute/sync',{script:code,args:[]});}
async function freePort(){const listener=require('node:net').createServer();await new Promise(r=>listener.listen(0,'127.0.0.1',r));const port=listener.address().port;await new Promise(r=>listener.close(r));return port;}
async function serve(req,res){
 if(req.url==='/command'){res.setHeader('Content-Type','application/json');res.end(JSON.stringify(pending));pending=null;return;}
 if(req.url==='/answer'){let body='';for await(const chunk of req)body+=chunk;answer=JSON.parse(body);res.end('{}');return;}
 const url=new URL(req.url,origin);seen.push({path:url.pathname,method:req.method,phase,privateCookie:(req.headers.cookie||'').includes('session='),rotatedCookie:(req.headers.cookie||'').includes('session=fixture-'+phase+'-secret'),nativeStep:(req.headers.cookie||'').includes('nativeStep='),rootCookie:(req.headers.cookie||'').includes('root=fixture-root-secret'),elsewhereCookie:(req.headers.cookie||'').includes('elsewhere='),range:!!req.headers.range});
 if(url.pathname==='/login'&&req.method==='GET'){res.setHeader('Content-Type','text/html');res.end('<!doctype html><title>UDM fixture sign-in</title><form method="POST"><label>User<input name="user"></label><label>Password<input name="password" type="password"></label><button>Sign in</button></form>');return;}
 if(url.pathname==='/login'&&req.method==='POST'){let body='';for await(const chunk of req)body+=chunk;const values=new URLSearchParams(body);assert.equal(values.get('user'),'fixture-user');assert.equal(values.get('password'),'fixture-password');res.writeHead(303,{'Set-Cookie':['session=fixture-private-secret; HttpOnly; Path=/private; SameSite=Lax','root=fixture-root-secret; HttpOnly; Path=/; SameSite=Strict; Max-Age=3600','elsewhere=fixture-other-path; HttpOnly; Path=/elsewhere; SameSite=Lax'],Location:'/private/index.html'});res.end();return;}
 if(url.pathname.startsWith('/private')&&!(req.headers.cookie||'').includes('session='+currentSecret)){res.writeHead(401);res.end('Sign in first');return;}
 if(url.pathname==='/private/index.html'){if(phase!=='browser'){currentSecret='fixture-'+phase+'-secret';res.setHeader('Set-Cookie',['session='+currentSecret+'; HttpOnly; Path=/private','nativeStep='+(phase==='offline'?'; Max-Age=0':'present')+'; Path=/private; HttpOnly']);}res.setHeader('Content-Type','text/html');res.end('<!doctype html><title>Private download fixture</title><h1>Signed in</h1><img src="pixel.png"><a href="report.bin">Confidential report</a><a href="redirect.bin">Public attachment</a><a href="/logout">Sign out</a>');return;}
 if(url.pathname==='/private/pixel.png'){if(phase==='offline'&&(req.headers.cookie||'').includes('nativeStep=')){res.writeHead(400);res.end('Deleted cookie was reused');return;}res.writeHead(200,{'Content-Type':'image/png','Content-Length':imagePayload.length});res.end(req.method==='HEAD'?undefined:imagePayload);return;}
 if(url.pathname==='/logout'){res.writeHead(200,{'Set-Cookie':'session=; Max-Age=0; Path=/private'});res.end('Logged out');return;}
 if(url.pathname==='/private/redirect.bin'){res.writeHead(302,{Location:'/public/result.bin'});res.end();return;}
 if(url.pathname==='/public/result.bin'&&(req.headers.cookie||'').includes('session=')){res.writeHead(400);res.end('Private cookie leaked');return;}
 const bytes=url.pathname==='/private/report.bin'?payload:url.pathname==='/public/result.bin'?publicPayload:null;if(!bytes){res.writeHead(404);res.end();return;}
 const match=/^bytes=(\d+)-(\d*)$/.exec(req.headers.range||'');let first=match?+match[1]:0,last=match&&match[2]?+match[2]:bytes.length-1;last=Math.min(last,bytes.length-1);
 res.writeHead(match?206:200,{'Content-Type':'application/octet-stream','Content-Length':last-first+1,'Accept-Ranges':'bytes',ETag:'"grabber-session-'+bytes.length+'"',...(match?{'Content-Range':`bytes ${first}-${last}/${bytes.length}`}:{})});res.end(req.method==='HEAD'?undefined:bytes.subarray(first,last+1));
}
(async()=>{
 assert(app&&host&&tests,'Provide the isolated native executables');assert(!fs.existsSync(output),'Use a fresh fixture directory');fs.mkdirSync(output,{recursive:true});
 server=http.createServer((req,res)=>{serve(req,res).catch(error=>{res.destroy(error);});});server.on('connection',socket=>{sockets.add(socket);socket.on('close',()=>sockets.delete(socket));});await new Promise(r=>server.listen(0,'127.0.0.1',r));origin='http://127.0.0.1:'+server.address().port;
 const seed=await spec('seed',{url:origin+'/private/index.html',login:origin+'/login'});assert(seed.ticket&&seed.projectId);pass('Native wizard logic arms a saved, expiring project sign-in request');
 desktop=spawn(app,['--background','--data-dir',dataDir,'--instance-tag',tag],{windowsHide:true,stdio:'ignore'});desktop.on('error',e=>{throw e;});
 const extension=path.join(output,'extension');fs.cpSync(path.join(project,'browser/firefox'),extension,{recursive:true});const background=path.join(extension,'background.js');fs.writeFileSync(background,fs.readFileSync(background,'utf8').replace("'com.udm.download_manager'",JSON.stringify(hostName)));
 const manifestPath=path.join(extension,'manifest.json'),manifest=JSON.parse(fs.readFileSync(manifestPath));manifest.permissions.push('cookies');manifest.optional_permissions=manifest.optional_permissions.filter(x=>x!=='cookies');fs.writeFileSync(manifestPath,JSON.stringify(manifest));
 const id=fs.readFileSync(path.join(project,'browser/extension-id.txt'),'utf8').trim(),hostManifest=path.join(output,'host.json');fs.writeFileSync(hostManifest,JSON.stringify({name:hostName,description:'Isolated UDM website session acceptance',path:host,type:'stdio',allowed_extensions:['udm@local.example']}));
 let exists=false;try{execFileSync('reg.exe',['query',registry],{windowsHide:true,stdio:'ignore'});exists=true;}catch{}assert(!exists);execFileSync('reg.exe',['add',registry,'/ve','/t','REG_SZ','/d',hostManifest,'/f'],{windowsHide:true,stdio:'ignore'});registered=true;

 const observer=[
 'setInterval(async()=>{try{const m=await(await fetch('+JSON.stringify(origin+'/command')+')).json();if(!m)return;let result;',
 "if(m.action==='popup'){const tab=await api.tabs.create({url:api.runtime.getURL('popup.html'),active:false});await new Promise(resolve=>{const timer=setInterval(async()=>{if((await api.tabs.get(tab.id)).status==='complete'){clearInterval(timer);resolve();}},30);});result={url:api.runtime.getURL('popup.html'),id:tab.id};}",
 "else if(m.action==='storage')result=await api.storage.local.get(null);else result=await nativeRequest(m);",
 'await fetch('+JSON.stringify(origin+'/answer')+",{method:'POST',body:JSON.stringify(result)});}catch(e){await fetch("+JSON.stringify(origin+'/answer')+",{method:'POST',body:JSON.stringify({error:e.message})});}},100);"
 ].join('\n');fs.appendFileSync(background,'\n'+observer);
 const port=await freePort();base='http://127.0.0.1:'+port;
 driver=spawn(process.env.UDM_GECKODRIVER,['--port',String(port),'--profile-root',output],{windowsHide:true,env:{...process.env,UDM_INSTANCE_TAG:tag},stdio:['ignore','pipe','pipe']});const log=fs.createWriteStream(path.join(output,'driver.log'));driver.stdout.pipe(log);driver.stderr.pipe(log);
 await until(async()=>{try{return await request('GET','/status');}catch{return false;}});
 const profile=path.join(output,'profile');fs.mkdirSync(profile);
 const created=await request('POST','/session',{capabilities:{alwaysMatch:{'moz:firefoxOptions':{binary:process.env.UDM_FIREFOX,args:['-headless','-profile',profile],prefs:{'browser.shell.checkDefaultBrowser':false,'extensions.webextensions.warnings-as-errors':false,'browser.download.alwaysOpenPanel':false}},acceptInsecureCerts:false}}});session=created.sessionId;
 context={close:async()=>{if(session){await request('DELETE','/session/'+session);session=null;}}};
 assert.equal(await cmd('/moz/addon/install',{path:extension,temporary:true}),'udm@local.example');
 assert((await bg({action:'diagnostics'})).ok);
 await cmd('/url',{url:origin+'/login'});await cmd('/element/'+await element('input[name=user]')+'/value',{text:'fixture-user'});await cmd('/element/'+await element('input[name=password]')+'/value',{text:'fixture-password'});await click('button');
 await until(async()=>await script("return document.querySelector('h1')?.textContent==='Signed in'"));pass('The real browser completes an HTTP-only cookie sign-in without exposing the password to UDM');
 const cookies=await request('GET','/session/'+session+'/cookie');assert(cookies.find(x=>x.name==='session')?.httpOnly);assert(!await script("return document.cookie.includes('fixture-private-secret')"));pass('The protected session uses HttpOnly cookies unavailable to page JavaScript');
 const popupInfo=await bg({action:'popup'});const handles=await request('GET','/session/'+session+'/window/handles');let selected=false;for(const handle of handles){await cmd('/window',{handle});if(await request('GET','/session/'+session+'/url')===popupInfo.url){selected=true;break;}}assert(selected);
 await until(async()=>await request('GET','/session/'+session+'/element/'+await element('#grabber-login')+'/displayed'));assert.equal((await cmd('/elements',{using:'css selector',value:'#grabber-project option'})).length,1);pass('The actual extension popup offers only the matching armed Grabber project');
 fs.writeFileSync(path.join(output,'session-popup.png'),Buffer.from(await request('GET','/session/'+session+'/screenshot'),'base64'));
 await click('#grabber-complete');await until(()=>!!state()?.Projects?.[0]?.ProtectedBrowserSession);assert((await request('GET','/session/'+session+'/element/'+await element('#status')+'/text')).includes('Website sign-in saved'));pass('A popup button transfers the selected session through the real native host');
 const popup={close:async()=>request('DELETE','/session/'+session+'/window')},worker={evaluate:async()=>bg({action:'storage'})};
 const catalog=fs.readFileSync(stateFile,'utf8');assert(!catalog.includes('fixture-private-secret')&&!catalog.includes('fixture-password')&&!catalog.includes('fixture-root-secret'));assert(!state().Projects[0].PendingBrowserLogin);pass('The saved catalog protects cookie values and consumes the one-time sign-in request');
 const local=await worker.evaluate(()=>chrome.storage.local.get(null));assert(!JSON.stringify(local).includes('fixture-private-secret'));pass('The extension does not persist the transferred session in local storage');
 await popup.close();await context.close();context=null;await stopDesktop();phase='native';const result=await spec('run');assert(result.sessionSaved);assert.deepEqual(result.errors,[]);assert.equal(result.files.length,2);const hashes=result.files.map(f=>hash(fs.readFileSync(f.path)));assert(hashes.includes(hash(payload))&&hashes.includes(hash(publicPayload)));pass('After the browser and desktop close, the saved session explores and downloads byte-exact files');
 assert(result.files.some(x=>x.description==='Confidential report'));pass('Authenticated file links retain their download descriptions');
 assert(seen.some(x=>x.path==='/public/result.bin'&&x.rootCookie));assert(!seen.some(x=>x.path==='/logout'));assert(!seen.some(x=>x.path.startsWith('/public')&&x.privateCookie));assert(!seen.some(x=>x.elsewhereCookie));pass('Exploration excludes logout pages and never leaks private-path cookies through redirects');
 assert(seen.some(x=>x.path==='/private/report.bin'&&x.method==='HEAD'&&x.privateCookie)&&seen.some(x=>x.path==='/private/report.bin'&&x.method==='GET'&&x.range&&x.privateCookie));pass('Authenticated metadata checks and ranged file requests both carry the correct cookies');

 assert(seen.some(x=>x.phase==='native'&&x.path==='/private/report.bin'&&x.rotatedCookie&&x.nativeStep));assert(!fs.readFileSync(stateFile,'utf8').includes('fixture-native-secret'));pass('Native exploration accepts replacement cookies and keeps them encrypted for queued ranged downloads');
 const offlineRoot=path.join(output,'offline');fs.mkdirSync(offlineRoot);phase='offline';const savedProject=state().Projects[0],offlineInput=path.join(offlineRoot,'input.json');
 fs.writeFileSync(offlineInput,JSON.stringify({url:origin+'/private/index.html',offline:{...savedProject,Depth:1,MaxPages:10,MaxMiB:8,ExternalAssets:false,SaveMode:'Folder',Folder:path.join(offlineRoot,'downloads')}}));
 await require('node:util').promisify(require('node:child_process').execFile)(tests,['--feature-spec',offlineInput],{windowsHide:true,timeout:90000});
 const archive=JSON.parse(fs.readFileSync(path.join(offlineRoot,'result.json')));assert.equal(archive.status,'Complete',JSON.stringify(archive));
 await require('node:util').promisify(require('node:child_process').execFile)(process.env.UDM_PYTHON,['-c',"import sys,zipfile; z=zipfile.ZipFile(sys.argv[1]); assert b'<h1>Signed in</h1>' in z.read('index.html')",archive.path],{windowsHide:true,timeout:20000});
 assert(!seen.some(x=>x.path==='/logout'));pass('The same saved browser session creates an offline ZIP containing the authenticated starting page');
 assert(seen.some(x=>x.phase==='offline'&&x.path==='/private/pixel.png'&&x.rotatedCookie&&!x.nativeStep));pass('Offline mirroring follows a second session rotation and honors cookie deletion before fetching an image');

})().catch(async error=>{if(session)try{const detail=await request('GET','/session/'+session+'/element/'+await element('#status')+'/text');fs.writeFileSync(path.join(output,'popup-error.txt'),detail);console.error('Popup status: '+detail);}catch{}results.push({passed:false,error:error.stack});console.error(error);process.exitCode=1;}).finally(async()=>{if(context)await context.close().catch(()=>{});if(driver)driver.kill();await stopDesktop();for(const socket of sockets)socket.destroy();if(server)server.close();if(registered)execFileSync('reg.exe',['delete',registry,'/f'],{windowsHide:true,stdio:'ignore'});fs.mkdirSync(output,{recursive:true});fs.writeFileSync(path.join(output,'acceptance.json'),JSON.stringify({browser:channel,scope:'Isolated localhost sign-in, actual extension popup/native app and native Grabber; no real account or personal browser profile used.',results,requests:seen},null,2));console.log(results.filter(x=>x.passed).length+' passed, '+results.filter(x=>!x.passed).length+' failed');});

