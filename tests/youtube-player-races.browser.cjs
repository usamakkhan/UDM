'use strict';
const {chromium}=require('playwright');
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const root=path.resolve(process.argv[2]),id='abcdefghijk',other='12345678901',checks=[];
assert(!fs.existsSync(root),'Use a fresh output directory');fs.mkdirSync(root,{recursive:true});
const data=title=>({videoDetails:{videoId:id,title},streamingData:{adaptiveFormats:[{itag:137,height:1080,lastModified:title==='New'?'2':'1',url:'https://rr1.googlevideo.com/videoplayback?id='+title+'&itag=137'}]}});
let browser;
(async()=>{
 browser=await chromium.launch({channel:'msedge',headless:true});
 for(const bundle of ['chromium','firefox'])for(const order of ['capture-first','observer-first']){
  const page=await browser.newPage(),pending=new Map();let serial=0;
  await page.route('https://www.youtube.com/**',async route=>{
   const url=new URL(route.request().url());
   if(url.pathname==='/youtubei/v1/player'||url.pathname==='/not-player'){pending.set(url.searchParams.get('token'),route);return;}
   await route.fulfill({contentType:'text/html',body:'<!doctype html><div id="movie_player"></div>'});
  });
  async function setup(initial=false){
   await page.goto('https://www.youtube.com/watch?v='+id+'&fixture='+(++serial));
   await page.evaluate(({id,initial})=>{
    globalThis.playerResponse=undefined;
    const player=document.getElementById('movie_player');player.getVideoData=()=>({video_id:new URL(location.href).searchParams.get('v')});player.getPlayerResponse=()=>playerResponse;
    if(initial)globalThis.ytInitialPlayerResponse={videoDetails:{videoId:id,title:'Initial'},streamingData:{adaptiveFormats:[{itag:137,height:1080,url:'https://rr1.googlevideo.com/videoplayback?id=Initial&itag=137'}]}};
    const original=fetch;globalThis.fetch=function(...args){
     if(String(args[0]).includes('stream=fixture')){
      const text=JSON.stringify({videoDetails:{videoId:id,title:'Old stream'},streamingData:{formats:[{itag:18}]}}),cut=Math.floor(text.length/2);
      const stream=new ReadableStream({start(controller){controller.enqueue(new TextEncoder().encode(text.slice(0,cut)));globalThis.finishStream=()=>{controller.enqueue(new TextEncoder().encode(text.slice(cut)));controller.close();};}});
      return globalThis.sitePromise=Promise.resolve(new Response(stream,{headers:{'content-type':'application/json'}}));
     }
     return globalThis.sitePromise=Reflect.apply(original,this,args);
    };
   },{id,initial});
   for(const file of order==='capture-first'?['capture.js','page-observer.js']:['page-observer.js','capture.js'])await page.addScriptTag({path:path.join(__dirname,'../browser',bundle,file)});
  }
  async function start(token,kind='fetch',target='/youtubei/v1/player'){
   await page.evaluate(({token,kind,target})=>{
    globalThis.requests??={};const url=target+'?token='+token;
    if(kind==='fetch'){const promise=fetch(url,{method:'POST',body:'fixture request'});requests[token]={samePromise:promise===sitePromise,done:promise.then(response=>response.text())};}
    else{const xhr=new XMLHttpRequest();xhr.open('GET',url);xhr.responseType=kind==='json'?'json':'text';requests[token]={xhr,done:new Promise((resolve,reject)=>{xhr.onload=()=>resolve(xhr.responseType==='json'?JSON.stringify(xhr.response):xhr.responseText);xhr.onerror=reject;})};xhr.send();}
   },{token,kind,target});
   for(let i=0;i<100&&!pending.has(token);++i)await page.waitForTimeout(10);assert(pending.has(token),'Owned API request reached routing fixture');
  }
  async function finish(token,title,status=200,videoId=id){
   const value=data(title);value.videoDetails.videoId=videoId;
   await pending.get(token).fulfill({status,contentType:'application/json',body:JSON.stringify(value)});pending.delete(token);
   const original=await page.evaluate(token=>requests[token].done,token);assert.equal(JSON.parse(original).videoDetails.title,title,'Original response stays intact');
   await page.waitForTimeout(60);
  }
  const title=()=>page.evaluate(id=>__udmCaptureV1.read(id)?.videoDetails.title??null,id);
  async function check(name,run){try{await run();checks.push({name:bundle+' '+order+' '+name,passed:true});}catch(error){checks.push({name:bundle+' '+order+' '+name,passed:false,error:error.message});}console.log(JSON.stringify(checks.at(-1)));}
  await check('newest player request wins reversed fetch completion',async()=>{
   await setup();await start('old');await start('new');await finish('new','New');assert.equal(await title(),'New');await finish('old','Old');assert.equal(await title(),'New');
   assert(await page.evaluate(()=>requests.old.samePromise&&requests.new.samePromise));
  });
  await check('same-video route revisit rejects an earlier fetch',async()=>{
   await setup();await start('visit-old');await page.evaluate(other=>{history.pushState({},'', '/watch?v='+other);history.pushState({},'', '/watch?v=abcdefghijk');},other);
   await start('visit-new');await finish('visit-new','New');await finish('visit-old','Old');assert.equal(await title(),'New');
  });
  await check('navigation while reading a cloned body rejects its metadata',async()=>{
   await setup();await page.evaluate(()=>{const promise=fetch('/youtubei/v1/player?stream=fixture');globalThis.streamingPromisePreserved=promise===sitePromise;globalThis.streamDone=promise.then(response=>response.text());});await page.waitForTimeout(30);
   await page.evaluate(other=>{history.pushState({},'', '/watch?v='+other);history.pushState({},'', '/watch?v=abcdefghijk');},other);
   await start('stream-new');await finish('stream-new','New');await page.evaluate(()=>finishStream());
   assert.equal(JSON.parse(await page.evaluate(()=>streamDone)).videoDetails.title,'Old stream');await page.waitForTimeout(60);assert.equal(await title(),'New');assert(await page.evaluate(()=>streamingPromisePreserved));
  });
  for(const kind of ['text','json'])await check('same-route navigation rejects earlier '+kind+' XHR',async()=>{
   await setup();await start('xhr-old',kind);await page.evaluate(()=>document.dispatchEvent(new Event('yt-navigate-start')));await start('xhr-new',kind);await finish('xhr-new','New');await finish('xhr-old','Old');assert.equal(await title(),'New');
  });
  await check('reopening an unsent XHR cannot retain its old player listener',async()=>{
   await setup();await page.evaluate(()=>{const xhr=new XMLHttpRequest();xhr.open('GET','/youtubei/v1/player?token=unused');xhr.open('GET','/not-player?token=reopened');globalThis.requests={reopened:{done:new Promise(resolve=>xhr.onload=()=>resolve(xhr.responseText))}};xhr.send();});
   for(let i=0;i<100&&!pending.has('reopened');++i)await page.waitForTimeout(10);assert(pending.has('reopened'));await finish('reopened','Unrelated');assert.equal(await title(),null);
  });
  await check('initial snapshot cannot overwrite accepted API metadata',async()=>{
   await setup(true);assert.equal(await title(),'Initial');await start('initial-new');await finish('initial-new','New');assert.equal(await title(),'New');
  });
  await check('earlier page objects cannot reappear after a visit changes',async()=>{
   await setup(true);await page.evaluate(()=>{playerResponse=ytInitialPlayerResponse;});assert.equal(await title(),'Initial');
   await page.evaluate(other=>{history.pushState({},'', '/watch?v='+other);history.pushState({},'', '/watch?v=abcdefghijk');},other);assert.equal(await title(),null);
   await page.evaluate(value=>{playerResponse=value;},data('New'));assert.equal(await title(),'New');
  });
  await check('same URL and rejected history updates preserve the catalog and exceptions',async()=>{
   await setup();await start('stable');await finish('stable','New');
   assert(await page.evaluate(()=>{const before=__udmCaptureV1.visit('abcdefghijk');history.replaceState({fixture:true},'',location.href);try{history.pushState({},'', 'https://different.invalid/');return false;}catch(error){return error.name==='SecurityError'&&__udmCaptureV1.visit('abcdefghijk')===before;}}));assert.equal(await title(),'New');
  });
  await check('a player response for another video cannot enter this visit',async()=>{
   await setup();await start('mismatch');await finish('mismatch','Unrelated',200,other);assert.equal(await title(),null);assert.equal(await page.evaluate(other=>__udmCaptureV1.read(other),other),null);
  });
  await check('an unsuccessful XHR response cannot become playable metadata',async()=>{
   await setup();await start('failed-xhr','json');await finish('failed-xhr','Unsuccessful',500);assert.equal(await title(),null);
  });
  await check('XHR ownership begins at send after open precedes navigation',async()=>{
   await setup();await page.evaluate(other=>{const xhr=new XMLHttpRequest();xhr.open('GET','/youtubei/v1/player?token=send-after-route');globalThis.requests={'send-after-route':{xhr,done:new Promise(resolve=>xhr.onload=()=>resolve(xhr.responseText))}};history.pushState({},'', '/watch?v='+other);history.pushState({},'', '/watch?v=abcdefghijk');xhr.send();},other);
   for(let i=0;i<100&&!pending.has('send-after-route');++i)await page.waitForTimeout(10);assert(pending.has('send-after-route'));await finish('send-after-route','New');assert.equal(await title(),'New');
  });
  await check('back and forward visits retire an outstanding request',async()=>{
   await setup();await page.evaluate(other=>{history.pushState({},'', '/watch?v='+other);history.pushState({},'', '/watch?v=abcdefghijk');},other);await start('history-old');
   await page.evaluate(()=>history.back());await page.waitForURL('**/watch?v='+other);await page.evaluate(()=>history.forward());await page.waitForURL('**/watch?v='+id);
   await start('history-new');await finish('history-new','New');await finish('history-old','Old');assert.equal(await title(),'New');
  });
  await check('fresh clones of stale getter metadata cannot replace accepted API fields',async()=>{
   await setup();await page.evaluate(value=>{globalThis.stale=value;document.getElementById('movie_player').getPlayerResponse=()=>structuredClone(stale);},data('Old'));
   assert.equal(await title(),'Old');await start('authority-new');await finish('authority-new','New');
   assert.equal(await title(),'New');assert((await page.evaluate(id=>__udmCaptureV1.read(id).streamingData.adaptiveFormats[0].url,id)).includes('id=New'));
  });
  await check('new API revisions replace authority without a stale getter rolling them back',async()=>{
   await setup();await page.evaluate(value=>{playerResponse=value;},data('Old'));await title();await start('authority-first');await finish('authority-first','New');
   await start('authority-next');await finish('authority-next','Newest');assert.equal(await title(),'Newest');
   assert((await page.evaluate(id=>__udmCaptureV1.read(id).streamingData.adaptiveFormats[0].url,id)).includes('id=Newest'));
  });
  await check('snapshot-only formats can change revision without overriding API formats',async()=>{
   await setup();await start('authority-add');await finish('authority-add','New');
   await page.evaluate(value=>{playerResponse=value;playerResponse.streamingData.adaptiveFormats.push({itag:140,lastModified:'1',url:'https://rr1.googlevideo.com/videoplayback?itag=140&old=1'});},data('Old'));await title();
   await page.evaluate(()=>{const audio=playerResponse.streamingData.adaptiveFormats[1];audio.lastModified='2';delete audio.url;});
   const formats=await page.evaluate(id=>__udmCaptureV1.read(id).streamingData.adaptiveFormats,id);
   assert.equal(formats.find(f=>f.itag===140).url,undefined);assert(formats.find(f=>f.itag===137).url.includes('id=New'));assert.equal(await title(),'New');
  });
  await check('getter additions cannot evict API formats at the catalog bound',async()=>{
   await setup();await start('authority-cap');await finish('authority-cap','New');
   await page.evaluate(value=>{playerResponse=value;playerResponse.streamingData.adaptiveFormats=Array.from({length:200},(_,i)=>({itag:1000+i,url:'https://rr1.googlevideo.com/videoplayback?itag='+(1000+i)}));},data('Old'));
   const formats=await page.evaluate(id=>__udmCaptureV1.read(id).streamingData.adaptiveFormats,id);assert.equal(formats.length,200);assert(formats.find(f=>f.itag===137).url.includes('id=New'));
  });
  await check('stale getters cannot change lifecycle after accepted API content',async()=>{
   await setup();await start('authority-live');await finish('authority-live','New');
   await page.evaluate(value=>{playerResponse=value;playerResponse.videoDetails.isUpcoming=true;},data('Old'));
   assert.equal(await title(),'New');assert.equal(await page.evaluate(id=>__udmCaptureV1.read(id).videoDetails.isUpcoming,id),false);
  });
  await check('a reused response object with a newer media revision can enter the returning visit',async()=>{
   await setup();await page.evaluate(value=>{playerResponse=value;},data('Old'));assert.equal(await title(),'Old');
   await page.evaluate(other=>{history.pushState({},'', '/watch?v='+other);history.pushState({},'', '/watch?v=abcdefghijk');playerResponse.videoDetails.title='New';playerResponse.streamingData.adaptiveFormats[0].lastModified='2';playerResponse.streamingData.adaptiveFormats[0].url='https://rr1.googlevideo.com/videoplayback?id=New&itag=137';},other);
   assert.equal(await title(),'New');
  });
  await check('title-only edits do not make a retired response fresh',async()=>{
   await setup();await page.evaluate(value=>{playerResponse=value;},data('Old'));await title();
   await page.evaluate(other=>{history.pushState({},'', '/watch?v='+other);history.pushState({},'', '/watch?v=abcdefghijk');playerResponse.videoDetails.title='New';},other);assert.equal(await title(),null);
  });
  await check('a backwards media revision cannot refresh a retired response',async()=>{
   await setup();await page.evaluate(value=>{playerResponse=value;},data('New'));await title();
   await page.evaluate(other=>{history.pushState({},'', '/watch?v='+other);history.pushState({},'', '/watch?v=abcdefghijk');playerResponse.streamingData.adaptiveFormats[0].lastModified='1';},other);assert.equal(await title(),null);
  });
  await check('a newer revision on a retired object waits until content playback is current',async()=>{
   await setup();await page.evaluate(value=>{playerResponse=value;},data('Old'));await title();
   await page.evaluate(other=>{history.pushState({},'', '/watch?v='+other);history.pushState({},'', '/watch?v=abcdefghijk');playerResponse.streamingData.adaptiveFormats[0].lastModified='2';document.getElementById('movie_player').classList.add('ad-showing');},other);assert.equal(await title(),null);
   await page.evaluate(()=>document.getElementById('movie_player').classList.remove('ad-showing'));assert.equal(await title(),'Old');
  });
  await check('revision comparison preserves increments beyond JavaScript integer precision',async()=>{
   await setup();await page.evaluate(value=>{playerResponse=value;playerResponse.streamingData.adaptiveFormats[0].lastModified='9007199254740992';},data('Old'));await title();
   await page.evaluate(other=>{history.pushState({},'', '/watch?v='+other);history.pushState({},'', '/watch?v=abcdefghijk');playerResponse.videoDetails.title='New';playerResponse.streamingData.adaptiveFormats[0].lastModified='9007199254740993';},other);assert.equal(await title(),'New');
  });
  await check('one advancing format cannot hide a backwards companion revision',async()=>{
   await setup();await page.evaluate(value=>{playerResponse=value;playerResponse.streamingData.adaptiveFormats.push({itag:140,lastModified:'2'});},data('Old'));await title();
   await page.evaluate(other=>{history.pushState({},'', '/watch?v='+other);history.pushState({},'', '/watch?v=abcdefghijk');playerResponse.streamingData.adaptiveFormats[0].lastModified='2';playerResponse.streamingData.adaptiveFormats[1].lastModified='1';},other);assert.equal(await title(),null);
  });
  await check('ambiguous duplicate format revisions cannot refresh a retired response',async()=>{
   await setup();await page.evaluate(value=>{playerResponse=value;},data('Old'));await title();
   await page.evaluate(other=>{history.pushState({},'', '/watch?v='+other);history.pushState({},'', '/watch?v=abcdefghijk');playerResponse.streamingData.adaptiveFormats[0].lastModified='2';playerResponse.streamingData.adaptiveFormats.push({...playerResponse.streamingData.adaptiveFormats[0],lastModified:'3'});},other);assert.equal(await title(),null);
  });
  for(const route of pending.values())await route.abort().catch(()=>{});await page.close();
 }
})().catch(error=>{checks.push({name:'fixture',passed:false,error:error.stack});process.exitCode=1;}).finally(async()=>{
 await browser?.close();const passed=checks.filter(check=>check.passed).length,failed=checks.length-passed;
 fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({passed:failed===0,checks,passedChecks:passed,failedChecks:failed,scope:'Owned responses in headless Edge; both bundles and both observer wrapping orders. No personal browser profile or real video download.'},null,2));
 if(failed)process.exitCode=1;console.log(JSON.stringify({passed,failed}));
});
