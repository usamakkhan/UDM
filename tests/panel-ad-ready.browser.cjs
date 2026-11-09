'use strict';
const {chromium}=require('playwright');
const fs=require('node:fs');
const path=require('node:path');
const assert=require('node:assert/strict');

const root=path.resolve(process.argv[2]);
const project=process.env.UDM_TEST_PROJECT||path.resolve(__dirname,'..');
const bundle=process.env.UDM_TEST_BUNDLE||'chromium';
const channel=process.env.UDM_TEST_BROWSER||'chrome';
assert(!fs.existsSync(root));
fs.mkdirSync(root,{recursive:true});
let browser;
const results=[];

(async()=>{
 browser=await chromium.launch({channel,headless:true});
 const page=await browser.newPage({viewport:{width:1280,height:900}});
 const cdp=await page.context().newCDPSession(page);
 await page.route('https://www.youtube.com/watch?*',route=>route.fulfill({contentType:'text/html',body:
  '<!doctype html><style>video{position:absolute;width:560px;height:315px;background:#456}</style>'+
  '<div class="ad-showing" id="movie_player"><video id="main" style="left:10px;top:40px"></video>'+
  '<video id="secondary" style="left:600px;top:40px"></video></div>'+
  '<div data-ad-playing="true"><video id="separate" style="left:10px;top:420px"></video></div>'}));
 await page.goto('https://www.youtube.com/watch?v=abcdefghijk');
 await page.evaluate(()=>{
  const runtimeId='udm-fixture',popup='https://fixture.invalid/popup.html';
  let receive;
  globalThis.calls=[];
  globalThis.chrome={storage:{local:{get:async()=>({}),set:async()=>{}}},runtime:{
   id:runtimeId,getURL:()=>popup,onMessage:{addListener(callback){receive=callback;}},
   async sendMessage(message){
    calls.push(message);
    if(message.action==='formats')return {ok:true,choices:[
     {key:'720',label:'Verified 720p',height:720},{key:'1080',label:'Verified 1080p',height:1080}]};
    return {ok:true};
   }
  }};
  globalThis.prepared=()=>{
   let result;
   receive({action:'prepared-video-qualities',page:location.href},{id:runtimeId,url:popup},value=>result=value);
   return result;
  };
 });
 for(const file of ['media.js','content.js'])await page.addScriptTag({path:path.join(project,'browser',bundle,file)});

 async function settle(){await page.evaluate(()=>new Promise(resolve=>requestAnimationFrame(()=>requestAnimationFrame(resolve))));}
 async function visiblePlayers(){return page.evaluate(()=>Array.from(document.querySelectorAll('video')).filter(video=>{
  const host=document.getElementById('udm-video-panel-'+video.getAttribute('data-udm-player'));
  return host&&getComputedStyle(host).display!=='none';
 }).map(video=>video.id));}
 async function marker(id,age=0){
  await page.evaluate(([id,age])=>document.documentElement.setAttribute('data-udm-content-ready',id+':'+(Date.now()-age)),[id,age]);
  await settle();
 }
 async function lookups(){return page.evaluate(()=>calls.filter(message=>message.action==='formats'));}
 async function node(name){
  for(let attempt=0;attempt<40;attempt++){
   const {nodes}=await cdp.send('Accessibility.getFullAXTree');
   const found=nodes.find(item=>!item.ignored&&item.name?.value===name&&item.backendDOMNodeId);
   if(found)return found;
   await page.waitForTimeout(25);
  }
  throw Error('Missing panel control '+name);
 }
 function pass(name){results.push({name,passed:true});console.log('PASS '+name);}

 await page.waitForFunction(()=>document.querySelectorAll('video[data-udm-player]').length===3);
 await settle();
 assert.deepEqual(await visiblePlayers(),[]);
 await marker('differentid');
 assert.deepEqual(await visiblePlayers(),[]);
 assert.equal((await lookups()).length,0);
 pass('Ads and readiness for another video leave all panels hidden');

 await marker('abcdefghijk');
 assert.deepEqual(await visiblePlayers(),[],'Readiness must not reveal an ambiguous group of players');
 pass('A verified video ID cannot choose between ambiguous players');

 await page.locator('#main').evaluate(video=>video.classList.add('html5-main-video'));
 await settle();
 assert.deepEqual(await visiblePlayers(),['main']);
 assert.equal(await page.locator('#main').evaluate(video=>video.readyState),0);
 assert.equal(await page.locator('#main').evaluate(video=>video.paused),true);
 await page.waitForFunction(()=>prepared()?.choices?.length===2);
 const requested=await lookups();
 assert.equal(requested.length,1);
 assert.equal(requested[0].prepareOnly,true);
 pass('Only the primary player shows and prepares verified choices before media metadata');

 const toggle=await node('Download this video with UDM');
 const {model}=await cdp.send('DOM.getBoxModel',{backendNodeId:toggle.backendDOMNodeId});
 const [left,top,,,right,bottom]=model.border;
 await page.mouse.click((left+right)/2,(top+bottom)/2);
 await node('Verified 720p');
 await node('Verified 1080p');
 assert.equal((await lookups()).length,1);
 assert.equal(await page.evaluate(()=>calls.some(message=>['media','youtube-audio','youtube-caption','site-download','site-video-batch'].includes(message.action))),false);
 pass('The first click shows both prepared choices without requesting formats again or starting a download');

 await page.keyboard.press('Escape');
 await page.locator('#main').evaluate(video=>video.setAttribute('data-ad-playing','true'));
 await settle();
 assert.deepEqual(await visiblePlayers(),[],'A separate ad flag on the primary video still applies');
 await page.locator('#main').evaluate(video=>video.removeAttribute('data-ad-playing'));
 await marker('abcdefghijk',300001);
 assert.deepEqual(await visiblePlayers(),[]);
 pass('Independent ad flags and expired readiness suppress the panel');

 await page.evaluate(()=>{document.querySelector('#secondary').remove();document.querySelector('#main').classList.remove('html5-main-video');});
 await marker('abcdefghijk');
 assert.deepEqual(await visiblePlayers(),['main']);
 await marker('abcdefghijk',300001);
 await page.locator('#movie_player').evaluate(player=>player.classList.remove('ad-showing'));
 await settle();
 assert.deepEqual(await visiblePlayers(),['main']);
 pass('A sole player works without the primary class and normal playback needs no marker');
 console.log(results.length+' panel ad-transition checks passed');
})().catch(error=>{
 results.push({passed:false,error:error.message});
 console.error(error);
 process.exitCode=1;
}).finally(async()=>{
 fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({bundle,channel,results},null,2));
 await browser?.close();
});
