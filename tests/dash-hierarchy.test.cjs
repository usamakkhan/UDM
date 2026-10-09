'use strict';
const assert=require('node:assert/strict'),M=require('../browser/chromium/media.js'),{box,fixture}=require('./support/sidx-tree.cjs');
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),crypto=require('node:crypto');
let passed=0;async function check(name,run){await run();passed++;console.log('PASS '+name);}
const clone=()=>fixture();
async function resolve(f,{mutate=()=>{},budget}={}){let calls=0;const out=await M.resolveSidx(f.index,async span=>{const r={bytes:Buffer.from(f.bytes.subarray(span.start,span.start+span.length)),total:f.bytes.length};mutate(r,++calls,span);return r;},budget);return {out,calls};}
function harness({mutate=()=>{},navigate=0,credentials=false,permission=true,extraAudio=0,rootHeaders={ETag:'"snapshot-1"'},capability=true}={}){
 const f=clone(),storage={},calls=[],fetches=[],player={page:'https://page.test/watch',timeOrigin:1,token:'v1',stamp:'1',current:'blob:fixture',sources:[],height:360,width:640,duration:8,videoCount:1,title:'Fixture'};
 player.manifests=[{url:'https://cdn.test/a.mpd',type:'application/dash+xml',page:player.page,time:Date.now(),text:'<MPD type="static"><Period><AdaptationSet mimeType="video/mp4"><Representation height="360"><BaseURL>v.mp4</BaseURL><SegmentBase indexRange="'+f.index.start+'-'+(f.index.start+f.index.length-1)+'"><Initialization range="0-99"/></SegmentBase></Representation></AdaptationSet></Period></MPD>'}];
 if(extraAudio)player.manifests[0].text=player.manifests[0].text.replace('</Period>','<AdaptationSet mimeType="audio/mp4"><Representation id="a"><SegmentList>'+'<SegmentURL media="a.m4s"/>'.repeat(extraAudio)+'</SegmentList></Representation></AdaptationSet></Period>');
 const api={runtime:{},storage:{local:{get:async()=>({settings:{cookies:credentials}})},session:{get:async key=>({[key]:storage[key]}),set:async v=>Object.assign(storage,v)}},tabs:{get:async()=>({id:1,url:player.page,incognito:false})},permissions:{contains:async()=>permission},cookies:{getAll:async()=>[]},scripting:{executeScript:async()=>[{frameId:0,documentId:'doc',result:player}]},webRequest:{onHeadersReceived:{addListener(){}}}};
 const sandbox={UdmMedia:M,URL,crypto,Uint8Array,DataView,AbortController,TextEncoder,TextDecoder,setTimeout,clearTimeout,navigator:{userAgent:'fixture'},nativeRequest:async m=>{if(m.action==='preferences')return {ok:true,adaptiveResources:capability?1:0};calls.push(m);return {ok:true};},requestContext:{resolve:()=>({method:'GET',headers:{Authorization:'Bearer fixture-only'}})},
 fetch:async(url,options)=>{fetches.push({url,options});const m=/^bytes=(\d+)-(\d+)$/.exec(options.headers.Range),start=+m[1],end=+m[2];const r={status:206,bytes:Buffer.from(f.bytes.subarray(start,end+1)),headers:{...rootHeaders,'Content-Range':'bytes '+start+'-'+end+'/'+f.bytes.length,'Content-Length':String(end-start+1)}};mutate(r,fetches.length);if(navigate===fetches.length)player.stamp='changed';return new Response(r.bytes,{status:r.status,headers:r.headers});}};
 vm.createContext(sandbox);vm.runInContext(fs.readFileSync(path.join(__dirname,'../browser/chromium/sites.js'),'utf8')+'\nglobalThis.sites=UdmSites;',sandbox);sandbox.sites.install(api);
 const message={token:'v1',page:player.page},sender={tab:{id:1},frameId:0,documentId:'doc',url:player.page};
 return {calls,fetches,f,run:async()=>{const list=await sandbox.sites.list(message,sender);assert.equal(fetches.length,0);return sandbox.sites.download({...message,key:list.choices[0].key},sender);}};
}
(async()=>{
 await check('Three-level independent index tree resolves every exact leaf in presentation order',async()=>{const f=clone(),r=await resolve(f);assert.deepEqual(r.out,f.ranges);assert.equal(r.calls,7);});
 await check('Flat compatibility wrapper still refuses unresolved child references',()=>assert.throws(()=>M.sidx(clone().bytes.subarray(100,164),100,1000),/bounded expansion/));
 await check('Direct version-zero indexes retain exact output',async()=>{const b=box({version:0,parts:[{length:100,duration:2000}]});const r=await M.resolveSidx({start:0,length:b.length},async()=>({bytes:b,total:b.length+100}));assert.deepEqual(r,[{start:b.length,length:100}]);});
 await check('Child timescales can differ when presentation times match exactly',async()=>{const r=await resolve(clone(),{mutate:(r,n)=>{if(n===2){r.bytes.writeUInt32BE(2000,16);r.bytes.writeUInt32BE(4000,44);r.bytes.writeUInt32BE(4000,56);}}});assert.equal(r.out.length,4);});
 for(const [name,mutate] of [
  ['child stream identity',(r,n)=>{if(n===2)r.bytes.writeUInt32BE(2,12);}],
  ['child presentation start',(r,n)=>{if(n===2)r.bytes.writeBigUInt64BE(1n,20);}],
  ['child total duration',(r,n)=>{if(n===2)r.bytes.writeUInt32BE(1,44);}],
  ['changed resource length',(r,n)=>{if(n===2)r.total++;}],
  ['truncated child',(r,n)=>{if(n===2)r.bytes=r.bytes.subarray(1);}],
  ['media overlaps another index',(r,n)=>{if(n===3)r.bytes.writeBigUInt64BE(0n,28);}],
  ['child is not an index',(r,n)=>{if(n===2)r.bytes.write('free',4);}],
  ['oversized child reference',(r,n)=>{if(n===1)r.bytes.writeUInt32BE(0x80080001,40);}],
  ['oversized media reference',(r,n)=>{if(n===3)r.bytes.writeUInt32BE(268435457,40);}],
 ])await check('Rejects '+name,()=>assert.rejects(resolve(clone(),{mutate})));
 await check('Unexpected child placement stops expansion early',async()=>{let calls=0;const f=clone();await assert.rejects(resolve(f,{mutate:(r,n)=>{calls=n;if(n===2)r.bytes.writeBigUInt64BE(0n,28);}}));assert(calls<=3);});
 await check('Shared request cap stops before issuing a 65th read',async()=>{let calls=0;await assert.rejects(resolve(clone(),{budget:{requests:64,bytes:0,references:0},mutate:()=>calls++}),/request or byte/);assert.equal(calls,0);});
 await check('Shared byte budget stops before excess data is fetched',()=>assert.rejects(resolve(clone(),{budget:{requests:0,bytes:2*1024*1024-1,references:0}}),/request or byte/));
 await check('Shared reference budget rejects a wide tree',()=>assert.rejects(resolve(clone(),{budget:{requests:0,bytes:0,references:M.MAX*2}}),/too many references/));
 await check('Deep chain is stopped at the ninth child before further requests',async()=>{const nodes=Array.from({length:10},(_,i)=>box({parts:[{index:i<9,length:i<9?52:100,duration:1}]}));let calls=0;const bytes=Buffer.concat([...nodes,Buffer.alloc(100)]);await assert.rejects(M.resolveSidx({start:0,length:52},async span=>{calls++;return {bytes:bytes.subarray(span.start,span.start+span.length),total:bytes.length};}),/eight levels/);assert.equal(calls,9);});
 await check('Expansion never forwards index references to the desktop',async()=>{const h=harness();await h.run();assert.equal(h.calls.length,1);assert.equal(h.fetches.length,7);assert.equal(h.calls[0].plan.tracks[0].segments.length,5);assert(!h.calls[0].plan.tracks[0].index);assert.deepEqual(Array.from(h.calls[0].plan.tracks[0].segments.slice(1),s=>({start:s.start,length:s.length})),h.f.ranges);});
 await check('Child reads bind to the first strong ETag and retain opt-out credentials',async()=>{const h=harness();await h.run();assert(!h.fetches[0].options.headers['If-Match']);assert(h.fetches.slice(1).every(f=>f.options.headers['If-Match']==='"snapshot-1"'));assert(h.fetches.every(f=>f.options.redirect==='error'&&f.options.credentials==='omit'&&!f.options.headers.Authorization));});
 await check('Explicit credential opt-in applies the same captured authorization to each child',async()=>{const h=harness({credentials:true});await h.run();assert(h.fetches.every(f=>f.options.credentials==='include'&&f.options.headers.Authorization==='Bearer fixture-only'));});
 await check('Last-Modified binds child reads when there is no strong ETag',async()=>{const date='Sun, 27 Sep 2026 01:00:00 GMT',h=harness({rootHeaders:{ETag:'W/"weak"','Last-Modified':date}});await h.run();assert(h.fetches.slice(1).every(f=>f.options.headers['If-Unmodified-Since']===date&&!f.options.headers['If-Match']));});
 for(const [name,options] of [
  ['child range ignored',{mutate:(r,n)=>{if(n===2)r.status=200;}}],
  ['changed ETag',{mutate:(r,n)=>{if(n===2)r.headers.ETag='"changed"';}}],
  ['missing ETag',{mutate:(r,n)=>{if(n===2)delete r.headers.ETag;}}],
  ['failed precondition',{mutate:(r,n)=>{if(n===2)r.status=412;}}],
  ['wrong child Content-Range',{mutate:(r,n)=>{if(n===2)r.headers['Content-Range']='bytes 0-63/1000';}}],
  ['compressed child bytes',{mutate:(r,n)=>{if(n===2)r.headers['Content-Encoding']='gzip';}}],
  ['navigation after root fetch',{navigate:1}],
  ['missing host permission',{permission:false}],
 ])await check(name+' creates no native job',async()=>{const h=harness(options);await assert.rejects(h.run());assert.equal(h.calls.length,0);assert(h.fetches.length<=2);});
 await check('An unindexed track after a resolved tree still obeys the selection segment cap',async()=>{const h=harness({extraAudio:M.MAX-4});await assert.rejects(h.run(),/segment limit/);assert.equal(h.calls.length,0);});
 await check('Native handoff contains the index resource identity and total length',async()=>{const h=harness();await h.run();assert.equal(h.calls[0].plan.resourceVersion,1);assert.deepEqual(JSON.parse(JSON.stringify(h.calls[0].plan.resources)),[{url:'https://cdn.test/v.mp4',size:h.f.bytes.length,etag:'"snapshot-1"'}]);});
 await check('Date-only resources keep their date instead of a weak ETag',async()=>{const date='Sun, 27 Sep 2026 01:00:00 GMT',h=harness({rootHeaders:{ETag:'W/"weak"','Last-Modified':date}});await h.run();assert.equal(h.calls[0].plan.resources[0].modified,date);assert(!h.calls[0].plan.resources[0].etag);});
 await check('Missing validators still bind the observed resource size',async()=>{const h=harness({rootHeaders:{}});await h.run();assert.equal(h.calls[0].plan.resources[0].size,h.f.bytes.length);assert(!h.calls[0].plan.resources[0].etag);assert(!h.calls[0].plan.resources[0].modified);});
 await check('An older desktop is rejected before index fetch or submission',async()=>{const h=harness({capability:false});await assert.rejects(h.run(),/Update the UDM desktop/);assert.equal(h.fetches.length,0);assert.equal(h.calls.length,0);});
 console.log('ALL '+passed+' HIERARCHICAL DASH CHECKS PASSED');
})().catch(e=>{console.error(e);process.exitCode=1;});
