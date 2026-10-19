'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm');
const source=fs.readFileSync(require.resolve('../browser/chromium/native-bridge.js'),'utf8');
function fixture({helloDelay=0,replyDelay=0}={}){
 let now=0,next=0;const timers=new Map(),ports=[];
 const later=(fn,delay)=>{const id=++next;timers.set(id,{at:now+delay,fn});return id;};
 const api={runtime:{connectNative(){
  const messages=[],listeners=[],disconnects=[];
  const port={messages,closed:false,onMessage:{addListener:f=>listeners.push(f)},onDisconnect:{addListener:f=>disconnects.push(f)},
   disconnect(){this.closed=true;},postMessage(message){messages.push(message);const hello=message.action==='hello',delay=hello?helloDelay:replyDelay;
    if(delay!==null)later(()=>{if(!port.closed)for(const f of listeners)f({ok:true,protocol:1,requestId:message.requestId});},delay);
   }};ports.push(port);return port;
 },sendNativeMessage(){throw Error('Unexpected legacy replay');}}};
 const context=vm.createContext({setTimeout:later,clearTimeout:id=>timers.delete(id)});vm.runInContext(source,context);
 const client=context.UdmNativeBridge.create(api,'fixture');
 async function flush(){for(let i=0;i<12;i++)await Promise.resolve();}
 async function tick(ms){const until=now+ms;await flush();for(;;){const due=[...timers].filter(([,v])=>v.at<=until).sort((a,b)=>a[1].at-b[1].at)[0];if(!due)break;now=due[1].at;timers.delete(due[0]);due[1].fn();await flush();}now=until;await flush();}
 function request(action,timeout){const result={state:'pending'};client.request({action},timeout).then(value=>Object.assign(result,{state:'resolved',value}),error=>Object.assign(result,{state:'rejected',error}));return result;}
 return {client,ports,tick,request,timers};
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
 console.log('ALL 5 NATIVE HANDSHAKE CHECKS PASSED');return 5;
}
module.exports=runHandshakeChecks;
if(require.main===module)runHandshakeChecks().catch(error=>{console.error(error);process.exitCode=1;});
