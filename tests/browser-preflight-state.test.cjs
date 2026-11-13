'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
const {harness}=require('./browser.test.cjs');
const results=[],output=process.argv[2]&&path.resolve(process.argv[2]);
if(output&&fs.existsSync(output))throw Error('Use a fresh result file');
const tick=()=>new Promise(resolve=>setImmediate(resolve));
const item={id:77,url:'https://example.test/file.zip',filename:'file.zip',mime:'application/zip',state:'in_progress',paused:false};
async function test(name,fn){try{await fn();results.push({name,passed:true});console.log('PASS '+name);}catch(error){results.push({name,passed:false,error:error.message});console.error('FAIL '+name+': '+error.message);}}
(async()=>{
 for(const family of ['chromium','firefox']){
  for(const [name,rows] of [['canceled',[{state:'interrupted',paused:false}]],['completed',[{state:'complete',paused:false}]],['removed',[]],['paused',[{state:'in_progress',paused:true}]],['lookup-failed',null]]){
   await test(family+' leaves '+name+' download untouched after policy preflight',async()=>{
    let release;const gate=new Promise(resolve=>release=resolve);
    const h=await harness({capture:true},false,{},()=>gate,null,family);
    const pending=h.events.download(item);await tick();
    assert.equal(h.preferences.length,1);assert.deepEqual(h.calls,[]);
    h.api.downloads.search=async()=>{if(!rows)throw Error('Download state unavailable');return rows;};
    release();await pending;
    assert.deepEqual(h.calls,[],'No pause, resume, cancel or native download is allowed');
   });
  }
  await test(family+' captures an unchanged live transfer exactly once',async()=>{
   const h=await harness({capture:true},false,{},null,null,family);
   await h.events.download(item);
   assert.deepEqual(h.calls.map(call=>call[0]),['pause','native','cancel']);
   assert.equal(h.calls.filter(call=>call[0]==='native'&&call[1].action==='add').length,1);
  });
  await test(family+' rechecks user resume after extension pause',async()=>{
   const h=await harness({capture:true},false,{},null,null,family);
   h.api.downloads.search=async()=>[{state:'in_progress',paused:false}];
   await h.events.download(item);
   assert.deepEqual(h.calls.map(call=>call[0]),['pause']);
  });
 }
 const failed=results.filter(result=>!result.passed).length;
 if(output){fs.mkdirSync(path.dirname(output),{recursive:true});fs.writeFileSync(output,JSON.stringify({passed:results.length-failed,failed,scope:'Browser API simulations for both source bundles; no personal browser or network transfer',results},null,2));}
 console.log(results.length+' checks; '+failed+' failed');if(failed)process.exitCode=1;
})().catch(error=>{console.error(error);process.exitCode=1;});
