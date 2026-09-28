'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),crypto=require('node:crypto');
const base=path.resolve(__dirname,'../browser/chromium'),Navigation=require('../browser/chromium/navigation.js');
let checks=0;const pass=name=>{checks++;console.log('PASS '+name);};
const pause=()=>new Promise(r=>setImmediate(r));
function deferred(){let resolve;return {promise:new Promise(r=>resolve=r),resolve};}
function harness({supported=true,stored={}}={}){
 const listeners={},state={incognito:false,documentId:'old',page:'https://player.test/watch',frameId:0,videoId:'Q3TI27IN7X0',timeOrigin:Date.now()-10000};
 const event=name=>({addListener:fn=>(listeners[name]??=[]).push(fn)});
 const emit=async(name,...args)=>{for(const fn of listeners[name]||[])await fn(...args);};
 const api={
  storage:{local:{get:async()=>({settings:{}}),set:async()=>{}},session:{
   get:async keys=>{if(state.getGate)await state.getGate;return keys==null?{...stored}:Object.fromEntries([].concat(keys).map(k=>[k,structuredClone(stored[k])]));},
   set:async values=>{Object.assign(stored,structuredClone(values));},remove:async keys=>{for(const k of [].concat(keys))delete stored[k];}}},
  tabs:{get:async id=>{if(state.tabGate)await state.tabGate;return {id,url:state.page,incognito:state.incognito};},onRemoved:event('removed'),onUpdated:event('updated')},
  webRequest:{onBeforeRequest:event('before'),onBeforeSendHeaders:event('sent'),onHeadersReceived:event('response'),onErrorOccurred:event('error')},
  ...(supported?{webNavigation:{onCommitted:event('commit'),onHistoryStateUpdated:event('history'),onReferenceFragmentUpdated:event('fragment'),onTabReplaced:event('replaced')}}:{}),
  permissions:{contains:async()=>true,onAdded:event('permission'),onRemoved:event('permissionRemoved')},
  runtime:{id:'test',onInstalled:event('installed'),onStartup:event('startup'),onMessage:event('message'),sendNativeMessage:async()=>({ok:true})},
  contextMenus:{onClicked:event('menu'),removeAll:async()=>{},create:()=>{}},
  downloads:{onCreated:event('download')},action:{setBadgeText:async()=>{}},
  scripting:{executeScript:async args=>{if(state.proofGate)await state.proofGate;return [{frameId:state.frameId,documentId:state.documentId,result:args.func.name==='player'?{videoId:state.videoId,page:state.page,timeOrigin:state.timeOrigin}:{page:state.page,timeOrigin:state.timeOrigin,token:'token-1',stamp:'1',current:'https://cdn.test/movie.mp4',sources:[],height:720,width:1280,videoCount:1,title:'Movie',encrypted:false}}];}}
 };
 const sandbox=vm.createContext({chrome:api,URL,URLSearchParams,TextEncoder,TextDecoder,Uint8Array,ArrayBuffer,btoa,crypto,Date,console,setTimeout,clearTimeout,navigator:{userAgent:'test'}});
 for(const file of ['navigation','media','formats','request-context','sites','ump','streaming-capture','background'])vm.runInContext(fs.readFileSync(path.join(base,file+'.js'),'utf8'),sandbox);
 const modules=vm.runInContext('({nav:captureNavigation,sites:UdmSites,request:requestContext,stream:UdmStreamingCapture})',sandbox);
 const commit=async(documentId,frameId=0,parentFrameId=-1)=>{await emit('commit',{tabId:7,frameId,parentFrameId,documentId});await modules.nav.settled();};
 const capture=(documentId,frameId=0,url='https://cdn.test/master.m3u8',mime='application/vnd.apple.mpegurl')=>emit('response',{requestId:crypto.randomUUID(),tabId:7,frameId,documentId,documentUrl:state.page,url,statusCode:200,responseHeaders:[{name:'Content-Type',value:mime}]});
 const body=Uint8Array.of(42,1,1,154,1,2,8,1);
 const request=(documentId,extra={})=>({requestId:crypto.randomUUID(),tabId:7,frameId:0,documentId,documentUrl:state.page,url:'https://cdn.test/download.zip',method:'GET',...extra});
 const sabr=async(documentId)=>{
  state.documentId=documentId;const r=request(documentId,{method:'POST',url:'https://rr1.googlevideo.com/videoplayback?sabr=1',requestBody:{raw:[{bytes:body.buffer}]}});
  await emit('before',r);await modules.stream.response({...r,statusCode:200,responseHeaders:[{name:'Content-Type',value:'application/vnd.yt-ump'}]});
 };
 return {...modules,api,stored,state,emit,commit,capture,requestEvent:request,sabr};
}
(async()=>{
 // Reproduce the old sequence using the still-supported legacy behavior, then the fix.
 let h=harness({supported:false});await h.capture('new');assert.equal(h.stored['site-media:7'].length,1);
 await h.emit('updated',7,{url:'https://player.test/new'});await pause();
 assert.equal(h.stored['site-media:7'],undefined);pass('Reproduces late URL notification deleting a newly captured playlist with legacy cleanup');

 h=harness();await h.commit('old');await h.capture('old');await h.capture('new');
 await h.emit('updated',7,{url:'https://player.test/new'});await h.commit('new');
 assert.deepEqual(h.stored['site-media:7'].map(x=>x.documentId),['new']);pass('New playlist survives capture-before-commit and late URL notification');
 await h.capture('old');assert.deepEqual(h.stored['site-media:7'].map(x=>x.documentId),['new']);pass('Late responses from retired documents cannot restore stale playlists');
 await h.capture('new',0,'https://cdn.test/movie.mp4','video/mp4');await h.emit('updated',7,{url:'https://player.test/new'});
 assert.equal(h.stored['media:7'][0].documentId,'new');pass('Raw media survives a delayed tab URL notification');

 await h.commit('child-old',2,0);await h.commit('sibling',3,0);await h.capture('child-old',2);await h.capture('sibling',3);await h.capture('child-new',2);await h.commit('child-new',2,0);
 assert.deepEqual(h.stored['site-media:7'].map(x=>x.documentId).sort(),['child-new','new','new','sibling'].sort());pass('Iframe replacement retires only that frame and preserves the sibling');
 await h.capture('next-top');await h.commit('next-top');
 assert.deepEqual(h.stored['site-media:7'].map(x=>x.documentId),['next-top']);pass('Top document replacement removes its prior iframe descendants');

 await h.commit('new');await h.capture('new');assert(h.stored['site-media:7'].some(x=>x.documentId==='new'));pass('Back-forward restoration makes the restored document valid again');
 const restarted=harness({stored:h.stored});await restarted.nav.ready;assert(!restarted.nav.valid({tabId:7,documentId:'next-top'}));
 await restarted.commit('after-restart');assert(!restarted.nav.valid({tabId:7,documentId:'new'}));pass('Service worker restart preserves active and retired document identity');
 assert(!JSON.stringify(h.stored['udm-navigation']).includes('https:'));pass('Lifecycle persistence contains no URLs, request bodies or credentials');

 h=harness();await h.commit('old');await h.capture('old');await h.sabr('old');
 const r=h.requestEvent('old');h.request.begin(r);h.request.headers({...r,requestHeaders:[{name:'Authorization',value:'Bearer test-only'},{name:'Cookie',value:'fixture=1'}]});
 h.request.finish({...r,statusCode:200});
 await h.emit('updated',7,{url:'https://player.test/new'});
 assert.equal(h.request.resolve({...r},true).headers.Authorization,'Bearer test-only');assert.equal(h.stored['sabr:7'].length,1);pass('URL notifications preserve current streaming body and authenticated request context');
 const newer=h.requestEvent('new',{requestId:'post-new',method:'POST',requestBody:{raw:[{bytes:Uint8Array.of(1,2,3).buffer}]}});
 h.request.begin(newer);h.request.headers({...newer,requestHeaders:[{name:'Content-Type',value:'application/octet-stream'},{name:'Content-Length',value:'3'}]});h.request.finish({...newer,statusCode:200,responseHeaders:[{name:'Content-Disposition',value:'attachment'}]});
 await h.sabr('new');await h.commit('new');
 assert.equal(h.stored['sabr:7'].length,1);assert.equal(h.stored['sabr:7'][0].documentId,'new');
 assert.equal(h.request.resolve(r,true),null);assert.equal(h.request.resolve({...newer,browserDownload:true},true).request.body,'AQID');pass('Document replacement preserves fresh POST bytes and SABR while retiring old credentials');
 assert.equal(h.request.resolve(newer,false).headers.Authorization,undefined);pass('Navigation does not grant credential sharing');

 h=harness();await h.commit('old');const gate=deferred();h.state.proofGate=gate.promise;
 const late=h.sabr('old');await pause();await h.commit('new');gate.resolve();await late;
 assert.equal(h.stored['sabr:7'],undefined);pass('Delayed old-player proof cannot resurrect streaming capture after document replacement');

 h=harness();await h.commit('old');const hold=deferred();h.state.getGate=hold.promise;
 const delayed=h.capture('old');await pause();const closing=h.nav.notify('removed',{tabId:7});await pause();hold.resolve();await Promise.all([delayed,closing]);await h.capture('old');
 assert.equal(h.stored['site-media:7'],undefined);assert.equal(h.stored['media:7'],undefined);pass('Tab close drains pending writes and rejects later responses');

 h=harness();await h.commit('old');const sender={tab:{id:7},frameId:0,documentId:'old',url:h.state.page},message={page:h.state.page,token:'token-1'};
 assert.equal((await h.sites.list(message,sender)).choices.length,1);assert.equal(h.stored['site-offers:7'][0].documentId,'old');
 await h.commit('new');assert.equal(h.stored['site-offers:7'].length,0);pass('Stored menu offers carry document identity and retire with their page');
 await assert.rejects(h.sites.download({...message,key:'old-key'},sender),/changed/);pass('Retired menu selections cannot create a download');

 h=harness();await h.commit('old');await h.capture('old');await h.commit('child',4,0);await h.capture('child',4);
 const token=h.nav.token(7,4);await h.emit('history',{tabId:7,frameId:0,documentId:'old'});await h.nav.settled();
 assert.equal(h.stored['site-media:7'].length,0);assert(!h.nav.valid({tabId:7,frameId:4,documentId:'child'},token));pass('Same-document route changes invalidate prior captures and descendant in-flight work');
 await h.capture('old');await h.emit('history',{tabId:7,frameId:0,documentId:'retired-other'});await h.nav.settled();
 assert.equal(h.stored['site-media:7'].length,1);pass('Stale route events from another document do not clear the current page');
 await h.emit('fragment',{tabId:7,frameId:0,documentId:'old'});await h.nav.settled();assert.equal(h.stored['site-media:7'].length,0);pass('Fragment route changes retain conservative stale-media invalidation');

 h=harness();h.state.incognito=true;await h.commit('private');await h.capture('private');await h.sabr('private');
 assert.equal(h.stored['site-media:7'],undefined);assert.equal(h.stored['sabr:7'],undefined);assert.equal(h.stored['udm-navigation'],undefined);pass('Private windows do not persist navigation or media captures');

 h=harness();await h.commit('old');h.nav.subscribe(()=>{throw Error('fixture storage failure');});await h.commit('new');await h.capture('new');await h.commit('third');
 assert(!h.nav.valid({tabId:7,documentId:'new'}));pass('One failing cleanup subscriber cannot block later navigation events');
 await h.emit('replaced',{replacedTabId:7,tabId:9});await h.nav.settled();assert.equal(h.stored['site-media:7'],undefined);pass('Replaced tab cleanup removes its saved captures');

 h=harness();await h.commit('old');await h.capture('old');await h.commit(undefined);assert.equal(h.stored['site-media:7'].length,0);pass('Browsers without document IDs use conservative committed-navigation cleanup');
 const saved={frames:[{tabId:7,frameId:0,documentId:'long-running',parentFrameId:-1,time:Date.now()-86400000}],retired:[]};
 h=harness({stored:{'udm-navigation':saved}});await h.nav.ready;await h.commit('new');assert(!h.nav.valid({tabId:7,documentId:'long-running'}));pass('Long-playing pages retain identity until navigation, without a three-minute expiry');

 const store={};const ev={addListener(){}};const api={webNavigation:{onCommitted:ev},tabs:{get:async()=>({incognito:false}),onRemoved:ev},storage:{session:{get:async()=>store,set:async x=>Object.assign(store,x)}}};
 const nav=Navigation.create(api);for(let tabId=1;tabId<=520;tabId++)await nav.notify('commit',{tabId,frameId:0,documentId:'doc-'+tabId});
 assert.equal(store['udm-navigation'].frames.length,512);pass('Persisted frame identities are bounded');
 console.log('ALL '+checks+' NAVIGATION REGRESSIONS PASSED');
})().catch(e=>{console.error(e);process.exitCode=1;});
