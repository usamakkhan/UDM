'use strict';
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict'),http=require('node:http'),https=require('node:https'),net=require('node:net'),crypto=require('node:crypto'),{spawn,execFileSync}=require('node:child_process'),{chromium}=require('playwright');
const {socksFixture}=require('./support/socks-fixture.cjs');
const project=path.resolve(__dirname,'..'),root=path.resolve(process.argv[2]),channel=process.env.UDM_TEST_BROWSER||'msedge',tag='edgeproxy'+crypto.randomBytes(6).toString('hex'),results=[],seen=[];
const payload=Buffer.alloc(4*1024*1024+31,79),hash=b=>crypto.createHash('sha256').update(b).digest('hex'),hostName='com.udm.edgeproxyfixture'+Date.now(),registry='HKCU\\Software\\'+(channel==='msedge'?'Microsoft\\Edge':'Google\\Chrome')+'\\NativeMessagingHosts\\'+hostName,state=path.join(root,'state/state.json'),configurations=[],requestFailures=[];
let server,A,B,S,S4,T,context,desktop,worker,page,origin,registered=false;const sockets=new Set();
const wait=ms=>new Promise(r=>setTimeout(r,ms));
async function until(fn,ms=45000){const end=Date.now()+ms;while(Date.now()<end){const value=await fn();if(value)return value;await wait(100);}throw Error('Chromium proxy fixture timed out');}
const pass=name=>{results.push({passed:true,name});console.log('PASS '+name);};
function jobs(){try{return JSON.parse(fs.readFileSync(state)).Downloads;}catch{return [];}}
async function listen(s){s.on('connection',c=>{sockets.add(c);c.on('close',()=>sockets.delete(c));});await new Promise(r=>s.listen(0,'127.0.0.1',r));return s;}
function serve(label,req,res){
 const u=new URL(req.url,'http://'+req.headers.host);if(u.hostname==='udm-ftp.invalid')label='SOCKS';if(u.hostname==='198.51.100.25'||u.pathname==='/socks-localhost.udmroute')label='SOCKS4';
 seen.push({route:label,method:req.method,url:u.href,range:req.headers.range||'',encrypted:!!req.socket.encrypted,hasProxyAuth:!!req.headers['proxy-authorization'],hasSession:req.headers.cookie?.includes('edge-session=synthetic')||false});
 if(u.pathname==='/page'){res.setHeader('Content-Type','text/html');res.setHeader('Set-Cookie','edge-session=synthetic; Path=/; HttpOnly; SameSite=Lax');res.end('<!doctype html><title>UDM isolated browser proxy fixture</title><a id="download">Download file</a>');return;}
 if(u.pathname==='/manual.udmroute'&&!req.headers.cookie?.includes('edge-session=synthetic')){res.writeHead(401);res.end('session required');return;}
 if(!u.pathname.endsWith('.udmroute')){res.writeHead(404);res.end();return;}
 let first=0,last=payload.length-1;const range=/^bytes=(\d+)-(\d*)$/.exec(req.headers.range||'');if(range){first=+range[1];last=range[2]?+range[2]:last;}
 res.writeHead(range?206:200,{'Content-Type':'application/octet-stream','Content-Disposition':'attachment; filename="'+u.pathname.slice(1)+'"','Content-Length':last-first+1,ETag:'"edge-proxy-fixture"','Accept-Ranges':'bytes',...(range?{'Content-Range':`bytes ${first}-${last}/${payload.length}`}:{})});
 if(req.method==='HEAD'){res.end();return;}if(range){res.end(payload.subarray(first,last+1));return;}
 // Keep the browser response alive long enough to exercise the pause/ack path
 // under cold-profile storage latency; native ranges still return immediately.
 let at=first;const timer=setInterval(()=>{const next=Math.min(at+16384,last+1);res.write(payload.subarray(at,next));at=next;if(at>last){clearInterval(timer);res.end();}},50);res.on('close',()=>clearInterval(timer));
}
async function configure(config){
 await worker.evaluate(config=>new Promise((resolve,reject)=>chrome.proxy.settings.set({value:config,scope:'regular'},()=>chrome.runtime.lastError?reject(Error(chrome.runtime.lastError.message)):resolve())),config);
 await wait(150);const current=await worker.evaluate(()=>new Promise(resolve=>chrome.proxy.settings.get({incognito:false},resolve)));assert.equal(current.value.mode,config.mode);configurations.push(current.value);
}
async function clickDownload(url){await page.locator('#download').evaluate((a,url)=>a.href=url,url);await page.locator('#download').click();}
async function complete(url,label){
 const job=await until(()=>{const j=jobs().find(x=>x.Url===url);if(j?.Status==='Failed')throw Error(j.Error);return j?.Status==='Complete'&&j;});
 assert(job.ProtectedBrowserProxy);assert.equal(hash(fs.readFileSync(path.join(job.Folder,job.FileName))),hash(payload));pass(label+' publishes exact bytes with a protected route despite the blocked desktop proxy');
 const traffic=seen.filter(x=>x.url===url);assert(traffic.some(x=>x.range));assert(traffic.every(x=>x.route===label));assert(traffic.every(x=>!x.hasProxyAuth));pass(label+' native ranges use the browser route without unrelated proxy credentials');
 return job;
}
(async()=>{
 assert(!fs.existsSync(root),'Use a fresh fixture directory');assert(process.env.UDM_APP_EXE&&process.env.UDM_HOST_EXE);fs.mkdirSync(path.dirname(state),{recursive:true});
 A=await listen(http.createServer((req,res)=>serve('A',req,res)));B=await listen(http.createServer((req,res)=>serve('B',req,res)));server=await listen(http.createServer((req,res)=>serve('direct',req,res)));origin='http://127.0.0.1:'+server.address().port;
 S=await socksFixture({url:'ftp://udm-ftp.invalid:'+server.address().port,ports:new Set()}).start();
 S4=await socksFixture({url:'ftp://udm-ftp.invalid:'+server.address().port,ports:new Set()},{version:4}).start();
 assert(process.env.UDM_TEST_TLS_DIR);T=await listen(https.createServer({key:fs.readFileSync(path.join(process.env.UDM_TEST_TLS_DIR,'key.pem')),cert:fs.readFileSync(path.join(process.env.UDM_TEST_TLS_DIR,'cert.pem'))},(q,r)=>serve('TLS',q,r)));
 fs.writeFileSync(state,JSON.stringify({Schema:1,Settings:{DownloadFolder:path.join(root,'downloads'),CategoryFolders:false,ProxyMode:'Use a proxy server',Proxy:'127.0.0.1:1',ProxyUser:'unrelated-proxy-user',SkipBrowserFileInfo:true,PrefetchFileInfo:false,SuppressProgressDialog:true,SuppressCompletionDialog:true,Sound:false,CaptureExtensions:'udmroute',DuplicatePolicy:'Numbered',Connections:4,Retries:0},Queues:[{Name:'Main queue',Enabled:true,Parallel:1}],Downloads:[],Projects:[]}));
 const ext=path.join(root,'extension');fs.cpSync(process.env.UDM_BROWSER_SOURCE||path.join(project,'browser/chromium'),ext,{recursive:true});const bg=path.join(ext,'background.js');fs.writeFileSync(bg,fs.readFileSync(bg,'utf8').replace("'com.udm.download_manager'",JSON.stringify(hostName)));
 const mf=path.join(ext,'manifest.json'),manifest=JSON.parse(fs.readFileSync(mf));for(const name of ['proxy','cookies'])if(!manifest.permissions.includes(name))manifest.permissions.push(name);manifest.optional_permissions=manifest.optional_permissions.filter(p=>p!=='cookies');fs.writeFileSync(mf,JSON.stringify(manifest));
 const hostManifest=path.join(root,'host.json');fs.writeFileSync(hostManifest,JSON.stringify({name:hostName,description:'UDM isolated Edge proxy acceptance',path:process.env.UDM_HOST_EXE,type:'stdio',allowed_origins:['chrome-extension://'+fs.readFileSync(path.join(project,'browser/extension-id.txt'),'utf8').trim()+'/']}));
 let exists=false;try{execFileSync('reg.exe',['query',registry],{windowsHide:true,stdio:'ignore'});exists=true;}catch{}assert(!exists);execFileSync('reg.exe',['add',registry,'/ve','/t','REG_SZ','/d',hostManifest,'/f'],{windowsHide:true,stdio:'ignore'});registered=true;
 desktop=spawn(process.env.UDM_APP_EXE,['--background','--data-dir',path.dirname(state),'--instance-tag',tag],{windowsHide:true,stdio:'ignore'});
 context=await chromium.launchPersistentContext(path.join(root,'profile'),{channel,headless:true,ignoreDefaultArgs:['--disable-extensions'],env:{...process.env,UDM_INSTANCE_TAG:tag},args:['--enable-unsafe-extension-debugging']});
 const cdp=await context.browser().newBrowserCDPSession();await cdp.send('Browser.setDownloadBehavior',{behavior:'allow',downloadPath:path.join(root,'browser-downloads')});await cdp.send('Extensions.loadUnpacked',{path:ext});worker=context.serviceWorkers()[0]||await context.waitForEvent('serviceworker');
 const identity=await worker.evaluate(()=>nativeRequest({action:'diagnostics'}));assert.equal(identity.version,'0.67.0');assert.equal(path.resolve(identity.dataDirectory).toLowerCase(),path.dirname(state).toLowerCase());pass(channel+' connects to native 0.67.0 with an isolated catalog');
 await worker.evaluate(()=>chrome.storage.local.set({settings:{capture:true,cookies:false,extensions:['udmroute']}}));page=context.pages()[0]||await context.newPage();page.on('requestfailed',request=>requestFailures.push({url:request.url(),failure:request.failure()}));await page.goto(origin+'/page');
 const proxy=server=>({scheme:'http',host:'127.0.0.1',port:server.address().port});
 for(const [label,url,config] of [
  ['A','http://alpha.udm-proxy.invalid/a.udmroute',{mode:'fixed_servers',rules:{singleProxy:proxy(A)}}],
  ['B','http://beta.udm-proxy.invalid/b.udmroute',{mode:'fixed_servers',rules:{proxyForHttp:proxy(B),proxyForHttps:proxy(A)}}],
  ['direct',origin+'/implicit.udmroute',{mode:'fixed_servers',rules:{singleProxy:proxy(A)}}],
  ['direct',origin+'/explicit.udmroute',{mode:'direct'}],
  ['A',origin+'/subtract.udmroute',{mode:'fixed_servers',rules:{singleProxy:proxy(A),bypassList:['<-loopback>']}}],
  ['direct',origin+'/cidr.udmroute',{mode:'fixed_servers',rules:{singleProxy:proxy(A),bypassList:['<-loopback>','127.0.0.0/8']}}],
  ['SOCKS','http://udm-ftp.invalid:'+server.address().port+'/socks.udmroute',{mode:'fixed_servers',rules:{proxyForHttps:proxy(A),fallbackProxy:{scheme:'socks5',host:'127.0.0.1',port:Number(S.settings.address.split(':')[1])}}}],
  ['TLS','http://tls.udm-proxy.invalid/encrypted.udmroute',{mode:'fixed_servers',rules:{singleProxy:{scheme:'https',host:'localhost',port:T.address().port}}}],
  ['TLS',origin+'/encrypted-loopback.udmroute',{mode:'fixed_servers',rules:{singleProxy:{scheme:'https',host:'localhost',port:T.address().port},bypassList:['<-loopback>']}}],
  ['SOCKS4',origin.replace('127.0.0.1','198.51.100.25')+'/socks4.udmroute',{mode:'fixed_servers',rules:{singleProxy:{scheme:'socks4',host:'127.0.0.1',port:Number(S4.settings.address.split(':')[1])}}}],
  ['SOCKS4',origin.replace('127.0.0.1','localhost')+'/socks-localhost.udmroute',{mode:'fixed_servers',rules:{singleProxy:{scheme:'socks4',host:'127.0.0.1',port:Number(S4.settings.address.split(':')[1])},bypassList:['<-loopback>']}}]
 ]){
  await configure(config);await page.goto(origin+'/page');await clickDownload(url);await complete(url,label);
  await until(async()=>{const [d]=await worker.evaluate(url=>chrome.downloads.search({url}),url);return d?.state==='interrupted'&&d.error==='USER_CANCELED';});pass(label+' browser transfer is canceled only after the native acknowledgement');
 }
 assert(S4.metrics.destinations.length>1&&S4.metrics.destinations.every(x=>!x.domain));pass('Both Edge and native SOCKS4 use local DNS and numeric wire destinations');
 assert(seen.filter(x=>x.route==='TLS').length>1&&seen.filter(x=>x.route==='TLS').every(x=>x.encrypted));pass('Encrypted browser proxies stay encrypted on the native requests');
 assert(S.metrics.destinations.length>1&&S.metrics.destinations.every(x=>x.domain&&x.host==='udm-ftp.invalid'));pass('Both Edge and native SOCKS requests preserve proxy-side DNS');
 await configure({mode:'fixed_servers',rules:{singleProxy:proxy(B),bypassList:['<-loopback>']}});await worker.evaluate(()=>chrome.storage.local.set({settings:{capture:true,cookies:true,extensions:['udmroute']}}));const explicit=origin+'/manual.udmroute';
 const answer=await worker.evaluate(async({url,origin})=>{const [tab]=await chrome.tabs.query({url:origin+'/page'});return handoff({url,filename:'manual.udmroute',referrer:tab.url,tabId:tab.id,frameId:0});},{url:explicit,origin});assert(answer.ok);const manual=await complete(explicit,'B');assert(!seen.some(x=>x.url===explicit&&!x.range));assert(manual.ProtectedBrowserSession);assert(seen.filter(x=>x.url===explicit).every(x=>x.hasSession));pass('An explicit link command inherits the browser route and signed-in session before any original network request');
 // The browser executes this PAC itself. UDM must not invent its selected route.
 await configure({mode:'pac_script',pacScript:{data:'function FindProxyForURL(url,host){return "PROXY 127.0.0.1:'+A.address().port+'";}',mandatory:true}});const unsupported='http://pac.udm-proxy.invalid/pac.udmroute';await clickDownload(unsupported);
 await until(async()=>{const [d]=await worker.evaluate(url=>chrome.downloads.search({url}),unsupported);return d?.state==='complete';});assert(!jobs().some(j=>j.Url===unsupported));pass('An unresolved browser PAC route completes in Edge without an incorrect native handoff');
 const storage=JSON.stringify(await worker.evaluate(async()=>({local:await chrome.storage.local.get(null),session:await chrome.storage.session.get(null)})));assert(!storage.includes('"host":"127.0.0.1","port":'));assert(!storage.includes('unrelated-proxy-user'));pass('Routing snapshots and proxy credentials are absent from extension storage');
 assert.equal(jobs().length,12);pass('Exactly twelve native downloads are created; the PAC download stays browser-owned');
})().catch(async e=>{results.push({passed:false,error:e.stack});console.error(e);if(worker)try{fs.writeFileSync(path.join(root,'failure.json'),JSON.stringify(await worker.evaluate(async()=>({context:requestContext.diagnostics(),error:(await chrome.storage.local.get('lastError')).lastError,downloads:(await chrome.downloads.search({})).map(d=>({url:d.url,state:d.state,error:d.error}))})),null,2));}catch{}process.exitCode=1;}).finally(async()=>{
 if(context)await context.close().catch(()=>{});if(desktop){desktop.kill();await new Promise(r=>{desktop.once('close',r);setTimeout(r,5000).unref();});}
 if(S)S.close();if(S4)S4.close();for(const s of sockets)s.destroy();for(const s of [server,A,B,T])if(s)s.close();if(registered)execFileSync('reg.exe',['delete',registry,'/f'],{windowsHide:true,stdio:'ignore'});
 fs.mkdirSync(root,{recursive:true});fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({browser:channel,passed:results.filter(x=>x.passed).length,failed:results.filter(x=>!x.passed).length,results,configurations,requestFailures,traffic:seen},null,2));
});
