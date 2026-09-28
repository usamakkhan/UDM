'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),crypto=require('node:crypto');
const M=require('../browser/chromium/media.js'),{box}=require('./support/sidx-tree.cjs'),{fixture}=require('./support/external-tree.cjs');
let passed=0;async function check(name,fn){await fn();passed++;console.log('PASS '+name);}
async function resolve(f,{whole=false,mutate=()=>{},budget}={}){
 const spec=whole?{}:f.index,at=spec.start||0,initial={bytes:Buffer.from(f.indexBytes.subarray(at,whole?undefined:at+spec.length)),total:f.indexBytes.length};let calls=0;
 mutate(initial,0,{start:at,length:initial.bytes.length});
 const result=await M.resolveExternalSidx(spec,initial,f.bytes.length,async span=>{const r={bytes:Buffer.from(f.indexBytes.subarray(span.start,span.start+span.length)),total:f.indexBytes.length};mutate(r,++calls,span);return r;},budget);return {result,calls};
}
function harness({whole=false,mixed=false,mutate=()=>{},navigate=0,credentials=false,deniedAfter=0,dateOnly=false}={}){
 const f=fixture({mixed}),storage={},calls=[],fetches=[],player={page:'https://page.test/watch',timeOrigin:1,token:'p1',stamp:'1',current:'blob:fixture',sources:[],height:360,width:640,duration:8,videoCount:1,title:'External tree'};
 player.manifests=[{url:'https://page.test/master.mpd',type:'application/dash+xml',page:player.page,time:Date.now(),text:'<MPD><Period><AdaptationSet mimeType="video/mp4"><Representation height="360"><BaseURL>https://media.test/v.mp4</BaseURL><SegmentBase><Initialization range="0-99"/><RepresentationIndex sourceURL="https://index.test/v.idx"'+(whole?'':' range="'+f.index.start+'-'+(f.index.start+f.index.length-1)+'"')+'/></SegmentBase></Representation></AdaptationSet></Period></MPD>'}];
 const api={runtime:{},storage:{local:{get:async()=>({settings:{cookies:credentials}})},session:{get:async key=>({[key]:storage[key]}),set:async v=>Object.assign(storage,v)}},tabs:{get:async()=>({id:1,url:player.page,incognito:false})},permissions:{contains:async()=>!deniedAfter||fetches.length<deniedAfter},cookies:{getAll:async()=>[]},scripting:{executeScript:async()=>[{frameId:0,documentId:'d1',result:player}]},webRequest:{onHeadersReceived:{addListener(){}}}};
 const sandbox={UdmMedia:M,URL,crypto,Uint8Array,DataView,AbortController,TextDecoder,setTimeout,clearTimeout,navigator:{userAgent:'fixture'},nativeRequest:async m=>{if(m.action==='preferences')return {ok:true,adaptiveResources:1};calls.push(m);return {ok:true};},requestContext:{resolve:({url})=>({method:'GET',headers:{Authorization:url.includes('index.test')?'Bearer index-only':'Bearer media-only'}})},fetch:async(url,options)=>{
  const index=url.includes('index.test'),bytes=index?f.indexBytes:f.bytes,match=/^bytes=(\d+)-(\d+)$/.exec(options.headers.Range||''),start=match?+match[1]:0,end=match?+match[2]:bytes.length-1;
  fetches.push({url,options,start});const headers={'Content-Length':String(end-start+1),...(match?{'Content-Range':'bytes '+start+'-'+end+'/'+bytes.length}:{}),...(dateOnly?{'Last-Modified':'Sun, 27 Sep 2026 01:00:00 GMT'}:{ETag:index?'"index-1"':'"media-1"'})};
  const response={status:match?206:200,headers,bytes:Buffer.from(bytes.subarray(start,end+1)),url};mutate(response,{index,child:index&&start!==0&&start!==f.index.start,number:fetches.length});if(navigate===fetches.length)player.stamp='changed';
  const result=new Response(response.bytes,{status:response.status,headers:response.headers});Object.defineProperty(result,'url',{value:response.url});return result;
 }};
 vm.createContext(sandbox);vm.runInContext(fs.readFileSync(path.join(__dirname,'../browser/chromium/sites.js'),'utf8')+'\nglobalThis.sites=UdmSites;',sandbox);sandbox.sites.install(api);
 const message={page:player.page,token:'p1'},sender={tab:{id:1},frameId:0,documentId:'d1',url:player.page};return {f,calls,fetches,run:async()=>{const list=await sandbox.sites.list(message,sender);assert.equal(fetches.length,0);return sandbox.sites.download({...message,key:list.choices[0].key},sender);}};
}
(async()=>{
 for(const version of [0,1])for(const mixed of [false,true])for(const whole of [false,true])await check('External v'+version+' '+(mixed?'mixed':'hierarchical')+' '+(whole?'whole':'ranged')+' tree produces exact independent media offsets',async()=>{const f=fixture({mixed,version}),r=await resolve(f,{whole});assert.deepEqual(r.result,f.ranges);assert.equal(r.calls,whole?0:mixed?1:2);});
 await check('Child timescales preserve exact rational timing',async()=>{const r=await resolve(fixture(),{mutate:(r,n)=>{if(n===1){r.bytes.writeUInt32BE(2000,16);r.bytes.writeUInt32BE(4000,44);r.bytes.writeUInt32BE(4000,56);}}});assert.equal(r.result.length,4);});
 await check('A ranged root may contain all its descendants',async()=>{const f=fixture();f.index.length=f.indexBytes.length-f.index.start;const r=await resolve(f);assert.equal(r.calls,0);assert.deepEqual(r.result,f.ranges);});
 for(const [name,mutate] of [
  ['child identity',(r,n)=>{if(n===1)r.bytes.writeUInt32BE(2,12);}],
  ['child time',(r,n)=>{if(n===1)r.bytes.writeBigUInt64BE(1n,20);}],
  ['child duration',(r,n)=>{if(n===1)r.bytes.writeUInt32BE(1,44);}],
  ['child media gap',(r,n)=>{if(n===1)r.bytes.writeBigUInt64BE(101n,28);}],
  ['child media overlap',(r,n)=>{if(n===2)r.bytes.writeBigUInt64BE(100n,28);}],
  ['leaf beyond media',(r,n)=>{if(n===1)r.bytes.writeUInt32BE(1000,104);}],
  ['changed index length',(r,n)=>{if(n===1)r.total++;}],
  ['truncated child',(r,n)=>{if(n===1)r.bytes=r.bytes.subarray(1);}],
  ['child missing index',(r,n)=>{if(n===1)r.bytes.write('free',4);}],
  ['child beyond index file',(r,n)=>{if(n===0)r.bytes.writeUInt32BE(0x80001000,40);}],
 ])await check('Rejects '+name,()=>assert.rejects(resolve(fixture(),{mutate})));
 await check('Whole-file unreferenced top-level index is rejected',async()=>{const f=fixture();f.indexBytes=Buffer.concat([f.indexBytes,box({parts:[{length:1,duration:1}]})]);await assert.rejects(resolve(f,{whole:true}),/Independent external/);});
 await check('Whole-file trailing malformed box is rejected',async()=>{const f=fixture();f.indexBytes=Buffer.concat([f.indexBytes,Buffer.alloc(7)]);await assert.rejects(resolve(f,{whole:true}),/Truncated external/);});
 await check('Partially cached child spans are rejected before mixed reads',async()=>{const f=fixture();f.index.length+=20;await assert.rejects(resolve(f),/partially overlap/);});
 for(const [name,budget,pattern] of [['request',{requests:64,bytes:0,references:0},/request or byte/],['byte',{requests:0,bytes:2*1024*1024-1,references:0},/request or byte/],['reference',{requests:0,bytes:0,references:2400},/too many references/]])await check('External tree obeys shared '+name+' budget',()=>assert.rejects(resolve(fixture(),{budget}),pattern));
 await check('Cached deep chain cannot evade depth limit',async()=>{const nodes=Array.from({length:10},(_,i)=>box({offset:100,parts:[{index:i<9,length:i<9?(9-i)*52:100,duration:1}]})),indexBytes=Buffer.concat(nodes);await assert.rejects(M.resolveExternalSidx({}, {bytes:indexBytes,total:indexBytes.length},200,()=>{throw Error('Unexpected read');}),/eight levels/);});
 await check('Flat external parser refuses a missing media bound',()=>assert.throws(()=>M.sidxExternal(box({parts:[{length:1,duration:1}]}),0,52),/media bounds/));
 for(const whole of [false,true])for(const mixed of [false,true])await check('Native handoff binds only media for '+(whole?'whole':'ranged')+' '+(mixed?'mixed':'hierarchical')+' indexes',async()=>{const h=harness({whole,mixed});await h.run();assert.equal(h.calls.length,1);assert.equal(h.fetches.length,whole?2:mixed?3:4);const plan=h.calls[0].plan;assert.deepEqual(JSON.parse(JSON.stringify(plan.resources)),[{url:'https://media.test/v.mp4',size:500,etag:'"media-1"'}]);assert.deepEqual(Array.from(plan.tracks[0].segments.slice(1),s=>({start:s.start,length:s.length})),h.f.ranges);assert(!JSON.stringify(plan).includes('index.test'));});
 await check('Child requests use index validators and exact scoped credentials',async()=>{const h=harness({credentials:true});await h.run();assert(h.fetches.filter(f=>f.url.includes('index.test')).slice(1).every(f=>f.options.headers['If-Match']==='"index-1"'&&f.options.headers.Authorization==='Bearer index-only'));assert.equal(h.fetches.find(f=>f.url.includes('media.test')).options.headers.Authorization,'Bearer media-only');});
 await check('Date-only indexes bind later child requests',async()=>{const h=harness({dateOnly:true});await h.run();assert(h.fetches.filter(f=>f.url.includes('index.test')).slice(1).every(f=>f.options.headers['If-Unmodified-Since']==='Sun, 27 Sep 2026 01:00:00 GMT'));});
 for(const [name,options] of [
  ['changed index ETag',{mutate:(r,c)=>{if(c.child)r.headers.ETag='"changed"';}}],
  ['missing index ETag',{mutate:(r,c)=>{if(c.child)delete r.headers.ETag;}}],
  ['child ignores Range',{mutate:(r,c)=>{if(c.child)r.status=200;}}],
  ['failed index precondition',{mutate:(r,c)=>{if(c.child)r.status=412;}}],
  ['child redirect',{mutate:(r,c)=>{if(c.child)r.status=302;}}],
  ['child range mismatch',{mutate:(r,c)=>{if(c.child)r.headers['Content-Range']='bytes 0-167/416';}}],
  ['child compression',{mutate:(r,c)=>{if(c.child)r.headers['Content-Encoding']='gzip';}}],
  ['navigation during child read',{navigate:3}],
  ['permission withdrawn before child read',{deniedAfter:2}],
 ])await check(name+' creates no native job',async()=>{const h=harness(options);await assert.rejects(h.run());assert.equal(h.calls.length,0);});
 console.log('ALL '+passed+' EXTERNAL TREE CHECKS PASSED');
})().catch(e=>{console.error(e);process.exitCode=1;});
