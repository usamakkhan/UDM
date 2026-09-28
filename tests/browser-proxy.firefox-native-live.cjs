'use strict';
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict'),http=require('node:http'),net=require('node:net'),crypto=require('node:crypto'),{spawn,execFileSync}=require('node:child_process');
const {socksFixture}=require('./support/socks-fixture.cjs');
const project=path.resolve(__dirname,'..'),root=path.resolve(process.argv[2]),tag='proxy'+crypto.randomBytes(6).toString('hex'),results=[],seen=[],payload=Buffer.alloc(4*1024*1024+31,79),hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const hostName='com.udm.proxyfixture'+Date.now(),registry='HKCU\\Software\\Mozilla\\NativeMessagingHosts\\'+hostName,state=path.join(root,'state/state.json');
let server,A,B,S,driver,desktop,session,base,origin,pending=null,answer=null,registered=false;const sockets=new Set();
const wait=ms=>new Promise(r=>setTimeout(r,ms));
async function until(fn,ms=45000){const end=Date.now()+ms;while(Date.now()<end){const r=await fn();if(r)return r;await wait(100);}throw Error('Firefox proxy fixture timed out');}
async function request(method,route,data){const r=await fetch(base+route,{method,headers:{'Content-Type':'application/json'},body:data?JSON.stringify(data):undefined,signal:AbortSignal.timeout(50000)}),j=await r.json();if(j.value?.error)throw Error(j.value.error+': '+j.value.message);return j.value;}
const cmd=(route,data)=>request('POST','/session/'+session+route,data);
async function bg(message){answer=null;pending=message;const r=await until(()=>answer);if(r.error)throw Error(r.error);return r;}
const pass=name=>{results.push({passed:true,name});console.log('PASS '+name);};
function jobs(){try{return JSON.parse(fs.readFileSync(state)).Downloads;}catch{return [];}}
async function listen(s){s.on('connection',c=>{sockets.add(c);c.on('close',()=>sockets.delete(c));});await new Promise(r=>s.listen(0,'127.0.0.1',r));return s;}
async function freePort(){const s=await listen(net.createServer());const p=s.address().port;await new Promise(r=>s.close(r));return p;}
function serve(label,req,res){
 let u;try{u=new URL(req.url,'http://'+req.headers.host);}catch{res.writeHead(400);res.end();return;}
 if(u.hostname==='udm-ftp.invalid')label='SOCKS';
 seen.push({route:label,method:req.method,url:u.href,range:req.headers.range||'',proxyAuth:req.headers['proxy-authorization']||''});
 if(u.pathname==='/page'){res.setHeader('Content-Type','text/html');res.end('<!doctype html><title>UDM request proxy fixture</title><a id="a" href="http://alpha.udm-proxy.invalid/a.udmroute">A download</a><a id="b" href="http://beta.udm-proxy.invalid/b.udmroute">B download</a><a id="d" href="'+origin+'/direct.udmroute">Direct download</a><a id="s" href="http://udm-ftp.invalid:'+server.address().port+'/socks.udmroute">SOCKS download</a>');return;}
 if(!u.pathname.endsWith('.udmroute')){res.writeHead(404);res.end();return;}
 let begin=0,end=payload.length-1;const range=/^bytes=(\d+)-(\d*)$/.exec(req.headers.range||'');if(range){begin=+range[1];end=range[2]?+range[2]:end;}
 res.writeHead(range?206:200,{'Content-Type':'application/octet-stream','Content-Disposition':'attachment; filename="'+u.pathname.slice(1)+'"','Content-Length':end-begin+1,ETag:'"proxy-capture"','Accept-Ranges':'bytes',...(range?{'Content-Range':'bytes '+begin+'-'+end+'/'+payload.length}:{})});
 if(req.method==='HEAD'){res.end();return;}
 let at=begin;const timer=setInterval(()=>{const next=Math.min(at+65536,end+1);res.write(payload.subarray(at,next));at=next;if(at===end+1){clearInterval(timer);res.end();}},50);res.on('close',()=>clearInterval(timer));
}
(async()=>{
 assert(!fs.existsSync(root),'Use a fresh fixture directory');fs.mkdirSync(path.dirname(state),{recursive:true});fs.mkdirSync(path.join(root,'browser-downloads'));
 A=await listen(http.createServer((req,res)=>serve('A',req,res)));B=await listen(http.createServer((req,res)=>serve('B',req,res)));
 server=await listen(http.createServer((req,res)=>{
  if(req.url==='/command'){res.setHeader('Content-Type','application/json');res.end(JSON.stringify(pending));pending=null;return;}
  if(req.url==='/answer'){let body='';req.on('data',b=>body+=b);req.on('end',()=>{answer=JSON.parse(body);res.end('{}');});return;}
  if(req.url==='/proxy.pac'){res.setHeader('Content-Type','application/x-ns-proxy-autoconfig');res.end('function FindProxyForURL(url,host){ if(host==="alpha.udm-proxy.invalid")return "PROXY 127.0.0.1:'+A.address().port+'"; if(host==="beta.udm-proxy.invalid")return "PROXY 127.0.0.1:'+B.address().port+'"; if(host==="udm-ftp.invalid")return "SOCKS5 '+S.settings.address+'"; return "DIRECT"; }');return;}
  serve('direct',req,res);
 }));origin='http://127.0.0.1:'+server.address().port;S=await socksFixture({url:'ftp://udm-ftp.invalid:'+server.address().port,ports:new Set()}).start();
 fs.writeFileSync(state,JSON.stringify({Schema:1,Settings:{DownloadFolder:path.join(root,'downloads'),CategoryFolders:false,ProxyMode:'Use a proxy server',Proxy:'127.0.0.1:1',ProxyUser:'unrelated-proxy-user',SkipBrowserFileInfo:true,PrefetchFileInfo:true,SuppressProgressDialog:true,SuppressCompletionDialog:true,Sound:false,CaptureExtensions:'udmroute',DuplicatePolicy:'Numbered',Connections:4},Queues:[{Name:'Main queue',Enabled:true,Parallel:1}],Downloads:[],Projects:[]}));
 const ext=path.join(root,'extension');fs.cpSync(path.join(project,'browser/firefox'),ext,{recursive:true});
 const bgFile=path.join(ext,'background.js');let background=fs.readFileSync(bgFile,'utf8').replace("'com.udm.download_manager'",JSON.stringify(hostName));
 const observer=[
 'setInterval(async()=>{try{const m=await(await fetch('+JSON.stringify(origin+'/command')+')).json();if(!m)return;let result;',
 "if(m.action==='configure'){const {settings}=await api.storage.local.get('settings');await api.storage.local.set({settings:{...settings,capture:true,extensions:['udmroute'],cookies:false}});result={ok:true};}",
 "else if(m.action==='debug')result={context:requestContext.diagnostics(),downloads:await api.downloads.search({}),storage:await api.storage.local.get(null)};",
 "else if(m.action==='downloads')result=await api.downloads.search({url:m.url});",
 "else if(m.action==='storage')result={local:await api.storage.local.get(null),session:await api.storage.session.get(null)};",
 'else result=await nativeRequest(m);await fetch('+JSON.stringify(origin+'/answer')+",{method:'POST',body:JSON.stringify(result)});",
 '}catch(e){await fetch('+JSON.stringify(origin+'/answer')+",{method:'POST',body:JSON.stringify({error:e.message})}).catch(()=>{});}},100);"
 ].join('\n');fs.writeFileSync(bgFile,background+'\n'+observer);
 const hostManifest=path.join(root,'host.json');fs.writeFileSync(hostManifest,JSON.stringify({name:hostName,description:'UDM isolated proxy fixture',path:process.env.UDM_HOST_EXE,type:'stdio',allowed_extensions:['udm@local.example']}));
 let exists=false;try{execFileSync('reg.exe',['query',registry],{windowsHide:true,stdio:'ignore'});exists=true;}catch{}assert(!exists);
 execFileSync('reg.exe',['add',registry,'/ve','/t','REG_SZ','/d',hostManifest,'/f'],{windowsHide:true,stdio:'ignore'});registered=true;
 desktop=spawn(process.env.UDM_APP_EXE,['--background','--data-dir',path.dirname(state),'--instance-tag',tag],{windowsHide:true,stdio:'ignore'});
 const profile=path.join(root,'profile');fs.mkdirSync(profile);base='http://127.0.0.1:'+await freePort();
 driver=spawn(process.env.UDM_GECKODRIVER,['--port',new URL(base).port,'--profile-root',root],{windowsHide:true,env:{...process.env,UDM_INSTANCE_TAG:tag},stdio:['ignore','pipe','pipe']});const log=fs.createWriteStream(path.join(root,'driver.log'));driver.stdout.pipe(log);driver.stderr.pipe(log);
 await until(async()=>{try{return await request('GET','/status');}catch{return false;}});
 const created=await request('POST','/session',{capabilities:{alwaysMatch:{'moz:firefoxOptions':{binary:process.env.UDM_FIREFOX,args:['-headless','-profile',profile],prefs:{'browser.shell.checkDefaultBrowser':false,'network.proxy.type':2,'network.proxy.socks_remote_dns':true,'network.proxy.socks5_remote_dns':true,'network.proxy.autoconfig_url':origin+'/proxy.pac','network.proxy.no_proxies_on':'localhost,127.0.0.1','browser.download.folderList':2,'browser.download.dir':path.join(root,'browser-downloads'),'browser.download.useDownloadDir':true,'browser.helperApps.neverAsk.saveToDisk':'application/octet-stream','browser.download.alwaysOpenPanel':false}},acceptInsecureCerts:false}}});session=created.sessionId;
 assert.equal(await cmd('/moz/addon/install',{path:ext,temporary:true}),'udm@local.example');const identity=await bg({action:'diagnostics'});assert.equal(identity.version,'0.41.0');assert.equal(path.resolve(identity.dataDirectory).toLowerCase(),path.dirname(state).toLowerCase());assert.equal((await bg({action:'preferences'})).browserProxy,1);await bg({action:'configure'});pass('Firefox extension 0.30.0 negotiates the new route capability with isolated native 0.41.0');
 await cmd('/url',{url:origin+'/page'});
 for(const [id,label,url] of [['a','A','http://alpha.udm-proxy.invalid/a.udmroute'],['b','B','http://beta.udm-proxy.invalid/b.udmroute'],['d','direct',origin+'/direct.udmroute'],['s','SOCKS','http://udm-ftp.invalid:'+server.address().port+'/socks.udmroute']]){
  const element=await cmd('/element',{using:'css selector',value:'#'+id});await cmd('/element/'+element['element-6066-11e4-a52e-4f735466cecf']+'/click',{});
  const j=await until(()=>{const j=jobs().find(x=>x.Url===url);if(j?.Status==='Failed')throw Error(j.Error);return j?.Status==='Complete'&&j;});
  assert(j.ProtectedBrowserProxy);assert.equal(hash(fs.readFileSync(path.join(j.Folder,j.FileName))),hash(payload));pass(label+' request publishes exact bytes with a protected browser route despite the blocked desktop default');
  const traffic=seen.filter(x=>x.url===url);assert(traffic.some(x=>x.range));assert(traffic.every(x=>x.route===label));assert(traffic.every(x=>!x.proxyAuth));pass(label+' browser and native range requests use only the observed route without proxy credentials');
  await until(async()=>{const rows=await bg({action:'downloads',url});return rows[0]?.state==='interrupted'&&!rows[0]?.paused&&rows[0]?.error==='USER_CANCELED';});pass(label+' browser download is canceled after the native acknowledgement');
 }
 assert(S.metrics.destinations.length>1&&S.metrics.destinations.every(x=>x.domain&&x.host==='udm-ftp.invalid'));pass('Firefox and native SOCKS connections preserve proxy-side DNS');
 const storage=JSON.stringify(await bg({action:'storage'}));assert(!storage.includes('"host":"127.0.0.1","port":'));assert(!storage.includes('"proxyInfo"'));assert(!storage.includes('"ProtectedBrowserProxy"'));pass('Captured request routes are absent from extension storage');
 fs.writeFileSync(path.join(root,'traffic.json'),JSON.stringify(seen,null,2));
})().catch(async e=>{results.push({passed:false,error:e.stack});console.error(e);if(session)try{fs.writeFileSync(path.join(root,'failure.json'),JSON.stringify({browser:await bg({action:'debug'}),traffic:seen},null,2));}catch{}process.exitCode=1;}).finally(async()=>{
 if(session)try{await request('DELETE','/session/'+session);}catch{}if(driver)driver.kill();if(desktop)desktop.kill();
 if(S)S.close();for(const s of sockets)s.destroy();for(const s of [server,A,B])if(s)await new Promise(r=>s.close(r));
 if(registered)try{execFileSync('reg.exe',['delete',registry,'/f'],{windowsHide:true,stdio:'ignore'});}catch{process.exitCode=1;}
 fs.mkdirSync(root,{recursive:true});fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({passed:results.filter(x=>x.passed).length,failed:results.filter(x=>!x.passed).length,results},null,2));
});
