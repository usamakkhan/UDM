'use strict';
const {chromium}=require('playwright'),fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const root=path.resolve(process.argv[2]),checks=[];assert(!fs.existsSync(root));fs.mkdirSync(root,{recursive:true});let browser;
(async()=>{
 browser=await chromium.launch({channel:'msedge',headless:true});
 for(const bundle of ['chromium','firefox'])for(const order of ['capture-first','observer-first']){
  const page=await browser.newPage(),pendingXhr=new Map();let visit=0;
  await page.route('https://www.youtube.com/**',route=>route.fulfill({contentType:'text/html',body:'<!doctype html><div id="movie_player"></div>'}));
  await page.route('https://rr1.googlevideo.com/**',route=>{if(route.request().method()==='OPTIONS')return route.fulfill({status:204,headers:{'access-control-allow-origin':'*','access-control-allow-methods':'POST','access-control-allow-headers':'*'}});pendingXhr.set(new URL(route.request().url()).searchParams.get('token'),route);});
  async function setup(){
   await page.goto('https://www.youtube.com/watch?v=abcdefghijk&fixture='+(++visit));
   await page.evaluate(()=>{
    const items=new Map(),deadlines=new Map();let timer=-1000;
    const counts=()=>({clones:0,opened:0,released:0,awaiting:0,cancels:0,sourceCancels:0});const counters={request:counts(),response:counts()};
    const nativeTimeout=setTimeout,nativeClear=clearTimeout;
    globalThis.setTimeout=function(callback,delay,...args){if(delay===30000){const id=timer--;deadlines.set(id,()=>callback(...args));return id;}return Reflect.apply(nativeTimeout,globalThis,[callback,delay,...args]);};
    globalThis.clearTimeout=function(id){if(deadlines.delete(id))return;return Reflect.apply(nativeClear,globalThis,[id]);};
    for(const [type,kind] of [[Request,'request'],[Response,'response']]){const clone=type.prototype.clone;type.prototype.clone=function(){const copy=Reflect.apply(clone,this,[]),counter=counters[kind];++counter.clones;const body=copy.body;if(body){const get=body.getReader;body.getReader=function(...args){const reader=Reflect.apply(get,this,args);++counter.opened;const read=reader.read,cancel=reader.cancel,release=reader.releaseLock;reader.read=function(...args){++counter.awaiting;return Reflect.apply(read,this,args).finally(()=>--counter.awaiting);};reader.cancel=function(...args){++counter.cancels;return Reflect.apply(cancel,this,args);};reader.releaseLock=function(...args){++counter.released;return Reflect.apply(release,this,args);};return reader;};}return copy;};}
    const player=document.getElementById('movie_player');player.getVideoData=()=>({video_id:'abcdefghijk'});player.getPlayerResponse=()=>undefined;
    const requestBytes=Uint8Array.of(42,1,1,154,1,2,8,1),identity=Uint8Array.of(42,13,10,11,...new TextEncoder().encode('abcdefghijk'));
    function stream(item,kind,bytes,split,closed){item[kind+'Bytes']=bytes;item[kind+'Split']=split;item[kind+'Finished']=closed;return new ReadableStream({start(controller){item[kind+'Controller']=controller;controller.enqueue(closed?bytes:bytes.slice(0,split));if(closed)controller.close();},cancel(){++counters[kind].sourceCancels;}});}
    function finish(item,kind){if(!item[kind+'Finished']){item[kind+'Controller'].enqueue(item[kind+'Bytes'].slice(item[kind+'Split']));item[kind+'Controller'].close();item[kind+'Finished']=true;}}
    function exact(buffer,expected){const bytes=new Uint8Array(buffer);return bytes.length===expected.length&&bytes.every((value,i)=>value===expected[i]);}
    globalThis.fetch=function(input,init){const url=typeof input==='string'?input:input.url,item=items.get(new URL(url).searchParams.get('token'));if(!item)throw Error('Unknown owned request');item.argsPreserved=input===item.input&&init===item.init;
     item.siteRequest=(input instanceof Request?input.arrayBuffer():init.body instanceof Blob?new Response(init.body).arrayBuffer():Promise.resolve(init.body.buffer.slice(init.body.byteOffset,init.body.byteOffset+init.body.byteLength))).then(bytes=>{item.requestDone=true;item.requestExact=exact(bytes,item.requestBytes);});
     if(item.mode==='throw')throw item.fetchError;
     return item.originalPromise;
    };
    globalThis.probe={
     start(token,{streamRequest=false,requestClosed=false,responseClosed=false,responseIdentity=false,mode='',oversize=false,blob=false,mime='application/vnd.yt-ump'}={}){
      const item={mode,requestDone:false,responseDone:false,requestExact:false,responseExact:false,fetchError:Error('owned fetch failure')};
      const body=oversize?new Uint8Array(131073).fill(1):requestBytes;item.requestBytes=body;
      const response=new Uint8Array(identity.length+5);response.set(identity);response.set([21,3,1,2,3],identity.length);
      const responseBody=stream(item,'response',response,responseIdentity?identity.length:5,responseClosed);
      item.originalPromise=mode==='reject'?Promise.reject(item.fetchError):Promise.resolve(new Response(responseBody,{headers:{'content-type':mime}}));
      const url='https://rr1.googlevideo.com/videoplayback?sabr=1&token='+token;
      if(streamRequest){item.input=new Request(url,{method:'POST',body:stream(item,'request',body,oversize?body.length:3,requestClosed),duplex:'half'});item.init=undefined;}
      else{item.input=url;let supplied=body;if(blob){supplied=new Blob([body]);supplied.arrayBuffer=()=>new Promise(()=>{});}item.init={method:'POST',body:supplied};}
      items.set(token,item);
      try{const promise=fetch(item.input,item.init);item.promisePreserved=promise===item.originalPromise;item.siteResponse=promise.then(value=>value.arrayBuffer()).then(bytes=>{item.responseDone=true;item.responseExact=exact(bytes,response);},error=>{item.responseDone=true;item.errorPreserved=error===item.fetchError;});}
      catch(error){item.thrownPreserved=error===item.fetchError;item.siteResponse=Promise.resolve();}
     },
     async finishRequest(token){const item=items.get(token);if(item.input instanceof Request)finish(item,'request');await item.siteRequest;},
     async finishResponse(token){const item=items.get(token);finish(item,'response');await item.siteResponse;},
     async cleanup(){for(const [token,item] of items){await this.finishRequest(token);await this.finishResponse(token);}},
     state(){return {request:{...counters.request},response:{...counters.response},deadlines:deadlines.size,diagnostics:__udmCaptureV1.diagnostics()};},
     sites(){return [...items.values()].map(({requestDone,responseDone,requestExact,responseExact,argsPreserved,promisePreserved,errorPreserved,thrownPreserved})=>({requestDone,responseDone,requestExact,responseExact,argsPreserved,promisePreserved,errorPreserved,thrownPreserved}));},
     runDeadlines(){const pending=[...deadlines];for(const [id,callback] of pending){deadlines.delete(id);callback();}return pending.length;}
    };
   });
   await page.addScriptTag({path:path.join(__dirname,'../browser',bundle,'ump.js')});
   for(const file of order==='capture-first'?['capture.js','page-observer.js']:['page-observer.js','capture.js'])await page.addScriptTag({path:path.join(__dirname,'../browser',bundle,file)});
  }
  const state=()=>page.evaluate(()=>probe.state());
  const wait=(predicate,arg)=>page.waitForFunction(predicate,arg,{timeout:1000});
  const exactSites=()=>page.evaluate(()=>probe.sites().every(item=>item.requestExact&&item.responseExact&&item.argsPreserved&&item.promisePreserved));
  async function navigate(){await page.evaluate(()=>{history.pushState({},'', '/watch?v=12345678901');history.pushState({},'', '/watch?v=abcdefghijk');});}
  async function check(name,run){let observation;try{await setup();await run();observation=await state();checks.push({name:bundle+' '+order+' '+name,passed:true,observation});}catch(error){observation=await state().catch(()=>null);checks.push({name:bundle+' '+order+' '+name,passed:false,error:error.message,observation});}finally{await page.evaluate(()=>probe.cleanup()).catch(()=>{});}console.log(JSON.stringify({name:checks.at(-1).name,passed:checks.at(-1).passed,error:checks.at(-1).error}));}
  await check('navigation releases a blocked streaming response clone',async()=>{
   await page.evaluate(()=>probe.start('response'));await wait(()=>probe.state().response.awaiting===1);await navigate();await wait(()=>probe.state().response.released===1);
   assert.equal(await page.evaluate(()=>probe.sites()[0].responseDone),false);assert.equal((await state()).response.sourceCancels,0);await page.evaluate(()=>probe.cleanup());assert(await exactSites());assert.equal(await page.evaluate(()=>__udmCaptureV1.session('abcdefghijk')),null);
  });
  await check('navigation releases both clones while the original upload remains unfinished',async()=>{
   await page.evaluate(()=>probe.start('upload',{streamRequest:true}));await wait(()=>probe.state().request.awaiting===1&&probe.state().response.opened===1);await navigate();await wait(()=>probe.state().request.released===1&&probe.state().response.released===1);
   assert(await page.evaluate(()=>!probe.sites()[0].requestDone&&!probe.sites()[0].responseDone));assert.equal((await state()).request.sourceCancels,0);await page.evaluate(()=>probe.cleanup());assert(await exactSites());
  });
  await check('eight blocked captures retain at most four request and four response inspections',async()=>{
   await page.evaluate(()=>{for(let i=0;i<8;++i)probe.start('burst-'+i,{streamRequest:true});});await page.waitForTimeout(80);
   assert((await state()).request.awaiting<=4);assert((await state()).response.opened-(await state()).response.released<=4);assert((await state()).diagnostics.requestReadPeak<=4);assert((await state()).diagnostics.sabrReadPeak<=4);
   await page.evaluate(()=>probe.cleanup());assert(await exactSites());assert.equal((await state()).deadlines,0);
  });
  await check('newly verified streaming metadata retires older response inspection',async()=>{
   await page.evaluate(()=>probe.start('older'));await wait(()=>probe.state().response.awaiting===1);await page.evaluate(()=>probe.start('newer',{responseIdentity:true}));await wait(()=>__udmCaptureV1.session('abcdefghijk')?.url.includes('token=newer'));
   await wait(()=>probe.state().response.awaiting===0);assert.equal(await page.evaluate(()=>probe.sites()[0].responseDone),false);await page.evaluate(()=>probe.cleanup());assert(await exactSites());
  });
  await check('equal timestamps cannot let an older verified session replace the newer request',async()=>{
   await page.evaluate(()=>{const fixed=Date.now();Date.now=()=>fixed;probe.start('old-tie');probe.start('new-tie',{responseIdentity:true});});await wait(()=>__udmCaptureV1.session('abcdefghijk')?.url.includes('token=new-tie'));await page.evaluate(()=>probe.cleanup());
   assert(await page.evaluate(()=>__udmCaptureV1.session('abcdefghijk')?.url.includes('token=new-tie')));assert(await exactSites());
  });
  await check('a deadline releases upload and response clones before either original body ends',async()=>{
   await page.evaluate(()=>probe.start('deadline',{streamRequest:true}));await wait(()=>probe.state().request.awaiting===1&&probe.state().response.opened===1);assert((await state()).deadlines>=1);await page.evaluate(()=>probe.runDeadlines());await wait(()=>probe.state().request.released===1&&probe.state().response.released===1);
   assert(await page.evaluate(()=>!probe.sites()[0].requestDone&&!probe.sites()[0].responseDone));await page.evaluate(()=>probe.cleanup());assert(await exactSites());assert.equal(await page.evaluate(()=>__udmCaptureV1.session('abcdefghijk')),null);
  });
  await check('an unrelated response releases its request inspection without cloning the response',async()=>{
   await page.evaluate(()=>probe.start('wrong-type',{streamRequest:true,mime:'text/html'}));await wait(()=>probe.state().request.released===1);assert.equal((await state()).response.clones,0);assert.equal(await page.evaluate(()=>probe.sites()[0].requestDone),false);await page.evaluate(()=>probe.cleanup());assert(await exactSites());
  });
  for(const mode of ['reject','throw'])await check('original fetch '+mode+' releases its request clone and preserves the error',async()=>{
   await page.evaluate(mode=>probe.start(mode,{streamRequest:true,mode}),mode);await wait(()=>probe.state().request.released===1);assert.equal(await page.evaluate(()=>probe.sites()[0].requestDone),false);assert(await page.evaluate(mode=>mode==='throw'?probe.sites()[0].thrownPreserved:probe.sites()[0].errorPreserved,mode));await page.evaluate(()=>probe.cleanup());assert(await page.evaluate(()=>probe.sites()[0].requestExact&&probe.sites()[0].argsPreserved));
  });
  await check('a valid streamed request is captured exactly before the original response ends',async()=>{
   await page.evaluate(()=>probe.start('healthy',{streamRequest:true,responseIdentity:true}));await wait(()=>probe.state().request.awaiting===1);await page.evaluate(()=>probe.finishRequest('healthy'));await wait(()=>!!__udmCaptureV1.session('abcdefghijk'));
   assert.equal(await page.evaluate(()=>__udmCaptureV1.session('abcdefghijk').body),Buffer.from([42,1,1,154,1,2,8,1]).toString('base64'));assert.equal(await page.evaluate(()=>probe.sites()[0].responseDone),false);await page.evaluate(()=>probe.cleanup());assert(await exactSites());assert.equal((await state()).deadlines,0);
  });
  await check('oversized streamed requests release their clone and preserve the original upload',async()=>{
   await page.evaluate(()=>probe.start('oversized',{streamRequest:true,oversize:true}));await wait(()=>probe.state().request.released===1);assert.equal(await page.evaluate(()=>probe.sites()[0].requestDone),false);await page.evaluate(()=>probe.cleanup());assert(await exactSites());assert.equal(await page.evaluate(()=>__udmCaptureV1.session('abcdefghijk')),null);
  });
  await check('Blob inspection uses the native body instead of a page override',async()=>{
   await page.evaluate(()=>probe.start('blob',{blob:true,responseIdentity:true}));await wait(()=>!!__udmCaptureV1.session('abcdefghijk'));await page.evaluate(()=>probe.cleanup());assert(await exactSites());assert.equal((await state()).deadlines,0);
  });
  await check('real XHR captures retain the newer request at equal timestamps',async()=>{
   await page.evaluate(()=>{const fixed=Date.now();Date.now=()=>fixed;globalThis.xhrResults={};for(const name of ['xhr-old','xhr-new']){const xhr=new XMLHttpRequest();xhr.open('POST','https://rr1.googlevideo.com/videoplayback?sabr=1&token='+name);xhr.responseType='arraybuffer';xhrResults[name]=new Promise((resolve,reject)=>{xhr.onload=()=>resolve(Array.from(new Uint8Array(xhr.response)));xhr.onerror=()=>reject(Error('Owned XHR failed'));});xhr.send(Uint8Array.of(42,1,1,154,1,2,8,1));}});
   for(let i=0;i<100&&pendingXhr.size<2;++i)await page.waitForTimeout(10);assert.equal(pendingXhr.size,2);
   const bytes=Buffer.from([42,13,10,11,...Buffer.from('abcdefghijk'),21,3,1,2,3]);
   for(const name of ['xhr-new','xhr-old']){await pendingXhr.get(name).fulfill({headers:{'content-type':'application/vnd.yt-ump','access-control-allow-origin':'*'},body:bytes});pendingXhr.delete(name);assert.deepEqual(await page.evaluate(name=>xhrResults[name],name),Array.from(bytes));if(name==='xhr-new')await wait(()=>__udmCaptureV1.session('abcdefghijk')?.url.includes('token=xhr-new'));}
   await page.waitForTimeout(30);assert(await page.evaluate(()=>__udmCaptureV1.session('abcdefghijk')?.url.includes('token=xhr-new')));
  });
  for(const route of pendingXhr.values())await route.abort().catch(()=>{});
  await page.close();
 }
})().catch(error=>checks.push({name:'fixture',passed:false,error:error.stack})).finally(async()=>{
 await browser?.close();const passed=checks.filter(item=>item.passed).length,failed=checks.length-passed;
 fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({passed:failed===0,passedChecks:passed,failedChecks:failed,checks,scope:'Owned Web Streams and valid bounded UMP identity/request fixtures in headless Edge. Both bundles and observer orders; deterministic deadline callback, no real video payload.'},null,2));if(failed)process.exitCode=1;console.log(JSON.stringify({passed,failed}));
});
