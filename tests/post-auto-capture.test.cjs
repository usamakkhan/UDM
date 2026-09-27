'use strict';
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
let passed=0;
function harness(browser,options={}){
 const listeners={},calls=[],storage={},body=options.body||'report=monthly&tag=one+two&tag=%E2%9C%93';
 const event=name=>({addListener:f=>(listeners[name]??=[]).push(f)});
 const emit=(name,data)=>Promise.all((listeners[name]||[]).map(fn=>fn(data)));
 let current={id:7,state:options.pausedState||'interrupted',paused:true,error:options.error,bytesReceived:options.bytesReceived};
 const api={
 runtime:{onInstalled:event('installed'),onMessage:event('message'),sendNativeMessage:async(_,m)=>{
   calls.push(m);
   if(m.action==='preferences'){
     if(options.hostUnavailable)throw Error('host unavailable');
     return {ok:true,captureAllowed:options.captureAllowed!==false,postDownloads:true,postBodyLimit:options.limit??1048576,extensions:['udmform']};
   }
   if(m.action==='add'){return {ok:!options.reject,error:options.reject?'Native rejected fixture':undefined};}
   return {ok:true};
 }},
 storage:{local:{get:async()=>({settings:{capture:true,cookies:!!options.cookies,extensions:['udmform']}}),set:async v=>Object.assign(storage,v)},session:{get:async()=>({}),set:async()=>{},remove:async()=>{}}},
 permissions:{contains:async()=>options.permission!==false},cookies:{getAll:async()=>[]},
 tabs:{onRemoved:event('removed'),onUpdated:event('updated')},action:{setBadgeText:async()=>{}},contextMenus:{onClicked:event('context')},
 downloads:{onCreated:event('download'),pause:async id=>{calls.push({action:'pause',id});await emit('error',{requestId:'fixture'});if(options.unpaused)current.paused=false;if(options.missing)current=null;},
 search:async()=>current?[current]:[],resume:async id=>calls.push({action:'resume',id}),
 cancel:async id=>{calls.push({action:'cancel',id});if(options.cancelReject)throw Error('cancel unavailable');current.paused=false;}},
 webRequest:{onBeforeRequest:event('begin'),onBeforeSendHeaders:event('headers'),onHeadersReceived:event('response'),onErrorOccurred:event('error')}
 };
 const ctx=vm.createContext({chrome:api,...(options.firefoxApi?{browser:api}:{}),URL,URLSearchParams,TextEncoder,ArrayBuffer,Uint8Array,btoa:s=>Buffer.from(s,'binary').toString('base64'),navigator:{userAgent:'fixture'},setTimeout,clearTimeout,console,UdmMedia:{policy:{blocked:()=>false,merge:(a,b)=>({...a,...b})}}});
 const folder=path.join(__dirname,'../browser',browser);
 vm.runInContext(fs.readFileSync(path.join(folder,'request-context.js'),'utf8'),ctx);
 vm.runInContext(fs.readFileSync(path.join(folder,'background.js'),'utf8'),ctx);
 const url='https://example.test/report.udmform',referrer='https://example.test/report';
 return {calls,storage,body,run:async()=>{
   const sendRequest=async()=>{
   await emit('begin',{requestId:'fixture',tabId:1,frameId:0,url,method:'POST',documentUrl:referrer,requestBody:{raw:[{bytes:new TextEncoder().encode(body).buffer}]}});
   await emit('headers',{requestId:'fixture',url,requestHeaders:[
    {name:'Content-Type',value:options.multipart?'multipart/form-data; boundary=fixture':'application/x-www-form-urlencoded'},
    {name:'Referer',value:referrer},{name:'Cookie',value:'synthetic=fixture'},{name:'Authorization',value:'Bearer fixture'}]});
   await emit('response',{requestId:'fixture',url,statusCode:200,responseHeaders:[{name:'Content-Disposition',value:'attachment; filename="report.udmform"'}]});
  };
  const item={id:7,url,filename:'report.udmform',referrer,state:'in_progress',incognito:false};
  if(options.unknown)await emit('download',item);
  else if(options.late){
   const running=emit('download',item);
   await new Promise(r=>setTimeout(r,30));
   assert.equal(calls.filter(c=>c.action==='pause').length,0);
   await sendRequest();await running;
  }else{await sendRequest();await emit('download',item);}
   await new Promise(r=>setImmediate(r));
 },add:()=>calls.find(c=>c.action==='add'),count:action=>calls.filter(c=>c.action===action).length};
}
async function check(browser,name,options,verify){const h=harness(browser,options);await h.run();verify(h);passed++;console.log('PASS '+browser+': '+name);}
(async()=>{
 for(const browser of ['chromium','firefox']){
  for(const pausedState of ['in_progress','interrupted'])await check(browser,'Paused '+pausedState+' retains exact POST after request abort',{pausedState},h=>{
   assert.equal(h.count('add'),1);assert.equal(h.add().request.method,'POST');assert.equal(Buffer.from(h.add().request.body,'base64').toString(),h.body);
   assert.equal(h.count('cancel'),1);assert.equal(h.count('resume'),0);
   assert(h.calls.findIndex(c=>c.action==='add')<h.calls.findIndex(c=>c.action==='cancel'));
  });
  await check(browser,'Download announced before webRequest waits for the matching POST',{late:true},h=>{
   assert.equal(h.count('add'),1);assert.equal(h.add().request.method,'POST');assert.equal(Buffer.from(h.add().request.body,'base64').toString(),h.body);
  });
  await check(browser,'Unknown request is never guessed to be GET',{unknown:true},h=>{
   assert.equal(h.count('pause'),0);assert.equal(h.count('add'),0);
  });
  await check(browser,'Unsupported multipart never pauses or replays',{multipart:true},h=>{assert.equal(h.count('pause'),0);assert.equal(h.count('add'),0);assert.match(h.storage.lastError,/remains in the browser/);});
  await check(browser,'Oversized request keeps the original browser transfer',{body:'x'.repeat(1048577)},h=>{assert.equal(h.count('pause'),0);assert.equal(h.count('add'),0);});
  await check(browser,'Native refusal releases paused browser download',{reject:true},h=>{assert.equal(h.count('add'),1);assert.equal(h.count('cancel'),0);assert.equal(h.count('resume'),1);});
  await check(browser,'Desktop capture disabled releases paused download',{captureAllowed:false},h=>{assert.equal(h.count('add'),0);assert.equal(h.count('resume'),1);});
  await check(browser,'Unavailable native host releases paused download',{hostUnavailable:true},h=>{assert.equal(h.count('add'),0);assert.equal(h.count('resume'),1);});
  await check(browser,'Older desktop cannot turn larger POST into GET',{body:'x'.repeat(65537),limit:65536},h=>{assert.equal(h.count('add'),0);assert.equal(h.count('resume'),1);});
  await check(browser,'Unpaused browser download is not duplicated',{unpaused:true},h=>{assert.equal(h.count('add'),0);assert.equal(h.count('resume'),0);});
  await check(browser,'Firefox pause before payload still hands off the saved POST',{firefoxApi:true,unpaused:true,error:'USER_CANCELED',bytesReceived:0},h=>{
   assert.equal(h.count('add'),1);assert.equal(h.count('cancel'),1);assert.equal(h.count('resume'),0);
  });
  await check(browser,'Chromium cancellation is not mistaken for Firefox zero-byte pause',{unpaused:true,error:'USER_CANCELED',bytesReceived:0},h=>{
   assert.equal(h.count('add'),0);assert.equal(h.count('cancel'),0);
  });
  await check(browser,'Removed browser download is not handed off',{missing:true},h=>{assert.equal(h.count('add'),0);assert.equal(h.count('cancel'),0);});
  await check(browser,'Unexpected paused state is released',{pausedState:'complete'},h=>{assert.equal(h.count('add'),0);assert.equal(h.count('resume'),1);});
  await check(browser,'Capture snapshot respects disabled credential sharing',{},h=>{assert.equal(h.add().headers.Cookie,undefined);assert.equal(h.add().headers.Authorization,undefined);assert.equal(h.add().cookies,'');});
  await check(browser,'Capture snapshot respects denied credential permission',{cookies:true,permission:false},h=>{assert.equal(h.add().headers.Cookie,undefined);assert.equal(h.add().headers.Authorization,undefined);assert.equal(h.add().cookies,'');});
  await check(browser,'Permitted credentials survive aborted browser request',{cookies:true},h=>{assert.equal(h.add().headers.Cookie,'synthetic=fixture');assert.equal(h.add().headers.Authorization,'Bearer fixture');assert.equal(h.add().cookies,'synthetic=fixture');});
  await check(browser,'Failed browser cancel does not replay an accepted job',{cancelReject:true},h=>{assert.equal(h.count('add'),1);assert.equal(h.count('resume'),0);assert.match(h.storage.lastError,/browser could not cancel/);});
  await check(browser,'Request body is never written into extension storage',{},h=>{assert(!JSON.stringify(h.storage).includes(Buffer.from(h.body).toString('base64')));});
 }
 console.log('ALL '+passed+' AUTOMATIC CAPTURE CHECKS PASSED');
})().catch(e=>{console.error(e);process.exitCode=1;});

