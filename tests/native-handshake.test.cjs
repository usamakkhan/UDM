'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm');
const variant=process.env.UDM_TEST_NATIVE_VARIANT||'chromium';assert(['chromium','firefox'].includes(variant));
const source=fs.readFileSync(require.resolve('../browser/'+variant+'/native-bridge.js'),'utf8');
function fixture({helloDelay=0,replyDelay=0,legacy=false,legacyDelay=0,persistent=true}={}){
 let now=0,next=0;const timers=new Map(),ports=[],oneShot=[],legacyReplies=[];
 const later=(fn,delay)=>{const id=++next;timers.set(id,{at:now+delay,fn});return id;};
 const api={runtime:{connectNative(){
  const messages=[],listeners=[],disconnects=[];
  const port={messages,closed:false,onMessage:{addListener:f=>listeners.push(f)},onDisconnect:{addListener:f=>disconnects.push(f)},
   disconnect(){this.closed=true;},postMessage(message){messages.push(message);const hello=message.action==='hello',delay=hello?helloDelay:replyDelay;
    if(delay!==null)later(()=>{if(!port.closed)for(const f of listeners)f(hello&&legacy?{ok:false}:{ok:true,protocol:1,requestId:message.requestId});},delay);
   },drop(){this.closed=true;for(const f of disconnects)f();}};ports.push(port);return port;
 },sendNativeMessage(_,message){if(!legacy)throw Error('Unexpected legacy replay');oneShot.push(message);return new Promise(resolve=>{legacyReplies.push(resolve);if(legacyDelay!==null)later(()=>resolve({ok:true}),legacyDelay);});}}};
 if(!persistent)api.runtime.connectNative=undefined;
 const context=vm.createContext({setTimeout:later,clearTimeout:id=>timers.delete(id)});vm.runInContext(source,context);
 const client=context.UdmNativeBridge.create(api,'fixture');
 async function flush(){for(let i=0;i<24;i++)await Promise.resolve();}
 async function tick(ms){const until=now+ms;await flush();for(;;){const due=[...timers].filter(([,v])=>v.at<=until).sort((a,b)=>a[1].at-b[1].at)[0];if(!due)break;now=due[1].at;timers.delete(due[0]);due[1].fn();await flush();}now=until;await flush();}
 function request(action,timeout,beforeSend,validateSelection){const result={state:'pending'};client.request({action},timeout,beforeSend,validateSelection).then(value=>Object.assign(result,{state:'resolved',value}),error=>Object.assign(result,{state:'rejected',error}));return result;}
 return {client,ports,tick,request,timers,oneShot,legacyReplies};
}
async function runHandshakeChecks(){
 {
  const h=fixture({helloDelay:10,replyDelay:1}),result=h.request('add');await h.tick(11);
  assert.equal(result.state,'resolved');assert.deepEqual(h.ports[0].messages.map(m=>m.action),['hello','add']);assert.equal(h.timers.size,0);
  h.client.disconnect();console.log('PASS responsive startup sends the download immediately after hello');
 }
 {
  const h=fixture({helloDelay:8000}),result=h.request('add');await h.tick(5001);
  assert.equal(result.state,'pending');assert.deepEqual(h.ports[0].messages.map(m=>m.action),['hello']);
  await h.tick(2999);assert.equal(result.state,'resolved');assert.equal(h.ports.length,1);assert.deepEqual(h.ports[0].messages.map(m=>m.action),['hello','add']);
  h.client.disconnect();console.log('PASS slow native startup preserves the original connection and sends one download');
 }
 {
  const h=fixture({helloDelay:8000}),first=h.request('add'),second=h.request('media');await h.tick(8000);
  assert.equal(first.state,'resolved');assert.equal(second.state,'resolved');assert.equal(h.ports.length,1);assert.deepEqual(h.ports[0].messages.map(m=>m.action),['hello','add','media']);
  h.client.disconnect();console.log('PASS concurrent commands share startup without duplicate admissions');
 }
 {
  const h=fixture({helloDelay:null}),result=h.request('add');await h.tick(14999);assert.equal(result.state,'pending');await h.tick(1);
  assert.equal(result.state,'rejected');assert.match(result.error.message,/connect/i);assert.match(result.error.message,/No download request was sent/);assert.equal(h.ports[0].closed,true);assert.equal(h.timers.size,0);assert.deepEqual(h.ports[0].messages.map(m=>m.action),['hello']);
  console.log('PASS missing hello stops at a bounded deadline without submitting a download');
 }
 {
  const h=fixture({replyDelay:null}),result=h.request('add',1000);await h.tick(1000);
  assert.equal(result.state,'rejected');assert.match(result.error.message,/download list/);assert.equal(h.ports.length,1);assert.deepEqual(h.ports[0].messages.map(m=>m.action),['hello','add']);assert.equal(h.timers.size,0);
  console.log('PASS lost download acknowledgement is never replayed');
 }
 {
  const h=fixture({helloDelay:10}),result=h.request('add');h.ports[0].drop();await h.tick(10);
  assert.equal(result.state,'resolved');assert.deepEqual(h.ports.map(port=>port.messages.map(m=>m.action)),[['hello'],['hello','add']]);
  h.client.disconnect();console.log('PASS disconnect before dispatch reconnects and sends one download');
 }
 {
  const h=fixture({replyDelay:null}),result=h.request('add');await h.tick(0);h.ports[0].drop();await h.tick(0);
  assert.equal(result.state,'rejected');assert.deepEqual(h.ports.map(port=>port.messages.map(m=>m.action)),[['hello','add']]);
  console.log('PASS disconnect after dispatch does not repeat the download');
 }
 console.log('ALL 7 NATIVE HANDSHAKE CHECKS PASSED');return 7;
}
module.exports=runHandshakeChecks;
async function runDeadlineChecks(){
 let count=0;const only=process.env.UDM_HANDSHAKE_CASE;
 const test=async(name,action)=>{if(only&&only!==name)return;await action();count++;console.log('PASS '+name);};
 await test('legacy_ack',async()=>{
  const h=fixture({legacy:true,legacyDelay:null}),r=h.request('add',1000);await h.tick(1000);
  assert.equal(r.state,'rejected');assert.match(r.error.message,/download list/);assert.equal(r.error.nativeBeforeSend,false);assert.equal(h.oneShot.length,1);
  h.legacyReplies[0]({ok:true});await h.tick(0);assert.equal(r.state,'rejected');assert.equal(h.oneShot.length,1);h.client.disconnect();assert.equal(h.timers.size,0);
 });
 await test('persistent_validation',async()=>{
  const h=fixture();let release,sent=0;const r=h.request('media',1000,()=>sent++,()=>new Promise(resolve=>{release=resolve;}));await h.tick(1000);
  assert.equal(r.state,'rejected');assert.match(r.error.message,/No download request was sent/);assert.equal(sent,0);assert.equal(h.ports[0].closed,false);
  const next=h.request('add');await h.tick(0);assert.equal(next.state,'resolved');release();await h.tick(0);
  assert.deepEqual(h.ports[0].messages.map(m=>m.action),['hello','add']);assert.equal(sent,0);h.client.disconnect();assert.equal(h.timers.size,0);
 });
 await test('legacy_validation',async()=>{
  const h=fixture({legacy:true});let release;const r=h.request('media',1000,undefined,()=>new Promise(resolve=>{release=resolve;}));await h.tick(1000);
  assert.equal(r.state,'rejected');assert.equal(h.oneShot.length,0);release();await h.tick(0);assert.equal(h.oneShot.length,0);h.client.disconnect();assert.equal(h.timers.size,0);
 });
 await test('shared_startup_budget',async()=>{
  const h=fixture({helloDelay:1500}),short=h.request('add',1000),long=h.request('media',3000);await h.tick(1000);
  assert.equal(short.state,'rejected');assert.equal(long.state,'pending');assert.equal(h.ports[0].closed,false);
  await h.tick(500);assert.equal(long.state,'resolved');assert.deepEqual(h.ports[0].messages.map(m=>m.action),['hello','media']);h.client.disconnect();assert.equal(h.timers.size,0);
 });
 await test('dispatch_budget',async()=>{
  const h=fixture({helloDelay:800,replyDelay:500}),r=h.request('add',1000);await h.tick(1000);
  assert.equal(r.state,'rejected');assert.match(r.error.message,/download list/);await h.tick(300);
  assert.equal(r.state,'rejected');assert.equal(h.ports.length,1);assert.deepEqual(h.ports[0].messages.map(m=>m.action),['hello','add']);assert.equal(h.timers.size,0);
 });
 await test('disconnect_validation',async()=>{
  const h=fixture();let release,sent=0;const r=h.request('media',4000,()=>sent++,()=>new Promise(resolve=>{release=resolve;}));await h.tick(0);h.client.disconnect();await h.tick(0);
  assert.equal(r.state,'rejected');assert.match(r.error.message,/connection closed/);release();await h.tick(0);assert.equal(sent,0);assert.equal(h.ports.length,1);assert.deepEqual(h.ports[0].messages.map(m=>m.action),['hello']);assert.equal(h.timers.size,0);
 });
 await test('before_send_disconnect',async()=>{
  const h=fixture(),r=h.request('media',4000,()=>h.client.disconnect());await h.tick(0);
  assert.equal(r.state,'rejected');assert.match(r.error.message,/connection closed/);assert.deepEqual(h.ports[0].messages.map(m=>m.action),['hello']);assert.equal(h.timers.size,0);
 });
 await test('disconnect_legacy',async()=>{
  const h=fixture({legacy:true,legacyDelay:null,persistent:false}),r=h.request('add',4000);await h.tick(0);h.client.disconnect();await h.tick(0);
  assert.equal(r.state,'rejected');assert.match(r.error.message,/connection closed/);assert.equal(h.oneShot.length,1);h.legacyReplies[0]({ok:true});await h.tick(0);assert.equal(r.state,'rejected');assert.equal(h.timers.size,0);
 });
 await test('status_retry_budget',async()=>{
  const h=fixture({replyDelay:null}),r=h.request('capture-status',1000);await h.tick(1000);
  assert.equal(r.state,'pending');assert.equal(h.ports.length,2);await h.tick(1000);
  assert.equal(r.state,'rejected');assert.equal(h.ports.length,2);assert.equal(h.ports.flatMap(p=>p.messages.filter(m=>m.action==='capture-status')).length,2);assert.equal(h.timers.size,0);
 });
 console.log('ALL '+count+' NATIVE DEADLINE CHECKS PASSED ('+variant+')');return count;
}
module.exports.runDeadlineChecks=runDeadlineChecks;
if(require.main===module)(async()=>{await runHandshakeChecks();await runDeadlineChecks();})().catch(error=>{console.error(error);process.exitCode=1;});
