'use strict';
const assert=require('node:assert/strict');
let count=0;const pass=name=>{count++;console.log('PASS '+name);};
const event=()=>{const listeners=new Set();return {addListener:f=>listeners.add(f),removeListener:f=>listeners.delete(f),emit:value=>{for(const f of [...listeners])f(value);},size:()=>listeners.size};};
const item={id:7,url:'https://example.test/file.pdf',filename:'',state:'in_progress',mime:'application/pdf'};
const next=()=>new Promise(resolve=>setImmediate(resolve));
function harness(search){const changed=event(),erased=event();return {api:{downloads:{search,onChanged:changed,onErased:erased}},changed,erased,clean(){assert.equal(changed.size(),0);assert.equal(erased.size(),0);}};}
(async()=>{
for(const family of ['chromium','firefox']){
 const R=require('../browser/'+family+'/file-recognition.js');
 const known={...item,filename:'chosen.bin'};let reads=0;
 assert.equal(await R.downloadItem({downloads:{search:async()=>{reads++;}}},known),known);assert.equal(reads,0);pass(family+' known filename has no extra API round trip');
 {
  const h=harness(async()=>[{...item,filename:'keep.txt',finalUrl:'https://example.test/final.pdf'}]);
  const settled=await R.downloadItem(h.api,item);assert.equal(settled.filename,'keep.txt');assert.equal(settled.finalUrl,'https://example.test/final.pdf');assert(!R.allowed(R.classify(settled),['pdf']));h.clean();pass(family+' final browser filename overrides the PDF URL and keeps its policy');
 }
 {
  let current={...item},reads=0;const h=harness(async()=>{reads++;return [current];});
  const pending=R.downloadItem(h.api,item);await next();assert.equal(h.changed.size(),1);h.changed.emit({id:99,filename:{current:'wrong.pdf'}});assert.equal(reads,1);
  current={...item,filename:'archive.bin',mime:'application/zip'};h.changed.emit({id:7,filename:{current:current.filename}});const settled=await pending;assert.equal(settled.filename,'archive.bin');assert(R.allowed(R.classify(settled),['zip']));h.clean();pass(family+' waits for the matching filename event before MIME capture');
 }
 {
  let resolveFirst,reads=0;const h=harness(()=>++reads===1?new Promise(r=>resolveFirst=r):Promise.resolve([{...item,filename:'new.pdf'}]));
  const pending=R.downloadItem(h.api,item);h.changed.emit({id:7,filename:{current:'new.pdf'}});assert.equal((await pending).filename,'new.pdf');resolveFirst([{...item,filename:'stale.txt'}]);await next();h.clean();pass(family+' stale in-flight reads cannot replace an accepted filename');
 }
 for(const [name,result]of [['erased item',[]],['wrong download',[{...item,id:8,filename:'wrong.pdf'}]],['completed download',[{...item,state:'complete',filename:'done.pdf'}]],['interrupted download',[{...item,state:'interrupted',filename:'stop.pdf'}]]]){
  const h=harness(async()=>result);assert.equal(await R.downloadItem(h.api,item),null);h.clean();pass(family+' '+name+' does not acquire native ownership');
 }
 {
  const h=harness(async()=>{throw Error('browser unavailable');});assert.equal(await R.downloadItem(h.api,item),null);h.clean();pass(family+' browser lookup errors leave capture with the browser');
 }
 {
  const h=harness(async()=>[{...item}]);const pending=R.downloadItem(h.api,item);h.erased.emit(7);assert.equal(await pending,null);await next();h.clean();pass(family+' erase notification terminates the filename wait');
 }
 {
  const h=harness(async()=>[{...item}]);assert.equal(await R.downloadItem(h.api,item,10),null);h.clean();pass(family+' unresolved filename times out without listener leaks');
 }
 assert.equal(await R.downloadItem({},item),null);pass(family+' missing download API does not guess a name');
}
console.log('ALL '+count+' DOWNLOAD FILENAME CHECKS PASSED');
})().catch(error=>{console.error(error);process.exitCode=1;});
