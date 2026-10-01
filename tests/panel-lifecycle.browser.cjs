'use strict';
const {chromium}=require('playwright'),fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const project=process.env.UDM_TEST_PROJECT||path.resolve(__dirname,'..'),out=process.argv[2],channel=process.env.UDM_TEST_BROWSER||'msedge';let browser;const results=[];
const pause=ms=>new Promise(r=>setTimeout(r,ms));
function pass(name){results.push({name,passed:true});console.log('PASS '+name);}
(async()=>{
 fs.mkdirSync(out,{recursive:true});browser=await chromium.launch({channel,headless:true});const context=await browser.newContext(),page=await context.newPage({viewport:{width:1200,height:800}});
 await page.route('https://udm-reload.test/',r=>r.fulfill({contentType:'text/html',body:'<!doctype html><video style="position:absolute;left:100px;top:150px;width:800px;height:450px;background:#456"></video>'}));await page.goto('https://udm-reload.test/');
 const cdp=await context.newCDPSession(page),errors=[];cdp.on('Runtime.exceptionThrown',event=>errors.push(event.exceptionDetails.exception?.description||event.exceptionDetails.text));await cdp.send('Runtime.enable');const frameId=(await cdp.send('Page.getFrameTree')).frameTree.frame.id;
 const media=fs.readFileSync(path.join(project,'browser/chromium/media.js'),'utf8'),content=fs.readFileSync(path.join(project,'browser/chromium/content.js'),'utf8');
 const shim="globalThis.chrome={runtime:{id:'udm-fixture',sendMessage:async()=>({ok:true,choices:[]}),onMessage:{addListener(){},removeListener(){}}},storage:{local:{get:async()=>({}),set:async()=>{}},onChanged:{addListener(){},removeListener(){}}}};";
 async function world(name){return (await cdp.send('Page.createIsolatedWorld',{frameId,worldName:name})).executionContextId;}
 async function run(id,expression){const r=await cdp.send('Runtime.evaluate',{contextId:id,expression,awaitPromise:true,returnByValue:true});if(r.exceptionDetails)throw Error(r.exceptionDetails.text+' '+r.exceptionDetails.exception?.description);return r.result.value;}
 async function settle(expected){for(let i=0;i<60;i++){if(await page.locator('[id^="udm-video-panel-"]').count()===expected)return;await pause(50);}assert.equal(await page.locator('[id^="udm-video-panel-"]').count(),expected);}
 const selection=fs.readFileSync(path.join(project,'browser/chromium/selection.js'),'utf8');
 for(const [name,missing] of [['runtime','{}'],['storage',"{runtime:{id:'udm-fixture'}}"],['identity',"{runtime:{},storage:{local:{}}}"]]){
  const invalid=await world('invalid-'+name);await run(invalid,'globalThis.chrome='+missing+';'+media+'\n'+content+'\n'+selection);await settle(0);
  assert.equal(await run(invalid,'!!globalThis.__udmPanelsV2||!!globalThis.__udmSelectedLinksV1'),false);pass('Missing '+name+' exits before taking ownership or adding listeners');
  await run(invalid,shim+media+'\n'+content+'\n'+selection);await settle(1);pass('Valid reinjection recovers after missing '+name+' APIs');
  await run(invalid,"chrome.runtime=undefined;document.dispatchEvent(new Event('selectionchange'))");await pause(1800);await settle(0);
 }
 for(const [name,source,event,flag] of [['video',content,'udm-panel-owner-replaced-v1','__udmPanelsV2'],['selection',selection,'udm-selection-owner-replaced-v1','__udmSelectedLinksV1']]){
  const lost=await world('invalidated-during-'+name+'-replacement');await run(lost,shim+`document.addEventListener('${event}',()=>{chrome.runtime=undefined;},{once:true});`+media+'\n'+source);assert.equal(await run(lost,'!!globalThis.'+flag),false);pass(name+' panel aborts cleanly if its API is invalidated during owner replacement');
 }
 const first=await world('extension-before-reload');await run(first,shim+media+'\n'+content);await settle(1);const oldId=await page.locator('[id^="udm-video-panel-"]').getAttribute('id');pass('Initial isolated extension world creates one panel');
 await run(first,content);await pause(100);await settle(1);pass('Repeated injection in the same context remains idempotent');
 const second=await world('extension-after-reload');await run(second,shim+media+'\n'+content);await settle(1);const newId=await page.locator('[id^="udm-video-panel-"]').getAttribute('id');assert.notEqual(newId,oldId);
 await pause(1900);await settle(1);assert.equal(await page.locator('[id^="udm-video-panel-"]').getAttribute('id'),newId);pass('New extension context disposes the old panel and its rescan timer');
 const marker=await page.locator('video').getAttribute('data-udm-player');assert.equal('udm-video-panel-'+marker,newId);pass('Replacement retains only the active player capture marker');
 await run(second,'chrome.runtime.id=undefined');await pause(1900);await settle(0);assert.equal(await page.locator('video').getAttribute('data-udm-player'),null);pass('Invalidated extension context cleans up its panel and player marker');
 const third=await world('extension-recovered');await run(third,shim+media+'\n'+content);await settle(1);pass('A fresh context restores a single working panel after invalidation');
 await page.evaluate(()=>{const p=document.createElement('p');p.innerHTML='<a href="https://udm-reload.test/file.zip">Download file</a>';document.body.append(p);const r=document.createRange();r.selectNodeContents(p);getSelection().removeAllRanges();getSelection().addRange(r);});
 await run(third,selection);await page.waitForFunction(()=>document.querySelectorAll('#udm-selected-links-panel').length===1);pass('Selected-link panel starts with the current extension context');
 const fourth=await world('selection-reloaded');await run(fourth,shim+media+'\n'+selection);await pause(250);assert.equal(await page.locator('#udm-selected-links-panel').count(),1);pass('Reload replaces the selected-link panel without leaving a duplicate');
 await run(fourth,"chrome.runtime=undefined;document.dispatchEvent(new Event('selectionchange'))");await page.waitForFunction(()=>!document.querySelector('#udm-selected-links-panel'));pass('Invalidated selected-link context removes its panel');
 assert.deepEqual(errors,[]);pass('Reload and invalidation produce no uncaught browser exceptions');
 await page.screenshot({path:path.join(out,'panel-lifecycle.png')});
})().catch(e=>{results.push({passed:false,error:e.message});console.error(e);process.exitCode=1;}).finally(async()=>{fs.writeFileSync(path.join(out,'results.json'),JSON.stringify({browser:channel,results},null,2));await browser?.close();});
