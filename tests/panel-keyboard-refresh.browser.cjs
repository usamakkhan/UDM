'use strict';
const {chromium}=require('playwright');
const fs=require('node:fs');
const path=require('node:path');
const assert=require('node:assert/strict');

const root=path.resolve(process.argv[2]);
const project=process.env.UDM_TEST_PROJECT||path.resolve(__dirname,'..');
const family=process.env.UDM_TEST_BUNDLE||'chromium';
const checks=[];
let browser;

(async()=>{
 fs.mkdirSync(root,{recursive:true});
 browser=await chromium.launch({channel:process.env.UDM_TEST_BROWSER||'msedge',headless:true});
 const page=await browser.newPage({viewport:{width:1200,height:800}});
 const cdp=await page.context().newCDPSession(page);
 await page.route('https://panel-focus.test/',route=>route.fulfill({contentType:'text/html',
  body:'<!doctype html><video style="position:absolute;left:100px;top:180px;width:800px;height:450px;background:#456"></video>'}));
 await page.goto('https://panel-focus.test/');
 await page.evaluate(()=>{
  globalThis.stream={key:'selected',label:'720p fixture',height:720,container:'mp4',audioOnlyAvailable:true,
   audioOptions:[{key:'en',label:'English',default:true},{key:'es',label:'Spanish'}]};
  globalThis.extra={key:'extra',label:'1080p fixture',height:1080,container:'ts'};
  globalThis.siteRequests=0;globalThis.siteHandoffs=[];globalThis.batchKeys=[];
  let sequence=0,lookup,progress;
  globalThis.chrome={storage:{local:{get:async()=>({}),set:async()=>{}}},runtime:{id:'udm-fixture',
   onMessage:{addListener(callback){progress=callback;}},sendMessage:message=>{
    if(message.action==='site-formats'){
     siteRequests++;
     lookup=message;
     return new Promise(resolve=>globalThis.finish=()=>resolve({ok:true,choices:[extra,{...stream,label:'720p finalized fixture'}]}));
    }
    if(message.action==='site-download'||message.action==='site-video-batch')siteHandoffs.push(message.action);
    if(message.action==='site-video-batch'){
     batchKeys.push(message.keys);
     return Promise.resolve({ok:true,count:message.keys.length});
    }
    return Promise.resolve({ok:true});
   }}};
  globalThis.update=(choices,moreFormats=true,videoBatch=false)=>progress({action:'site-formats-progress',token:lookup.token,progressId:lookup.progressId,
   sequence:++sequence,result:{ok:true,moreFormats,videoBatch,choices}},{id:'udm-fixture'});
 });
 for(const name of ['media.js','content.js'])await page.addScriptTag({path:path.join(project,'browser',family,name)});
 async function node(name){
  for(let attempt=0;attempt<50;attempt++){
   const tree=await cdp.send('Accessibility.getFullAXTree');
   const found=tree.nodes.find(item=>!item.ignored&&item.name?.value===name&&item.backendDOMNodeId);
   if(found)return found;
   await page.waitForTimeout(30);
  }
  throw Error('Missing control '+name);
 }
 async function focus(name){await cdp.send('DOM.focus',{backendNodeId:(await node(name)).backendDOMNodeId});}
 async function focused(name){return (await node(name)).properties?.some(p=>p.name==='focused'&&p.value?.value===true);}
 async function disabled(name){return (await node(name)).properties?.some(p=>p.name==='disabled'&&p.value?.value===true);}
 async function activate(name){await focus(name);await page.keyboard.press('Enter');}
 async function point(name){
  const {model}=await cdp.send('DOM.getBoxModel',{backendNodeId:(await node(name)).backendDOMNodeId});
  return {x:(model.border[0]+model.border[4])/2,y:(model.border[1]+model.border[5])/2};
 }
 function pass(name){checks.push(name);console.log('PASS '+name);}

 await focus('Download this video with UDM');
 await page.keyboard.press('ArrowDown');
 await page.waitForFunction(()=>typeof finish==='function');
 await page.evaluate(()=>update([stream]));
 assert(await focused('720p fixture'));
 await page.evaluate(()=>update([extra]));
 assert(await focused('Download this video with UDM'),'A removed format must return focus to the panel trigger');
 await node('The previous selection is no longer available. Choose another format.');
 const priorRequests=await page.evaluate(()=>siteRequests);
 await page.keyboard.press('Enter');
 assert.deepEqual(await page.evaluate(()=>siteHandoffs),[],'Enter must not download the replacement quality');
 pass('A removed format cannot silently redirect Enter to a different quality');

 await page.keyboard.press('ArrowDown');
 await page.waitForFunction(prior=>siteRequests>prior,priorRequests);
 await page.evaluate(()=>update([extra,stream],false));
 await focus('Download all (default audio)');
 await page.evaluate(()=>update([stream]));
 assert(await focused('Download this video with UDM'),'A hidden batch action must return focus to the panel trigger');
 assert.deepEqual(await page.evaluate(()=>siteHandoffs),[]);
 pass('A catalog refresh that removes Download all leaves focus on the panel');
 await page.keyboard.press('ArrowDown');
 assert(await focused('720p fixture'),'Keyboard navigation can deliberately select the remaining format');
 await page.keyboard.press('ArrowDown');
 const options='Audio/subtitle options for 720p fixture';
 assert(await focused(options));
 await page.evaluate(()=>update([extra,stream]));
 await node('1080p fixture');
 assert(await focused(options),'A refreshed catalog must keep keyboard focus on the same track options');
 pass('Track options retain keyboard focus when an earlier row arrives');

 await page.keyboard.press('Enter');
 await node('Audio track');
 await focus('Download video');
 await page.evaluate(()=>update([extra,{...stream,key:'replacement',label:'720p replacement'}]));
 assert(await disabled('Download video'),'A removed format must disable the old chooser handoff');
 assert(await disabled('Download audio (M4A)'));
 assert(await focused('Back'));
 await node('This format is no longer in the list. Go back to choose another format.');
 pass('A replaced offer blocks both old handoffs and leaves Back focused');

 await page.evaluate(()=>finish());
 assert(!(await disabled('Download video')));
 assert(!(await disabled('Download audio (M4A)')));
 await node('Audio track');
 pass('The same offer can return without closing the chooser');
 await activate('Back');
 assert(await focused('Audio/subtitle options for 720p finalized fixture'),'Back must return to the track options that opened the chooser');
 pass('A finalized catalog preserves the chooser and Back restores its opening control');

 await activate('Audio only (M4A)…');
 await node('Audio track');
 await activate('Back');
 assert(await focused('Audio only (M4A)…'),'Back must return to the audio-only control that opened the chooser');
 pass('Back returns to the audio-only control');
 await page.keyboard.press('Escape');
 assert(await focused('Download this video with UDM'));
 pass('Escape returns to the video panel');

 async function openFreshBatch(){
  // A completed catalog can be cached. Refresh starts a new live lookup.
  const prior=await page.evaluate(()=>siteRequests);
  await activate('Video panel options');
  await activate('Refresh');
  await page.waitForFunction(count=>siteRequests>count,prior);
  await page.evaluate(()=>update([stream,extra],false,true));
  const button=await point('Download all (default audio)');
  await page.mouse.move(button.x,button.y);
  await page.mouse.down();
  await page.evaluate(()=>update([{...extra,key:'new',label:'New 1080p fixture'},stream],false,true));
 }
 await openFreshBatch();
 await page.mouse.up();
 await page.waitForFunction(()=>batchKeys.length===1);
 assert.deepEqual(await page.evaluate(()=>batchKeys),[['selected','extra']],
  'The pointer release must use the batch shown at pointer down');
 pass('Download all keeps the visible batch during a catalog refresh under the pointer');

 await openFreshBatch();
 await page.mouse.move(10,10);
 await page.mouse.up();
 await node('New 1080p fixture');
 assert.deepEqual(await page.evaluate(()=>batchKeys),[['selected','extra']]);
 pass('Releasing outside the batch action applies the deferred refresh without sending a download');
})().catch(error=>{console.error(error);process.exitCode=1;}).finally(async()=>{
 fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({family,checks},null,2));
 await browser?.close();
});
