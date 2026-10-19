'use strict';
// Diagnostic only: measures hello latency without opening the desktop or submitting a download.
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const {execFileSync}=require('node:child_process');
const {chromium}=require('playwright');
const root=path.resolve(process.argv[2]),channel=process.env.UDM_TEST_BROWSER||'msedge';
const project=path.resolve(__dirname,'..'),host=process.env.UDM_HOST_EXE;
const name='com.udm.handshakefixture'+Date.now();
const registry='HKCU\\Software\\'+(channel==='msedge'?'Microsoft\\Edge':'Google\\Chrome')+'\\NativeMessagingHosts\\'+name;
let context,registered=false;
const results={browser:channel,diagnosticDeadlineMs:15000,productionHelloDeadlineMs:15000,trials:[]};
(async()=>{
 assert(host&&fs.existsSync(host));assert(!fs.existsSync(root),'Use a fresh fixture directory');
 const extension=path.join(root,'extension');fs.mkdirSync(extension,{recursive:true});
 const original=JSON.parse(fs.readFileSync(path.join(project,'browser/chromium/manifest.json')));
 fs.writeFileSync(path.join(extension,'manifest.json'),JSON.stringify({manifest_version:3,name:'UDM isolated native handshake diagnostic',version:'1.0.0',key:original.key,permissions:['nativeMessaging'],background:{service_worker:'worker.js'}}));
 fs.writeFileSync(path.join(extension,'worker.js'),'chrome.runtime.onInstalled.addListener(()=>{});');
 const manifest=path.join(root,'host.json');
 fs.writeFileSync(manifest,JSON.stringify({name,description:'UDM isolated hello diagnostic',path:host,type:'stdio',allowed_origins:['chrome-extension://'+fs.readFileSync(path.join(project,'browser/extension-id.txt'),'utf8').trim()+'/']}));
 let exists=false;try{execFileSync('reg.exe',['query',registry],{windowsHide:true,stdio:'ignore'});exists=true;}catch{}assert(!exists);
 execFileSync('reg.exe',['add',registry,'/ve','/t','REG_SZ','/d',manifest,'/f'],{windowsHide:true,stdio:'ignore'});registered=true;
 context=await chromium.launchPersistentContext(path.join(root,'profile'),{channel,headless:true,ignoreDefaultArgs:['--disable-extensions'],args:['--enable-unsafe-extension-debugging']});
 const cdp=await context.browser().newBrowserCDPSession();await cdp.send('Extensions.loadUnpacked',{path:extension});
 const worker=context.serviceWorkers()[0]||await context.waitForEvent('serviceworker');
 results.browserVersion=context.browser().version();
 for(let trial=0;trial<3;trial++){
  const result=await worker.evaluate(({name,trial})=>new Promise(resolve=>{
   const started=performance.now(),port=chrome.runtime.connectNative(name);let done=false;
   const finish=value=>{if(done)return;done=true;clearTimeout(timer);const elapsedMs=Math.round(performance.now()-started);port.disconnect();resolve({...value,elapsedMs});};
   const timer=setTimeout(()=>finish({ok:false,error:'Diagnostic hello deadline exceeded'}),15000);
   port.onMessage.addListener(reply=>finish({ok:reply.ok===true&&reply.protocol===1&&reply.requestId===trial+1,protocol:reply.protocol,version:reply.version}));
   port.onDisconnect.addListener(()=>finish({ok:false,error:chrome.runtime.lastError?.message||'Native host disconnected'}));
   port.postMessage({action:'hello',protocol:1,requestId:trial+1});
  }),{name,trial});
  results.trials.push(result);console.log(JSON.stringify({trial,...result}));
 }
 results.withinProductionDeadline=results.trials.every(t=>t.ok&&t.elapsedMs<results.productionHelloDeadlineMs);
 if(!results.withinProductionDeadline)process.exitCode=1;
})().catch(error=>{results.error=error.message;console.error(error.message);process.exitCode=1;}).finally(async()=>{
 if(context)await context.close().catch(()=>{});
 if(registered)execFileSync('reg.exe',['delete',registry,'/f'],{windowsHide:true,stdio:'ignore'});
 fs.mkdirSync(root,{recursive:true});fs.writeFileSync(path.join(root,'results.json'),JSON.stringify(results,null,2));
});
