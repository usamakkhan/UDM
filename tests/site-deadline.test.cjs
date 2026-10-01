'use strict';
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),crypto=require('node:crypto'),assert=require('node:assert/strict');
const project=process.env.UDM_PROJECT||path.resolve(__dirname,'..'),baseline=process.env.UDM_DEADLINE_BASELINE==='1';let checks=0;
const flush=async()=>{for(let i=0;i<12;i++)await new Promise(r=>setImmediate(r));};
function deferred(){let resolve;const promise=new Promise(r=>resolve=r);return {promise,resolve};}
function fixture(family){
 let clock=1790760000000,serial=0;const timers=new Map(),session={},sent=[],calls=[],gates={},aborts=[];
 class Clock extends Date{static now(){return clock;}}
 const setTimer=(fn,ms)=>{const id=++serial;timers.set(id,{fn,at:clock+ms});return id;},clearTimer=id=>timers.delete(id);
 const advance=async ms=>{const end=clock+ms;for(;;){const row=[...timers].filter(([_,t])=>t.at<=end).sort((a,b)=>a[1].at-b[1].at)[0];if(!row)break;clock=row[1].at;timers.delete(row[0]);row[1].fn();await flush();}clock=end;await flush();};
 const page='https://player.test/watch',base='https://media.test/',master='#EXTM3U\n#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID="a",NAME="English",LANGUAGE="en",DEFAULT=YES,URI="audio.m3u8"\n#EXT-X-STREAM-INF:BANDWIDTH=1000000,RESOLUTION=640x360,CODECS="avc1.42c01e,mp4a.40.2",AUDIO="a"\nvideo.m3u8\n',leaf='#EXTM3U\n#EXTINF:2,\nsegment.ts\n#EXT-X-ENDLIST\n';
 const player={page,token:'test-player',stamp:'1',current:'blob:'+page,sources:[],height:360,width:640,videoCount:1,title:'Deadline fixture',encrypted:false,manifests:[{url:base+'master.m3u8',type:'application/vnd.apple.mpegurl',page,time:clock,text:master}]};
 let reads=0,phase='catalog';const event=()=>({addListener(){}});
 async function gate(name,value){calls.push(name);if(gates[name])await gates[name].promise;return value;}
 const api={storage:{local:{get:()=>gate('settings',{settings:{cookies:true}})},session:{get:async k=>gate('offers',{[k]:session[k]}),set:async v=>Object.assign(session,v),remove:async()=>{}}},tabs:{get:async id=>gate('tab',{id,url:page,incognito:false})},scripting:{executeScript:async()=>{const value=[{frameId:0,documentId:'doc',result:structuredClone(player)}];return gate(phase==='download'&&++reads===2?'final-player':'player',value);}},permissions:{contains:async()=>gate('permission',true),onAdded:event(),onRemoved:event()},cookies:{getAll:()=>gate('cookies',[])},runtime:{onStartup:event(),sendNativeMessage:async(_,m)=>{sent.push(m);return gate('native',{ok:true,id:'native-'+sent.length});}}};
 const sandbox={navigator:{userAgent:'fixture'},UdmMedia:require(path.join(project,'browser',family,'media.js')),URL,Date:Clock,crypto,AbortController,TextEncoder,TextDecoder,setTimeout:setTimer,clearTimeout:clearTimer,
  handoff:async(item,context,ownership,guard)=>{await gate('direct-credentials',null);const send=()=>{sent.push({action:'add',...item});return gate('native',{ok:true,id:'direct'});};return guard?guard(send):send();},
  fetch:async(address,{signal})=>{signal.addEventListener('abort',()=>aborts.push(address),{once:true});await gate('fetch',null);return {ok:true,url:address,body:new ReadableStream({start(c){c.enqueue(new TextEncoder().encode(leaf));c.close();}})};}};
 vm.createContext(sandbox);vm.runInContext(fs.readFileSync(path.join(project,'browser',family,'sites.js'),'utf8')+'\nglobalThis.sites=UdmSites;',sandbox);sandbox.sites.install(api);
 const sender={tab:{id:7},frameId:0,documentId:'doc',url:page},message={page,token:player.token};
 return {sent,calls,gates,aborts,advance,player,timers,jump:ms=>{clock+=ms;},async start({direct=false}={}){
   if(direct){player.current=base+'video.mp4';player.manifests=[];}
   const catalog=await sandbox.sites.list(message,sender),choice=catalog.choices.find(c=>c.container==='mp4')||catalog.choices[0];assert(choice);phase='download';reads=0;
   return ()=>sandbox.sites.download({...message,key:choice.key,...(!direct?{output:'audio'}:{})},sender);
 }};
}
async function check(family,name,fn){await fn();checks++;console.log('PASS '+family+' '+name);}
(async()=>{
 if(baseline){const h=fixture('chromium'),run=await h.start(),gate=h.gates['final-player']=deferred();let result;const pending=run().then(v=>result=v,e=>result={error:e.message});await flush();assert(h.calls.includes('final-player'));await h.advance(65001);assert.equal(result,undefined);gate.resolve();await pending;assert.equal(h.sent.length,1);console.log('CONFIRMED: current site handoff remains pending beyond panel timeout, then submits a late native job');return;}
 for(const family of ['chromium','firefox']){
  for(const step of ['tab','settings','offers','player','final-player','permission','cookies','fetch'])await check(family,'stalled '+step+' ends before panel timeout and cannot submit late',async()=>{
   const h=fixture(family),run=await h.start(),gate=h.gates[step]=deferred();let result;const pending=run().then(v=>result=v,e=>result={error:e.message});await flush();assert(h.calls.includes(step));await h.advance(20001);assert.match(result?.error||'',/No download was sent/);assert.equal(h.sent.length,0);gate.resolve();await pending;await flush();assert.equal(h.sent.length,0);assert.equal(h.timers.size,0);
  });
  await check(family,'late page reply cannot add a second job after a successful retry',async()=>{const h=fixture(family),run=await h.start(),gate=h.gates['final-player']=deferred();const first=run().catch(e=>e);await flush();await h.advance(20001);assert.match((await first).message,/No download was sent/);delete h.gates['final-player'];assert.equal((await run()).ok,true);assert.equal(h.sent.length,1);gate.resolve();await flush();assert.equal(h.sent.length,1);});
  await check(family,'accepted audio plan still includes only selected audio',async()=>{const h=fixture(family),run=await h.start();assert.equal((await run()).ok,true);assert.equal(h.sent.length,1);assert.equal(h.sent[0].plan.tracks.length,1);assert.equal(h.sent[0].plan.tracks[0].kind,'audio');assert.equal(h.timers.size,0);});
  await check(family,'a slow native acknowledgment is uncertain and never automatically retried',async()=>{const h=fixture(family),run=await h.start(),gate=h.gates.native=deferred();let result;const pending=run().then(v=>result=v,e=>result={error:e.message});await flush();assert.equal(h.sent.length,1);await h.advance(40001);assert.match(result?.error||'',/Check its download list/);gate.resolve();await pending;await flush();assert.equal(h.sent.length,1);assert.equal(h.timers.size,0);});
  await check(family,'direct media credentials cannot outlive preflight and submit late',async()=>{const h=fixture(family),run=await h.start({direct:true}),gate=h.gates['direct-credentials']=deferred();let result;const pending=run().then(v=>result=v,e=>result={error:e.message});await flush();await h.advance(20001);assert.match(result?.error||'',/No download was sent/);gate.resolve();await pending;await flush();assert.equal(h.sent.length,0);});
  await check(family,'direct media sends once through the guarded native boundary',async()=>{const h=fixture(family),run=await h.start({direct:true});assert.equal((await run()).ok,true);assert.equal(h.sent.length,1);assert.equal(h.sent[0].action,'add');assert.equal(h.timers.size,0);});
  await check(family,'a delayed timer callback cannot admit an expired preparation',async()=>{const h=fixture(family),run=await h.start(),gate=h.gates['final-player']=deferred();const pending=run().catch(e=>e);await flush();h.jump(20001);gate.resolve();assert.match((await pending).message,/No download was sent/);assert.equal(h.sent.length,0);assert.equal(h.timers.size,0);});
 }
 console.log('ALL '+checks+' SITE DEADLINE CHECKS PASSED');
})().catch(error=>{console.error(error);process.exitCode=1;});
