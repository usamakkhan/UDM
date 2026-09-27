'use strict';
const {spawn}=require('node:child_process'),assert=require('node:assert/strict'),path=require('node:path');
const exe=process.env.UDM_HOST_EXE||path.join(__dirname,'../release/Udm.NativeHost.exe');
function frame(value){const b=Buffer.from(typeof value==='string'?value:JSON.stringify(value)),h=Buffer.alloc(4);h.writeUInt32LE(b.length);return Buffer.concat([h,b]);}
function run(chunks){return new Promise((resolve,reject)=>{const p=spawn(exe,[],{windowsHide:true}),out=[];const timer=setTimeout(()=>{p.kill();reject(Error('Host protocol timeout'));},10000);p.on('error',reject);p.stdout.on('data',c=>out.push(c));p.stdin.on('error',()=>{});p.on('close',code=>{clearTimeout(timer);try{const data=Buffer.concat(out),replies=[];for(let i=0;i<data.length;){const n=data.readUInt32LE(i);assert(n>0&&n<=262144);replies.push(JSON.parse(data.subarray(i+4,i+4+n)));i+=4+n;assert(i<=data.length);}resolve({code,replies});}catch(e){reject(e);}});(async()=>{for(const b of chunks){p.stdin.write(b);await new Promise(r=>setTimeout(r,5));}p.stdin.end();})();});}
(async()=>{
 let r=await run([frame({action:'hello',requestId:1}),frame({action:'hello',requestId:2})]);assert.equal(r.code,0);assert.deepEqual(r.replies.map(x=>x.requestId),[1,2]);assert(r.replies.every(x=>x.ok&&x.protocol===1&&x.version==='0.40.0'));console.log('PASS real host accepts multiple framed requests on one process');
 const f=frame({action:'hello',requestId:42});r=await run([f.subarray(0,2),f.subarray(2,7),f.subarray(7)]);assert.equal(r.replies[0].requestId,42);console.log('PASS real host handles fragmented prefix and JSON body');
 r=await run([frame({action:'hello'})]);assert.equal(r.code,0);assert(r.replies[0].ok);assert.equal(r.replies[0].requestId,undefined);console.log('PASS legacy one-shot clients retain their framing');
 r=await run([frame({action:'unknown-fixture-action',requestId:7}),frame({action:'hello',requestId:8})]);assert.equal(r.replies[0].ok,false);assert.equal(r.replies[1].ok,true);console.log('PASS failed command preserves the persistent channel');
 r=await run([frame('{bad-json')]);assert.equal(r.replies[0].ok,false);console.log('PASS malformed JSON returns a bounded error');
 const big=Buffer.alloc(4);big.writeUInt32LE(2097153);r=await run([big]);assert.equal(r.code,1);assert.equal(r.replies.length,0);console.log('PASS oversized native messages rejected before allocation');
 r=await run([f.subarray(0,8)]);assert.equal(r.code,1);assert.equal(r.replies.length,0);console.log('PASS truncated native bodies cannot dispatch a command');

 r=await run([frame({action:'hello',requestId:90,padding:'x'.repeat(1500000)})]);assert.equal(r.code,0);assert.equal(r.replies[0].postBodyLimit,1048576);assert.equal(r.replies[0].requestId,90);console.log('PASS native input accepts a large frame and advertises its POST byte limit');
 console.log('ALL 8 REAL NATIVE PROTOCOL CHECKS PASSED');
})().catch(e=>{console.error(e);process.exitCode=1;});
