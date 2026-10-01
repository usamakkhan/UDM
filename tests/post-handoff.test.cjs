'use strict';
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'../browser/chromium');let passed=0;
const pass=name=>{passed++;console.log('PASS '+name);};
function harness(preferences,bytes){
 const calls=[],events={};const event=name=>({addListener:f=>events[name]=f});
 const api={runtime:{onInstalled:event('installed'),onMessage:event('message'),sendNativeMessage:async(_,m)=>{calls.push(m);return m.action==='preferences'?preferences:{ok:true,id:'form'};}},
 storage:{local:{get:async()=>({}),set:async()=>{}},session:{get:async()=>({}),set:async()=>{},remove:async()=>{}}},
 tabs:{onRemoved:event('removed'),onUpdated:event('updated')},action:{setBadgeText:async()=>{}},contextMenus:{onClicked:event('context')},downloads:{onCreated:event('download')},webRequest:{onHeadersReceived:event('headers')}};
 const request={method:'POST',contentType:'application/octet-stream',body:Buffer.alloc(bytes,7).toString('base64')};
 const context=vm.createContext({chrome:api,URL,URLSearchParams,navigator:{userAgent:'fixture'},setTimeout,clearTimeout,console,UdmRequestContext:{create:()=>({install(){},resolve:()=>({method:'POST',headers:{},request})})}});
 vm.runInContext(fs.readFileSync(path.join(root,'file-recognition.js'),'utf8')+'\n'+fs.readFileSync(path.join(root,'background.js'),'utf8'),context);
 return {calls,run:()=>context.handoff({url:'https://example.test/form.bin',browserDownload:true})};
}
(async()=>{
 for(const [size,limit] of [[0,0],[65536,65536],[1048576,1048576]]){
  const h=harness({ok:true,postDownloads:true,postBodyLimit:limit},size);await h.run();assert.equal(h.calls.filter(c=>c.action==='add').length,1);pass('Desktop byte limit '+limit+' accepts matching form body');
 }
 let h=harness({ok:true,postDownloads:true},65536);await h.run();assert.equal(h.calls[1].action,'add');pass('Legacy desktop still accepts small form downloads');
 for(const preferences of [{ok:true,postDownloads:true},{ok:true,postDownloads:true,postBodyLimit:65536},{ok:true,postDownloads:false},{ok:false,postDownloads:true,postBodyLimit:1048576}]){
  h=harness(preferences,65537);await assert.rejects(h.run(),/remains in the browser/);assert(!h.calls.some(c=>c.action==='add'));
 }pass('Unsupported desktop versions reject larger bodies before submitting a native download');
 h=harness({ok:true,postDownloads:true,postBodyLimit:1048576},1048577);await assert.rejects(h.run());assert.equal(h.calls.length,1);pass('Limit uses decoded body bytes rather than base64 characters');
 console.log('ALL '+passed+' POST HANDOFF CHECKS PASSED');
})().catch(e=>{console.error(e);process.exitCode=1;});

