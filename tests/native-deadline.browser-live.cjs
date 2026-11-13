'use strict';
// Private Edge/Chrome profile and unique native registration; static hello only.
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict'),crypto=require('node:crypto'),{execFileSync}=require('node:child_process'),{chromium}=require('playwright');
const root=path.resolve(process.argv[2]),project=path.resolve(__dirname,'..'),channel=process.env.UDM_TEST_BROWSER||'msedge';assert(['msedge','chrome'].includes(channel));
const build=JSON.parse(fs.readFileSync(path.join(project,'docs/validation/native-deadlines-7.60-build.json'))),host=path.join(project,build.output,'Udm.NativeHost.exe');
const sha=file=>crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex').toUpperCase();assert.equal(sha(host),build.binaries.find(x=>x.name==='Udm.NativeHost.exe').sha256);
const name='com.udm.deadlinefixture'+Date.now(),registry='HKCU\\Software\\'+(channel==='msedge'?'Microsoft\\Edge':'Google\\Chrome')+'\\NativeMessagingHosts\\'+name;
let context,registered=false;const results={scope:'Fresh browser profile, actual native host, copied current bridge. Only static hello commands. Validation and legacy acknowledgement stalls are deliberately injected around actual browser/native APIs; no download or personal-browser acceptance.',desktop:build.version,browser:channel,hostSha256:sha(host),checks:[],registrationRemoved:false};
(async()=>{
 assert(!fs.existsSync(root),'Fresh fixture root required');const extension=path.join(root,'extension');fs.mkdirSync(extension,{recursive:true});
 const original=JSON.parse(fs.readFileSync(path.join(project,'browser/chromium/manifest.json')));
 fs.writeFileSync(path.join(extension,'manifest.json'),JSON.stringify({manifest_version:3,name:'UDM isolated request lifetime fixture',version:'1.0.0',key:original.key,permissions:['nativeMessaging'],background:{service_worker:'worker.js'}}));
 const source=path.join(project,'browser/chromium/native-bridge.js');fs.copyFileSync(source,path.join(extension,'native-bridge.js'));results.bridgeSha256=sha(source);
 fs.writeFileSync(path.join(extension,'worker.js'),'importScripts("native-bridge.js");chrome.runtime.onInstalled.addListener(()=>{});');
 const manifest=path.join(root,'host.json');fs.writeFileSync(manifest,JSON.stringify({name,description:'UDM owned static native request fixture',path:host,type:'stdio',allowed_origins:['chrome-extension://'+fs.readFileSync(path.join(project,'browser/extension-id.txt'),'utf8').trim()+'/']}));
 let exists=false;try{execFileSync('reg.exe',['query',registry],{windowsHide:true,stdio:'ignore'});exists=true;}catch{}assert(!exists,'Unique fixture registration required');
 execFileSync('reg.exe',['add',registry,'/ve','/t','REG_SZ','/d',manifest,'/f'],{windowsHide:true,stdio:'ignore'});registered=true;
 context=await chromium.launchPersistentContext(path.join(root,'profile'),{channel,headless:true,ignoreDefaultArgs:['--disable-extensions'],args:['--enable-unsafe-extension-debugging']});
 const cdp=await context.browser().newBrowserCDPSession();await cdp.send('Extensions.loadUnpacked',{path:extension});
 const worker=context.serviceWorkers()[0]||await context.waitForEvent('serviceworker',{timeout:15000});results.browserVersion=context.browser().version();
 for(const mode of ['healthy','validation','disconnect_validation','legacy_ack','disconnect_legacy','before_send_disconnect']){
  const result=await worker.evaluate(async({name,mode})=>{
   const wire=[],oneShot=[],versions=[];let releaseValidation,releaseAck,enteredResolve,ackResolve;
   const entered=new Promise(resolve=>{enteredResolve=resolve;}),ackSeen=new Promise(resolve=>{ackResolve=resolve;});
   const legacy=mode==='legacy_ack'||mode==='disconnect_legacy';
   const api={runtime:{get lastError(){return chrome.runtime.lastError;},connectNative(host){
    const port=chrome.runtime.connectNative(host);
    return {onMessage:{addListener(fn){port.onMessage.addListener(reply=>{versions.push(reply.version);fn(legacy?{ok:false}:reply);});}},onDisconnect:port.onDisconnect,disconnect(){port.disconnect();},postMessage(message){wire.push({action:message.action,requestId:message.requestId});port.postMessage(message);}};
   },async sendNativeMessage(host,message){oneShot.push({action:message.action});const reply=await chrome.runtime.sendNativeMessage(host,message);versions.push(reply.version);ackResolve();return new Promise(resolve=>{releaseAck=()=>resolve(reply);});}}};
   const client=UdmNativeBridge.create(api,name);let sent=0;
   const timerRace=(promise,ms=15000)=>{let timer;return Promise.race([promise,new Promise((_,reject)=>{timer=setTimeout(()=>reject(Error('Fixture wait timed out')),ms);})]).finally(()=>clearTimeout(timer));};
   try{
    if(!legacy)await timerRace(client.warmControl());
    const validate=mode==='validation'||mode==='disconnect_validation'?()=>{enteredResolve();return new Promise(resolve=>{releaseValidation=resolve;});}:undefined;
    const before=()=>{sent++;if(mode==='before_send_disconnect')client.disconnect();};
    const started=performance.now();
    const outcome=client.request({action:'hello',probe:'owned-lifetime'},mode==='disconnect_validation'||mode==='disconnect_legacy'?4000:1000,before,validate)
     .then(reply=>({ok:true,version:reply.version}),error=>({ok:false,message:error.message,beforeSend:error.nativeBeforeSend}));
    if(mode==='disconnect_validation'){await timerRace(entered);client.disconnect();}
    if(mode==='disconnect_legacy'){await timerRace(ackSeen);client.disconnect();}
    const value=await timerRace(outcome,6000);const elapsedMs=performance.now()-started;
    releaseValidation?.();releaseAck?.();await new Promise(resolve=>setTimeout(resolve,50));
    return {mode,...value,elapsedMs,sent,wire,oneShot,versions,diagnostics:client.diagnostics()};
   }finally{releaseValidation?.();releaseAck?.();client.disconnect();}
  },{name,mode});
  assert(result.versions.every(version=>version===build.version),'Actual host identity');
  if(mode==='healthy'){assert.equal(result.ok,true);assert.equal(result.version,build.version);assert.equal(result.sent,1);assert.equal(result.wire.length,2);}
  else{
   assert.equal(result.ok,false);assert.equal(result.diagnostics.pending,0);
   if(mode==='validation'){assert.match(result.message,/No download request was sent/);assert.equal(result.sent,0);assert.equal(result.wire.length,1);}
   if(mode==='disconnect_validation'){assert.match(result.message,/connection closed/);assert.equal(result.sent,0);assert.equal(result.wire.length,1);}
   if(mode==='before_send_disconnect'){assert.match(result.message,/connection closed/);assert.equal(result.wire.length,1);}
   if(mode==='legacy_ack'||mode==='disconnect_legacy'){assert.equal(result.sent,1);assert.equal(result.oneShot.length,1);assert.equal(result.wire.length,1);assert.match(result.message,mode==='legacy_ack'?/download list/:/connection closed/);}
  }
  results.checks.push({...result,passed:true});console.log(JSON.stringify({mode,passed:true,elapsedMs:result.elapsedMs}));
 }
 assert.equal(sha(path.join(extension,'native-bridge.js')),results.bridgeSha256);assert.equal(sha(host),results.hostSha256);results.passed=true;
})().catch(error=>{results.passed=false;results.error=error.stack;console.error(error.stack);process.exitCode=1;}).finally(async()=>{
 if(context)await context.close().catch(error=>{results.closeError=error.message;});
 if(registered){execFileSync('reg.exe',['delete',registry,'/f'],{windowsHide:true,stdio:'ignore'});try{execFileSync('reg.exe',['query',registry],{windowsHide:true,stdio:'ignore'});}catch{results.registrationRemoved=true;}}
 fs.mkdirSync(root,{recursive:true});fs.writeFileSync(path.join(root,'results.json'),JSON.stringify(results,null,2));
});
