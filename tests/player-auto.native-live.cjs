'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
module.exports=async({worker,page,until,jobs,pass,project})=>{
 const hash=file=>crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
 await page.goto('http://127.0.0.1:43821/embed');await page.waitForFunction(()=>document.querySelector('video').videoHeight===360);
 await page.waitForTimeout(2200);assert.equal(jobs().length,0);pass('Configured player capture cannot enable the browser capture opt-in');
 await worker.evaluate(async()=>{const s=await chrome.storage.local.get('settings');await chrome.storage.local.set({settings:{...s.settings,capture:true}});});
 await page.reload();await page.waitForFunction(()=>document.querySelector('video').videoHeight===360);
 const job=await until(()=>{const j=jobs().find(x=>x.Url.endsWith('/sample.mp4'));if(j?.Status==='Failed')throw Error(j.Error);return j?.Status==='Complete'&&j;},30000);
 assert.equal(hash(path.join(job.Folder,job.FileName)),hash(path.join(project,'native/browser-fixture-data/sample.mp4')));pass('Opted-in direct-player capture creates a byte-identical native MP4 download');
 await page.waitForTimeout(4500);assert.equal(jobs().length,1);pass('Repeated player observations do not duplicate the acknowledged download');
 await worker.evaluate(async()=>{const s=await chrome.storage.local.get('settings');await chrome.storage.local.set({settings:{...s.settings,capture:false}});});
 await page.waitForTimeout(300);await page.evaluate(()=>{document.querySelector('video').src='/sample.mp4?optout=1';});await page.waitForFunction(()=>document.querySelector('video').currentSrc.includes('optout=1')&&!document.querySelector('video').paused);await page.waitForTimeout(2600);assert.equal(jobs().length,1);pass('Turning capture off prevents new player sources from creating jobs');
};
