'use strict';
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
(async()=>{let passed=0;
 for(const browser of ['chromium','firefox'])for(const marker of ['advideo=1','advideo=0','el=adunit','el=normal&el=adunit','%61dvideo=1','el=detailpage']){
  const id='abcdefghijk',url='https://rr1.googlevideo.com/videoplayback?sabr=1&'+marker,original=new Response(Uint8Array.of(1),{headers:{'content-type':'application/vnd.yt-ump'}}),promise=Promise.resolve(original);let called;
  const context=vm.createContext({URL,URLSearchParams,ArrayBuffer,Uint8Array,TextDecoder,Response,btoa,location:new URL('https://www.youtube.com/watch?v='+id),document:{getElementById:()=>({getVideoData:()=>({video_id:id})})},__udmUmpV1:{request:()=>true,identity:()=>true},fetch:(...args)=>{called=args;return promise;}});
  vm.runInContext(fs.readFileSync(path.resolve(__dirname,'../browser',browser,'capture.js'),'utf8'),context);
  const args=[url,{method:'POST',body:Uint8Array.of(1)}];assert.equal(context.fetch(...args),promise);assert.deepEqual(called,args);passed++;
  const allowed=marker==='el=detailpage',deadline=Date.now()+1000;
  while(allowed&&!context.__udmCaptureV1.session(id)&&Date.now()<deadline)await new Promise(r=>setTimeout(r,5));
  assert.equal(!!context.__udmCaptureV1.session(id),allowed);assert.equal(context.__udmCaptureV1.diagnostics().requests,allowed?1:0);assert.equal(original.bodyUsed,false);passed++;
 }
 console.log(JSON.stringify({passed,failed:0}));
})().catch(e=>{console.error(e);process.exitCode=1;});
