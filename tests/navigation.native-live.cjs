'use strict';
const assert=require('node:assert/strict');
module.exports=async({worker,page,until,pass})=>{
 const origin='http://127.0.0.1:43821';
 const seen=[];
 async function check(frame,expected){
  await frame.waitForFunction(()=>document.querySelector('video')?.videoHeight===360);
  const token=await until(()=>frame.locator('video').getAttribute('data-udm-player'));
  const info=await until(()=>worker.evaluate(async({token})=>{
   const [tab]=await chrome.tabs.query({active:true,lastFocusedWindow:true});if(!tab)return null;
   const rows=await chrome.scripting.executeScript({target:{tabId:tab.id,allFrames:true},func:token=>({matches:!!document.querySelector('video[data-udm-player="'+token+'"]'),page:location.href}),args:[token]});
   const row=rows.find(x=>x.result.matches);if(!row?.documentId)return null;
   await captureNavigation.settled();
   const captures=(await chrome.storage.session.get('site-media:'+tab.id))['site-media:'+tab.id]||[];
   const current=captures.filter(x=>x.frameId===row.frameId&&x.documentId===row.documentId&&x.kind==='hls');
   if(!current.some(x=>x.url.includes('master')))return null;
   const choices=await UdmSites.list({token,page:row.result.page},{tab:{id:tab.id},frameId:row.frameId,documentId:row.documentId,url:row.result.page});
   return {documentId:row.documentId,frameId:row.frameId,current:current.map(x=>x.url),stale:captures.filter(x=>x.frameId===row.frameId&&x.documentId!==row.documentId),choices:choices.choices};
  },{token}));
  assert.equal(info.stale.length,0);assert(info.current.some(x=>x.includes(expected)));
  assert(info.choices.some(x=>x.source===expected));assert(!info.choices.some(x=>/master-(before|after)/.test(x.source)&&x.source!==expected));
  seen.push({documentId:info.documentId,frameId:info.frameId,expected});return info;
 }
 for(let n=0;n<8;n++){const variant=n%2?'after':'before';await page.goto(origin+'/hls?catalog='+variant);await check(page,'master-'+variant+'.m3u8');}
 pass('Eight consecutive top-level HLS navigations retain only the current document catalog',{navigations:8});
 const first=seen.at(-1);await page.reload();const reloaded=await check(page,'master-after.m3u8');assert.notEqual(first.documentId,reloaded.documentId);
 await page.goto(origin+'/hls?catalog=before');await check(page,'master-before.m3u8');await page.goBack();await check(page,'master-after.m3u8');
 pass('Same-URL reload and browser Back preserve a usable current video menu');
 await page.goto(origin+'/isolation');const embedded=await until(()=>page.frame({name:'player'}));await check(embedded,'master-before.m3u8');
 const frameIds=[];
 for(let n=0;n<8;n++){const variant=n%2?'before':'after';await embedded.goto(origin+'/hls?catalog='+variant);frameIds.push((await check(embedded,'master-'+variant+'.m3u8')).frameId);}
 assert.equal(new Set(frameIds).size,1);
 pass('Eight consecutive iframe replacements retain frame identity without leaking old catalogs',{navigations:8});
 const tabId=await worker.evaluate(async()=>{const [tab]=await chrome.tabs.query({active:true,lastFocusedWindow:true});return tab.id;});
 await page.close();await until(()=>worker.evaluate(async tabId=>{await captureNavigation.settled();const s=await chrome.storage.session.get(['site-media:'+tabId,'site-offers:'+tabId,'media:'+tabId,'sabr:'+tabId]);return Object.keys(s).length===0;},tabId));
 pass('Closing a real media tab clears playlists, menu offers and raw/streaming context',{capturesChecked:seen.length});
};

