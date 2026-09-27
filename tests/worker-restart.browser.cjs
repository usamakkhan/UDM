'use strict';
const {chromium}=require('playwright'),fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const root=path.resolve(process.argv[2]),project=path.resolve(__dirname,'..');let context;const results=[];
const wait=ms=>new Promise(r=>setTimeout(r,ms));
async function until(fn){for(let i=0;i<100;i++){const v=await fn();if(v)return v;await wait(100);}throw Error('Worker recovery timed out');}
(async()=>{
 fs.mkdirSync(root,{recursive:true});context=await chromium.launchPersistentContext(path.join(root,'profile'),{channel:process.env.UDM_TEST_BROWSER||'chrome',headless:true,ignoreDefaultArgs:['--disable-extensions'],args:['--enable-unsafe-extension-debugging']});
 await context.route('https://udm-worker.test/**',r=>r.fulfill({contentType:'text/html',body:'<!doctype html><title>Worker recovery fixture</title><a href="https://udm-worker.test/file.zip">Fixture file</a>'}));
 const control=await context.browser().newBrowserCDPSession();await control.send('Extensions.loadUnpacked',{path:path.join(project,'browser/chromium')});
 let worker=context.serviceWorkers()[0]||await context.waitForEvent('serviceworker');context.on('serviceworker',value=>worker=value);const page=await context.newPage();await page.goto('https://udm-worker.test/');
 const session=await context.newCDPSession(page);let versions=[];session.on('ServiceWorker.workerVersionUpdated',e=>{for(const row of e.versions){versions=versions.filter(v=>v.versionId!==row.versionId);versions.push(row);}});await session.send('ServiceWorker.enable');
 const opened=context.waitForEvent('page');await worker.evaluate(async()=>{const tab=(await chrome.tabs.query({url:'https://udm-worker.test/*'}))[0];await browserControls.collect(tab,false);});const links=await opened;await links.getByRole('checkbox',{name:'Select Fixture file',exact:true}).waitFor();
 const token=new URL(links.url()).hash.slice(1),version=await until(()=>versions.find(v=>v.scriptURL===worker.url()&&v.runningStatus==='running'));
 await worker.evaluate(()=>{globalThis.restartMarker='before-stop';nativeClient.disconnect();});await session.send('ServiceWorker.stopWorker',{versionId:version.versionId});await until(()=>versions.some(v=>v.versionId===version.versionId&&v.runningStatus==='stopped'));
 await links.reload();await links.getByRole('checkbox',{name:'Select Fixture file',exact:true}).waitFor();assert.equal(await worker.evaluate(()=>globalThis.restartMarker),undefined);
 assert.equal(new URL(links.url()).hash.slice(1),token);assert.equal(await links.getByRole('status').textContent(),'');results.push({name:'Actual worker termination preserves the open link-selection list',passed:true});console.log('PASS actual worker termination preserves the open link-selection list');
 // Persist an in-flight state without making a native download request.
 await worker.evaluate(async token=>{const data=await chrome.storage.session.get('linkBatches');const row=data.linkBatches.find(([key])=>key===token);row[1].busy=true;await chrome.storage.session.set({linkBatches:data.linkBatches});},token);
 const next=await until(()=>versions.find(v=>v.scriptURL===worker.url()&&v.runningStatus==='running'));await worker.evaluate(()=>nativeClient.disconnect());await session.send('ServiceWorker.stopWorker',{versionId:next.versionId});await until(()=>versions.some(v=>v.versionId===next.versionId&&v.runningStatus==='stopped'));
 await links.reload();await links.getByRole('status').filter({hasText:'Check UDM'}).waitFor();assert.equal(await links.getByRole('checkbox',{name:'Select Fixture file',exact:true}).count(),0);results.push({name:'Restart refuses an uncertain submitted batch instead of exposing it for replay',passed:true});console.log('PASS restart refuses an uncertain submitted batch');
 await links.screenshot({path:path.join(root,'uncertain-batch.png')});
})().catch(e=>{results.push({passed:false,error:e.message});console.error(e);process.exitCode=1;}).finally(async()=>{fs.mkdirSync(root,{recursive:true});fs.writeFileSync(path.join(root,'results.json'),JSON.stringify(results,null,2));await context?.close();});
