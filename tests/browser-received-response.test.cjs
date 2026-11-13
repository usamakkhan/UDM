'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
const {harness}=require('./browser.test.cjs');
const results=[],output=process.argv[2]&&path.resolve(process.argv[2]);
if(output&&fs.existsSync(output))throw Error('Use a fresh result file');
const item={id:91,url:'https://example.test/file.zip',filename:'file.zip',mime:'application/zip',state:'in_progress',paused:false};
async function test(name,fn){try{await fn();results.push({name,passed:true});console.log('PASS '+name);}catch(error){results.push({name,passed:false,error:error.message});console.error('FAIL '+name+': '+error.message);}}
(async()=>{
 for(const family of ['chromium','firefox']){
  for(const transactional of [false,true])for(const totalBytes of [256]){
   await test(family+' '+(transactional?'transactional':'legacy')+' retains fully received '+totalBytes+'-byte response before ownership',async()=>{
    let begins=0;
    const recovery={create:()=>({install(){},begin:async()=>{begins++;throw Error('Unexpected ownership');}})};
    const h=await harness({capture:true},false,transactional?{captureTransaction:1}:{},null,null,family,[],transactional?{UdmCaptureRecovery:recovery}:{});
    h.api.downloads.search=async()=>[{...item,totalBytes,bytesReceived:totalBytes}];
    await h.events.download(item);
    assert.deepEqual(h.calls,[],'No pause, resume, cancel or native Add');
    assert.equal(begins,0,'No ownership checkpoint for an already received response');
   });
  }
  await test(family+' resumes response that finishes between live check and pause',async()=>{
   const h=await harness({capture:true},false,{},null,null,family);let reads=0;
   h.api.downloads.search=async()=>++reads===1?[{...item,totalBytes:256,bytesReceived:128}]:[{...item,paused:true,totalBytes:256,bytesReceived:256}];
   await h.events.download(item);
   assert.deepEqual(h.calls.map(call=>call[0]),['pause','resume']);
  });
  await test(family+' unknown byte length remains eligible',async()=>{
   const h=await harness({capture:true},false,{},null,null,family);let reads=0;
   h.api.downloads.search=async()=>[{...item,paused:++reads>1,totalBytes:-1,bytesReceived:0}];
   await h.events.download(item);
   assert.deepEqual(h.calls.map(call=>call[0]),['pause','native','cancel']);
  });
  await test(family+' zero byte-length placeholder remains eligible',async()=>{
   const h=await harness({capture:true},false,{},null,null,family);let reads=0;
   h.api.downloads.search=async()=>[{...item,paused:++reads>1,totalBytes:0,bytesReceived:0}];
   await h.events.download(item);
   assert.deepEqual(h.calls.map(call=>call[0]),['pause','native','cancel']);
  });
 }
 const failed=results.filter(result=>!result.passed).length;
 if(output){fs.mkdirSync(path.dirname(output),{recursive:true});fs.writeFileSync(output,JSON.stringify({passed:results.length-failed,failed,scope:'Both bundle sources in browser API simulations; no personal browser or origin transfer',results},null,2));}
 console.log(results.length+' checks; '+failed+' failed');if(failed)process.exitCode=1;
})().catch(error=>{console.error(error);process.exitCode=1;});
