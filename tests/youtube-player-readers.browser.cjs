'use strict';
const {chromium}=require('playwright'),fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const root=path.resolve(process.argv[2]),checks=[];assert(!fs.existsSync(root));fs.mkdirSync(root,{recursive:true});let browser;
(async()=>{
 browser=await chromium.launch({channel:'msedge',headless:true});
 for(const bundle of ['chromium','firefox'])for(const order of ['capture-first','observer-first']){
  const page=await browser.newPage();let visit=0;
  await page.route('https://www.youtube.com/**',route=>route.fulfill({contentType:'text/html',body:'<!doctype html><div id="movie_player"></div>'}));
  async function setup(){
   await page.goto('https://www.youtube.com/watch?v=abcdefghijk&fixture='+(++visit));
   await page.evaluate(()=>{
    const streams=new Map(),deadlines=new Map();let timer=-1000;
    const counters={clones:0,opened:0,released:0,awaiting:0,cancelCalls:0,sourceCancels:0};
    const nativeTimeout=setTimeout,nativeClear=clearTimeout;
    globalThis.setTimeout=function(callback,delay,...args){if(delay===30000){const id=timer--;deadlines.set(id,()=>callback(...args));return id;}return Reflect.apply(nativeTimeout,globalThis,[callback,delay,...args]);};
    globalThis.clearTimeout=function(id){if(deadlines.delete(id))return;return Reflect.apply(nativeClear,globalThis,[id]);};
    const nativeClone=Response.prototype.clone;
    Response.prototype.clone=function(){const clone=Reflect.apply(nativeClone,this,[]);++counters.clones;const body=clone.body;if(body){const getReader=body.getReader;body.getReader=function(...args){const reader=Reflect.apply(getReader,this,args);++counters.opened;const read=reader.read,cancel=reader.cancel,release=reader.releaseLock;reader.read=function(...args){++counters.awaiting;return Reflect.apply(read,this,args).finally(()=>--counters.awaiting);};reader.cancel=function(...args){++counters.cancelCalls;return Reflect.apply(cancel,this,args);};reader.releaseLock=function(...args){++counters.released;return Reflect.apply(release,this,args);};return reader;};}return clone;};
    const player=document.getElementById('movie_player');player.getVideoData=()=>({video_id:'abcdefghijk'});player.getPlayerResponse=()=>undefined;
    globalThis.fetch=function(input){const token=new URL(input,location.href).searchParams.get('token'),item=streams.get(token);if(!item)throw Error('Unknown owned stream');return item.originalPromise;};
    globalThis.probe={
     start(token,{closed=false,oversize=false,advertised=false}={}){
      const full=JSON.stringify({videoDetails:{videoId:'abcdefghijk',title:token},streamingData:{formats:[{itag:18,height:360}]},padding:oversize?'x'.repeat(2097152):''});
      const split=oversize?2097153:Math.floor(full.length/2),item={full,finished:false,siteDone:false,exact:false,error:''};
      const stream=new ReadableStream({start(controller){item.controller=controller;controller.enqueue(new TextEncoder().encode(closed?full:full.slice(0,split)));if(closed){controller.close();item.finished=true;}},cancel(){++counters.sourceCancels;}});
      item.originalPromise=Promise.resolve(new Response(stream,{headers:{'content-type':'application/json',...(advertised?{'content-length':'2097153'}:{})}}));streams.set(token,item);
      const promise=fetch('/youtubei/v1/player?token='+token);item.promiseUnchanged=promise===item.originalPromise;
      item.done=promise.then(response=>response.text()).then(text=>{item.siteDone=true;item.exact=text===full;},error=>{item.siteDone=true;item.error=error.message;});item.split=split;
     },
     finish(token){const item=streams.get(token);if(!item.finished){item.controller.enqueue(new TextEncoder().encode(item.full.slice(item.split)));item.controller.close();item.finished=true;}return item.done;},
     fail(token){const item=streams.get(token);item.controller.error(Error('owned stream failure'));item.finished=true;return item.done;},
     async cleanup(){for(const [token,item] of streams)if(!item.finished)await this.finish(token);},
     state(){return {...counters,deadlines:deadlines.size,diagnostics:globalThis.__udmCaptureV1?.diagnostics()};},
     sites(){return [...streams.values()].map(({siteDone,exact,error,promiseUnchanged})=>({siteDone,exact,error,promiseUnchanged}));},
     runDeadlines(){const pending=[...deadlines];for(const [id,callback] of pending){deadlines.delete(id);callback();}return pending.length;}
     ,async nativeError(){
      let controller;const response=new Response(new ReadableStream({start(value){controller=value;value.enqueue(new TextEncoder().encode('{'));}}));
      const clone=Reflect.apply(nativeClone,response,[]),reader=clone.body.getReader();await reader.read();const observed=reader.read().catch(()=>{});
      const site=response.text().then(()=>'',error=>error.message);await new Promise(resolve=>nativeTimeout(resolve,0));controller.error(Error('owned stream failure'));
      const expected=await site;await observed;reader.releaseLock();return expected;
     }
    };
   });
   for(const file of order==='capture-first'?['capture.js','page-observer.js']:['page-observer.js','capture.js'])await page.addScriptTag({path:path.join(__dirname,'../browser',bundle,file)});
  }
  const state=()=>page.evaluate(()=>probe.state());
  async function waiting(count){await page.waitForFunction(count=>probe.state().awaiting===count,count,{timeout:1000});}
  async function check(name,run){let observation;try{await setup();await run();observation=await state();checks.push({name:bundle+' '+order+' '+name,passed:true,observation});}catch(error){observation=await state().catch(()=>null);checks.push({name:bundle+' '+order+' '+name,passed:false,error:error.message,observation});}finally{await page.evaluate(()=>probe.cleanup()).catch(()=>{});}console.log(JSON.stringify({name:checks.at(-1).name,passed:checks.at(-1).passed,error:checks.at(-1).error}));}
  await check('retiring a visit releases its blocked clone reader before the site finishes',async()=>{
   await page.evaluate(()=>probe.start('navigation'));await waiting(1);await page.evaluate(()=>{history.pushState({},'', '/watch?v=12345678901');history.pushState({},'', '/watch?v=abcdefghijk');});await waiting(0);
   assert.equal((await state()).released,1);assert.equal((await state()).sourceCancels,0);assert.equal(await page.evaluate(()=>probe.sites()[0].siteDone),false);
   await page.evaluate(()=>probe.finish('navigation'));assert(await page.evaluate(()=>probe.sites().every(item=>item.exact&&item.promiseUnchanged)));
  });
  await check('same-URL state updates retain a productive reader',async()=>{
   await page.evaluate(()=>probe.start('same-url'));await waiting(1);await page.evaluate(()=>history.replaceState({fixture:true},'',location.href));assert.equal((await state()).awaiting,1);assert.equal((await state()).cancelCalls,0);
   await page.evaluate(()=>probe.finish('same-url'));await waiting(0);assert(await page.evaluate(()=>probe.sites()[0].exact&&__udmCaptureV1.read('abcdefghijk')?.videoDetails.title==='same-url'));
  });
  await check('eight slow responses retain at most four actual inspection readers',async()=>{
   await page.evaluate(()=>{for(let i=0;i<8;++i)probe.start('burst-'+i);});await page.waitForTimeout(80);assert((await state()).awaiting<=4);assert((await state()).diagnostics.playerReadPeak<=4);assert.equal((await state()).sourceCancels,0);
   await page.evaluate(()=>probe.cleanup());await waiting(0);assert(await page.evaluate(()=>probe.sites().every(item=>item.exact&&item.promiseUnchanged)));
  });
  await check('accepting newer metadata cancels an older blocked inspection',async()=>{
   await page.evaluate(()=>probe.start('older'));await waiting(1);await page.evaluate(()=>probe.start('newer',{closed:true}));await waiting(0);
   assert.equal(await page.evaluate(()=>probe.sites()[0].siteDone),false);assert.equal(await page.evaluate(()=>__udmCaptureV1.read('abcdefghijk')?.videoDetails.title),'newer');
   await page.evaluate(()=>probe.finish('older'));assert.equal(await page.evaluate(()=>__udmCaptureV1.read('abcdefghijk')?.videoDetails.title),'newer');assert(await page.evaluate(()=>probe.sites().every(item=>item.exact)));
  });
  await check('byte-limit cancellation releases without awaiting the other tee branch',async()=>{
   await page.evaluate(()=>probe.start('byte-limit',{oversize:true}));await page.waitForFunction(()=>probe.state().released===1,null,{timeout:1000});assert.equal(await page.evaluate(()=>probe.sites()[0].siteDone),false);
   await page.evaluate(()=>probe.finish('byte-limit'));assert(await page.evaluate(()=>probe.sites()[0].exact));assert.equal(await page.evaluate(()=>__udmCaptureV1.read('abcdefghijk')),null);
  });
  await check('an advertised oversized response is declined before cloning',async()=>{
   await page.evaluate(()=>probe.start('advertised',{advertised:true}));await page.waitForTimeout(50);assert.equal((await state()).clones,0);await page.evaluate(()=>probe.finish('advertised'));assert(await page.evaluate(()=>probe.sites()[0].exact));
  });
  await check('the thirty-second inspection deadline releases only the clone',async()=>{
   await page.evaluate(()=>probe.start('deadline'));await waiting(1);assert.equal((await state()).deadlines,1);assert.equal(await page.evaluate(()=>probe.runDeadlines()),1);await waiting(0);assert.equal((await state()).released,1);assert.equal((await state()).sourceCancels,0);
   assert.equal(await page.evaluate(()=>probe.sites()[0].siteDone),false);await page.evaluate(()=>probe.finish('deadline'));assert(await page.evaluate(()=>probe.sites()[0].exact));
  });
  await check('a response-body error releases the inspection and preserves the site error',async()=>{
   const expectedError=await page.evaluate(()=>probe.nativeError());assert(expectedError);await page.evaluate(()=>probe.start('failed'));await waiting(1);await page.evaluate(()=>probe.fail('failed'));await waiting(0);assert.equal(await page.evaluate(()=>probe.sites()[0].error),expectedError);assert.equal((await state()).released,1);
   await page.evaluate(()=>probe.start('healthy',{closed:true}));await page.waitForFunction(()=>__udmCaptureV1.read('abcdefghijk')?.videoDetails.title==='healthy');assert(await page.evaluate(()=>probe.sites()[1].exact));
  });
  await page.close();
 }
})().catch(error=>checks.push({name:'fixture',passed:false,error:error.stack})).finally(async()=>{
 await browser?.close();const passed=checks.filter(item=>item.passed).length,failed=checks.length-passed;
 fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({passed:failed===0,passedChecks:passed,failedChecks:failed,checks,scope:'Owned Web Streams in headless Edge, both bundles and observer orders. Deadline callback invoked deterministically; no thirty-second timing claim.'},null,2));if(failed)process.exitCode=1;console.log(JSON.stringify({passed,failed}));
});
