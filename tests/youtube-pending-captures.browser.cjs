'use strict';
const {chromium}=require('playwright'),fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const root=path.resolve(process.argv[2]),checks=[];assert(!fs.existsSync(root));fs.mkdirSync(root,{recursive:true});let browser;
(async()=>{
 browser=await chromium.launch({channel:'msedge',headless:true});
 for(const bundle of ['chromium','firefox'])for(const order of ['capture-first','observer-first']){
  const page=await browser.newPage(),cdp=await page.context().newCDPSession(page),routes=new Map();let visit=0;
  const responseBytes=Buffer.from([42,13,10,11,...Buffer.from('abcdefghijk'),21,3,1,2,3]);
  await page.route('https://www.youtube.com/**',route=>route.fulfill({contentType:'text/html',body:'<!doctype html><div id="movie_player"></div>'}));
  await page.route('https://rr1.googlevideo.com/**',route=>{if(route.request().method()==='OPTIONS')return route.fulfill({status:204,headers:{'access-control-allow-origin':'*','access-control-allow-methods':'POST','access-control-allow-headers':'*'}});routes.set(new URL(route.request().url()).searchParams.get('token'),route);});
  async function setup(){
   await page.goto('https://www.youtube.com/watch?v=abcdefghijk&fixture='+(++visit));
   await page.evaluate(()=>{
    const rows=new Map(),known=new WeakSet(),copies=[],deadlines=new Map();let timer=-1000;
    const request=()=>Uint8Array.of(42,1,1,154,1,2,8,1),response=()=>Uint8Array.of(42,13,10,11,...new TextEncoder().encode('abcdefghijk'),21,3,1,2,3);
    const nativeSlice=ArrayBuffer.prototype.slice,nativeBlob=Blob.prototype.arrayBuffer,nativeTimeout=setTimeout,nativeClear=clearTimeout;
    ArrayBuffer.prototype.slice=function(...args){const value=Reflect.apply(nativeSlice,this,args);if(known.has(this))copies.push(new WeakRef(value));return value;};
    globalThis.setTimeout=function(callback,delay,...args){if(delay===30000){const id=timer--;deadlines.set(id,()=>callback(...args));return id;}return Reflect.apply(nativeTimeout,globalThis,[callback,delay,...args]);};
    globalThis.clearTimeout=function(id){if(deadlines.delete(id))return;return Reflect.apply(nativeClear,globalThis,[id]);};
    Blob.prototype.arrayBuffer=function(){for(const row of rows.values())if(this===row.blob&&row.blobPromise)return row.blobPromise;return Reflect.apply(nativeBlob,this,[]);};
    const player=document.getElementById('movie_player');player.getVideoData=()=>({video_id:'abcdefghijk'});player.getPlayerResponse=()=>undefined;
    globalThis.fetch=function(input,init){const token=new URL(typeof input==='string'?input:input.url).searchParams.get('token'),row=rows.get(token);row.argsSame=input===row.url&&init===row.init;if(row.mode==='throw')throw row.error;return row.original;};
    globalThis.probe={
     start(token,mode=''){
      const body=request();known.add(body.buffer);const row={mode,body,url:'https://rr1.googlevideo.com/videoplayback?sabr=1&token='+token,error:Error('owned fetch failure'),done:false,exact:false};
      row.original=new Promise((resolve,reject)=>{row.resolve=resolve;row.reject=reject;});row.init={method:'POST',body};
      if(mode==='blob'){row.blob=new Blob([body]);row.init.body=row.blob;row.blobPromise=new Promise(resolve=>{row.resolveBlob=resolve;});}
      rows.set(token,row);try{const promise=fetch(row.url,row.init);row.promiseSame=promise===row.original;row.site=promise.then(value=>value.arrayBuffer()).then(buffer=>{row.done=true;row.exact=Array.from(new Uint8Array(buffer)).join(',')===Array.from(response()).join(',');},error=>{row.done=true;row.errorSame=error===row.error;});}catch(error){row.thrownSame=error===row.error;row.site=Promise.resolve();}
     },
     async finish(token){const row=rows.get(token);if(row.mode==='blob'&&row.resolveBlob)this.deliverBlob(token);if(!row.done&&!row.finished){row.finished=true;row.resolve(new Response(response(),{headers:{'content-type':'application/vnd.yt-ump'}}));}await row.site;},
     reject(token){const row=rows.get(token);row.reject(row.error);return row.site;},
     deliverBlob(token){const row=rows.get(token),bytes=request().buffer;copies.push(new WeakRef(bytes));const resolve=row.resolveBlob;row.resolveBlob=null;row.blobPromise=null;resolve(bytes);},
     startXhr(token){const body=request();known.add(body.buffer);const xhr=new XMLHttpRequest();xhr.open('POST','https://rr1.googlevideo.com/videoplayback?sabr=1&token='+token);xhr.responseType='arraybuffer';const row={xhr,events:[],done:false};row.site=new Promise(resolve=>{xhr.onload=()=>{row.events.push('load');row.done=true;row.exact=Array.from(new Uint8Array(xhr.response)).join(',')===Array.from(response()).join(',');resolve();};xhr.onabort=()=>{row.events.push('abort');row.done=true;resolve();};xhr.onerror=()=>{row.events.push('error');row.done=true;resolve();};});rows.set(token,row);xhr.send(body);},
     abort(token){rows.get(token).xhr.abort();return rows.get(token).site;},
     secondSend(token){try{rows.get(token).xhr.send(request());return false;}catch(error){return error.name==='InvalidStateError';}},
     reopen(token,next){const row=rows.get(token),xhr=row.xhr;xhr.open('POST','https://rr1.googlevideo.com/not-video?token='+next);const body=request();row.done=false;row.site=new Promise(resolve=>{xhr.onload=()=>{row.done=true;row.exact=Array.from(new Uint8Array(xhr.response)).join(',')===Array.from(response()).join(',');resolve();};});xhr.send(body);},
     async cleanup(){for(const [token,row] of rows)if(!row.xhr)await this.finish(token);},
     state(){return {copies:copies.length,deadlines:deadlines.size,diagnostics:__udmCaptureV1.diagnostics()};},
     liveCopies(){return copies.filter(value=>value.deref()).length;},
     sites(){return [...rows.values()].map(({done,exact,promiseSame,argsSame,errorSame,thrownSame,events})=>({done,exact,promiseSame,argsSame,errorSame,thrownSame,events}));},
     runDeadlines(){const pending=[...deadlines];for(const [id,callback] of pending){deadlines.delete(id);callback();}return pending.length;}
    };
   });
   await page.addScriptTag({path:path.join(__dirname,'../browser',bundle,'ump.js')});
   for(const file of order==='capture-first'?['capture.js','page-observer.js']:['page-observer.js','capture.js'])await page.addScriptTag({path:path.join(__dirname,'../browser',bundle,file)});
  }
  const state=()=>page.evaluate(()=>probe.state());
  async function gc(){await page.waitForTimeout(20);await cdp.send('HeapProfiler.collectGarbage');await cdp.send('HeapProfiler.collectGarbage');return page.evaluate(()=>probe.liveCopies());}
  const wait=(fn,arg)=>page.waitForFunction(fn,arg,{timeout:1000});
  async function navigate(){await page.evaluate(()=>{history.pushState({},'', '/watch?v=12345678901');history.pushState({},'', '/watch?v=abcdefghijk');});}
  async function routeReady(token){for(let i=0;i<100&&!routes.has(token);++i)await page.waitForTimeout(10);assert(routes.has(token));}
  async function fulfill(token){await routes.get(token).fulfill({headers:{'access-control-allow-origin':'*','content-type':'application/vnd.yt-ump'},body:responseBytes});routes.delete(token);await page.waitForTimeout(30);}
  async function check(name,run){try{await setup();await run();checks.push({name:bundle+' '+order+' '+name,passed:true,observation:await state()});}catch(error){checks.push({name:bundle+' '+order+' '+name,passed:false,error:error.message,observation:await state().catch(()=>null)});}finally{await page.evaluate(()=>probe.cleanup()).catch(()=>{});for(const route of routes.values())await route.abort().catch(()=>{});routes.clear();}console.log(JSON.stringify({name:checks.at(-1).name,passed:checks.at(-1).passed,error:checks.at(-1).error}));}
  await check('navigation releases the actual copied body before headers arrive',async()=>{
   await page.evaluate(()=>probe.start('navigation'));await wait(()=>probe.state().copies===1);assert.equal(await gc(),1);await navigate();assert.equal(await gc(),0);assert.equal(await page.evaluate(()=>probe.sites()[0].done),false);await page.evaluate(()=>probe.finish('navigation'));assert(await page.evaluate(()=>probe.sites()[0].exact&&probe.sites()[0].promiseSame&&probe.sites()[0].argsSame));assert.equal(await page.evaluate(()=>__udmCaptureV1.session('abcdefghijk')),null);
  });
  await check('twelve header-stalled requests retain at most eight copied bodies',async()=>{
   await page.evaluate(()=>{for(let i=0;i<12;++i)probe.start('burst-'+i);});await wait(()=>probe.state().copies===12);assert((await gc())<=8);assert((await state()).diagnostics.streamTaskPeak<=8);assert((await state()).diagnostics.streamTaskBytes<=64);await page.evaluate(()=>probe.cleanup());assert(await page.evaluate(()=>probe.sites().every(row=>row.exact&&row.promiseSame&&row.argsSame)));assert.equal(await gc(),0);
  });
  await check('a pending-header deadline clears copied bytes without finishing the site request',async()=>{
   await page.evaluate(()=>probe.start('deadline'));await wait(()=>probe.state().copies===1);assert.equal((await state()).deadlines,1);await page.evaluate(()=>probe.runDeadlines());assert.equal(await gc(),0);assert.equal(await page.evaluate(()=>probe.sites()[0].done),false);await page.evaluate(()=>probe.finish('deadline'));assert.equal(await page.evaluate(()=>__udmCaptureV1.session('abcdefghijk')),null);
  });
  await check('newly verified capture releases older pending-header snapshots',async()=>{
   await page.evaluate(()=>{probe.start('old');probe.start('new');});await page.evaluate(()=>probe.finish('new'));await wait(()=>__udmCaptureV1.session('abcdefghijk')?.url.includes('token=new'));assert.equal(await gc(),0);assert.equal(await page.evaluate(()=>probe.sites()[0].done),false);await page.evaluate(()=>probe.finish('old'));assert(await page.evaluate(()=>__udmCaptureV1.session('abcdefghijk')?.url.includes('token=new')));
  });
  await check('late body production cannot restore a retired capture snapshot',async()=>{
   await page.evaluate(()=>probe.start('blob','blob'));await navigate();await page.evaluate(()=>probe.deliverBlob('blob'));await page.waitForTimeout(30);assert.equal(await gc(),0);await page.evaluate(()=>probe.finish('blob'));assert(await page.evaluate(()=>probe.sites()[0].exact));assert.equal(await page.evaluate(()=>__udmCaptureV1.session('abcdefghijk')),null);
  });
  for(const mode of ['reject','throw'])await check('fetch '+mode+' leaves no copied request snapshot',async()=>{
   await page.evaluate(mode=>probe.start(mode,mode),mode);if(mode==='reject')await page.evaluate(mode=>probe.reject(mode),mode);assert.equal(await gc(),0);assert(await page.evaluate(mode=>mode==='reject'?probe.sites()[0].errorSame:probe.sites()[0].thrownSame,mode));
  });
  await check('navigation releases an XHR snapshot and keeps the caller load handler',async()=>{
   await page.evaluate(()=>probe.startXhr('xhr-nav'));await routeReady('xhr-nav');assert.equal(await gc(),1);await navigate();assert.equal(await gc(),0);await fulfill('xhr-nav');assert(await page.evaluate(()=>probe.sites()[0].done&&probe.sites()[0].exact));assert.equal(await page.evaluate(()=>__udmCaptureV1.session('abcdefghijk')),null);
  });
  await check('XHR abort releases its snapshot and preserves the caller abort event',async()=>{
   await page.evaluate(()=>probe.startXhr('xhr-abort'));await routeReady('xhr-abort');await page.evaluate(()=>probe.abort('xhr-abort'));assert.equal(await gc(),0);assert.deepEqual(await page.evaluate(()=>probe.sites()[0].events),['abort']);
  });
  await check('XHR reopen removes the old capture listener from an unrelated response',async()=>{
   await page.evaluate(()=>probe.startXhr('xhr-old'));await routeReady('xhr-old');await page.evaluate(()=>probe.reopen('xhr-old','xhr-other'));await routeReady('xhr-other');assert.equal(await gc(),0);await fulfill('xhr-other');assert(await page.evaluate(()=>probe.sites()[0].done&&probe.sites()[0].exact));assert.equal(await page.evaluate(()=>__udmCaptureV1.session('abcdefghijk')),null);
  });
  await check('an invalid second XHR send preserves the first capture and native error',async()=>{
   await page.evaluate(()=>probe.startXhr('xhr-send'));await routeReady('xhr-send');
   assert(await page.evaluate(()=>probe.secondSend('xhr-send')));assert.equal((await state()).copies,1);await fulfill('xhr-send');await wait(()=>!!__udmCaptureV1.session('abcdefghijk'));assert(await page.evaluate(()=>probe.sites()[0].exact));assert.equal(await gc(),0);
  });
  await check('a large XHR is inspected through a bounded prefix while the caller receives every byte',async()=>{
   const bytes=Buffer.alloc(1048576);responseBytes.copy(bytes,0,0,15);bytes.set([1,1,0],15);for(let i=18;i<bytes.length;i+=2){bytes[i]=1;bytes[i+1]=0;}
   await page.evaluate(()=>{globalThis.prefixCopies=[];const native=Response;globalThis.Response=new Proxy(native,{construct(target,args,newTarget){prefixCopies.push(args[0]?.byteLength||0);return Reflect.construct(target,args,newTarget);}});globalThis.largeXhrDone=new Promise((resolve,reject)=>{const xhr=new XMLHttpRequest();xhr.open('POST','https://rr1.googlevideo.com/videoplayback?sabr=1&token=xhr-large');xhr.responseType='arraybuffer';xhr.onload=async()=>{const digest=await crypto.subtle.digest('SHA-256',xhr.response);resolve({length:xhr.response.byteLength,sha256:Array.from(new Uint8Array(digest),value=>value.toString(16).padStart(2,'0')).join('')});};xhr.onerror=reject;xhr.send(Uint8Array.of(42,1,1,154,1,2,8,1));});});
   await routeReady('xhr-large');await routes.get('xhr-large').fulfill({headers:{'access-control-allow-origin':'*','content-type':'application/vnd.yt-ump'},body:bytes});routes.delete('xhr-large');
   const delivered=await page.evaluate(()=>largeXhrDone);assert.equal(delivered.length,bytes.length);assert.equal(delivered.sha256,require('node:crypto').createHash('sha256').update(bytes).digest('hex'));
   await wait(()=>!!__udmCaptureV1.session('abcdefghijk'));assert.deepEqual(await page.evaluate(()=>prefixCopies),[262144]);assert.equal(await gc(),0);
  });
  await page.close();
 }
})().catch(error=>checks.push({name:'fixture',passed:false,error:error.stack})).finally(async()=>{
 await browser?.close();const passed=checks.filter(item=>item.passed).length,failed=checks.length-passed;
 fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({passed:failed===0,passedChecks:passed,failedChecks:failed,checks,scope:'Owned fetch promises, real XHR routes and native Web Streams in headless Edge. Weak references and forced GC check actual observer copies. Deterministic deadline callback; no real video download.'},null,2));if(failed)process.exitCode=1;console.log(JSON.stringify({passed,failed}));
});
