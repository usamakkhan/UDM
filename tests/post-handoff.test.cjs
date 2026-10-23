'use strict';
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'../browser/chromium');let passed=0;
const pass=name=>{passed++;console.log('PASS '+name);};
function harness(preferences,bytes){
 const calls=[],events={},staging={received:Buffer.alloc(0),declared:null};const event=name=>({addListener:f=>events[name]=f});
 const api={runtime:{onInstalled:event('installed'),onMessage:event('message'),sendNativeMessage:async(_,m)=>{calls.push(m);if(m.action==='preferences')return preferences;if(m.action==='post-body-begin'){staging.received=Buffer.alloc(0);staging.declared=m;return preferences.beginFails?{ok:false,error:'no room'}:{ok:true,bodyToken:'0123456789abcdef0123456789abcdef',chunkBytes:preferences.chunkBytes||1048576};}if(m.action==='post-body-chunk'){assert.equal(m.bodyToken,'0123456789abcdef0123456789abcdef');assert.equal(m.offset,staging.received.length);const data=Buffer.from(m.data,'base64');assert.equal(data.toString('base64'),m.data);assert(data.length<=(preferences.chunkBytes||1048576));staging.received=Buffer.concat([staging.received,data]);return {ok:true,received:staging.received.length};}if(m.action==='post-body-finish'){assert.equal(m.bodyToken,'0123456789abcdef0123456789abcdef');assert.equal(staging.received.length,staging.declared.size);assert.equal(require('node:crypto').createHash('sha256').update(staging.received).digest('hex'),staging.declared.sha256);return {ok:true,request:{method:'POST',contentType:staging.declared.contentType,bodyFile:m.bodyToken,bodySize:staging.declared.size,bodySha256:staging.declared.sha256}};}return {ok:true,id:'form'};}},
 storage:{local:{get:async()=>({}),set:async()=>{}},session:{get:async()=>({}),set:async()=>{},remove:async()=>{}}},
 tabs:{onRemoved:event('removed'),onUpdated:event('updated')},action:{setBadgeText:async()=>{}},contextMenus:{onClicked:event('context')},downloads:{onCreated:event('download')},webRequest:{onHeadersReceived:event('headers')}};
 const request=preferences.staged?{method:'POST',contentType:'application/octet-stream',bytes:preferences.staged}:{method:'POST',contentType:'application/octet-stream',body:Buffer.alloc(bytes,7).toString('base64')};
 const context=vm.createContext({chrome:api,URL,URLSearchParams,navigator:{userAgent:'fixture'},setTimeout,clearTimeout,console,crypto,btoa:v=>Buffer.from(v,'binary').toString('base64'),Uint8Array,UdmRequestContext:{create:()=>({install(){},resolve:()=>({method:'POST',headers:{},request})})}});
 vm.runInContext(fs.readFileSync(path.join(root,'file-recognition.js'),'utf8')+'\n'+fs.readFileSync(path.join(root,'background.js'),'utf8'),context);
 return {calls,staging,run:()=>context.handoff({url:'https://example.test/form.bin',browserDownload:true})};
}
(async()=>{
 for(const [size,limit] of [[0,0],[65536,65536],[1048576,1048576],[4*1048576,4*1048576]]){
  const h=harness({ok:true,postDownloads:true,postBodyLimit:limit},size);await h.run();assert.equal(h.calls.filter(c=>c.action==='add').length,1);pass('Desktop byte limit '+limit+' accepts matching form body');
 }
 let h=harness({ok:true,postDownloads:true},65536);await h.run();assert.equal(h.calls[1].action,'add');pass('Legacy desktop still accepts small form downloads');
 for(const preferences of [{ok:true,postDownloads:true},{ok:true,postDownloads:true,postBodyLimit:65536},{ok:true,postDownloads:false},{ok:false,postDownloads:true,postBodyLimit:1048576}]){
  h=harness(preferences,65537);await assert.rejects(h.run(),/remains in the browser/);assert(!h.calls.some(c=>c.action==='add'));
 }pass('Unsupported desktop versions reject larger bodies before submitting a native download');
 h=harness({ok:true,postDownloads:true,postBodyLimit:1048576},1048577);await assert.rejects(h.run());assert.equal(h.calls.length,1);pass('Limit uses decoded body bytes rather than base64 characters');
 // Bodies above the inline limit: every byte value (not UTF-8) goes to the desktop in ordered base64 chunks, then the add carries the desktop's reference.
 {const bytes=new Uint8Array(5*1048576+13);for(let i=0;i<bytes.length;i++)bytes[i]=(i*131+7)%256;
  h=harness({ok:true,postDownloads:true,postBodyLimit:4*1048576,stagedPostBodyLimit:64*1048576,staged:bytes,chunkBytes:1048576},0);await h.run();
  const actions=h.calls.map(c=>c.action);assert.deepEqual(actions.filter(a=>a!=='preferences'),['post-body-begin',...Array(6).fill('post-body-chunk'),'post-body-finish','add']);
  assert.equal(Buffer.compare(h.staging.received,Buffer.from(bytes)),0);const add=h.calls.find(c=>c.action==='add');assert.equal(add.request.bodyFile,'0123456789abcdef0123456789abcdef');assert.equal(add.request.bodySize,bytes.length);assert.equal(add.request.body,undefined);
  pass('Staged body streams in six ordered chunks and the add references the sealed body');
  h=harness({ok:true,postDownloads:true,postBodyLimit:4*1048576,stagedPostBodyLimit:5*1048576,staged:bytes},0);await assert.rejects(h.run(),/remains in the browser/);assert(!h.calls.some(c=>c.action==='post-body-begin'||c.action==='add'));pass('A body above the staged limit never starts staging');
  h=harness({ok:true,postDownloads:true,postBodyLimit:4*1048576,staged:bytes},0);await assert.rejects(h.run(),/remains in the browser/);assert(!h.calls.some(c=>c.action==='post-body-begin'||c.action==='add'));pass('A desktop without staging support keeps large bodies in the browser');
  h=harness({ok:true,postDownloads:true,postBodyLimit:4*1048576,stagedPostBodyLimit:64*1048576,staged:bytes,beginFails:true},0);await assert.rejects(h.run(),/no room/);assert(!h.calls.some(c=>c.action==='add'));pass('A refused staging never submits the download');}
 console.log('ALL '+passed+' POST HANDOFF CHECKS PASSED');
})().catch(e=>{console.error(e);process.exitCode=1;});

