// Real Edge navigation races; original page fetch/XHR responses must remain intact.
'use strict';
const {chromium}=require('playwright'),fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict'),http=require('node:http');
const root=path.resolve(process.argv[2]),checks=[];let browser,server;const pending=new Map();let base;
(async()=>{fs.mkdirSync(root,{recursive:true});server=http.createServer((req,res)=>{if(req.url.endsWith('.m3u8'))pending.set(req.url,res);else{res.setHeader('Content-Type','text/html');res.end('<!doctype html><title>Observer fixture</title>');}});await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));base='http://127.0.0.1:'+server.address().port;browser=await chromium.launch({channel:'msedge',headless:true});
 for(const family of ['chromium','firefox']){
  const page=await browser.newPage();pending.clear();
  await page.goto(base+'/a');await page.addScriptTag({path:path.join(__dirname,'../browser',family,'page-observer.js')});
  async function waitRoute(name){for(let i=0;i<100&&!pending.has(name);i++)await page.waitForTimeout(10);assert(pending.has(name));return pending.get(name);}
  async function fulfill(name){const res=await waitRoute(name);res.setHeader('Content-Type','application/vnd.apple.mpegurl');res.end('#EXTM3U\n#EXTINF:2,\n'+name.slice(1)+'.ts\n#EXT-X-ENDLIST\n');}
  async function settle(){await page.waitForTimeout(100);return page.evaluate(()=>__udmMediaObserverV1.read());}
  function check(ok,name){checks.push({name:family+' '+name,passed:!!ok});assert(ok,name);}
  await page.evaluate(()=>{globalThis.originalResponse=fetch('/old.m3u8').then(r=>r.text());});await waitRoute('/old.m3u8');
  await page.evaluate(()=>{history.pushState({},'', '/b');history.pushState({},'', '/a');});await fulfill('/old.m3u8');
  check((await page.evaluate(()=>originalResponse)).startsWith('#EXTM3U'),'navigation does not change the original fetch response');
  check((await settle()).length===0,'late fetch from previous visit cannot populate the current page');
  await page.evaluate(()=>{globalThis.currentResponse=fetch('/current.m3u8').then(r=>r.text());});await fulfill('/current.m3u8');await page.evaluate(()=>currentResponse);check((await settle()).length===1,'current visit still captures its playlist');
  await page.evaluate(()=>{history.replaceState({},'', '/b');history.replaceState({},'', '/a');});check((await settle()).length===0,'round-trip replaceState clears prior cached playlists');
  await page.evaluate(()=>{globalThis.xhr=new XMLHttpRequest();xhr.open('GET','/xhr-old.m3u8');globalThis.xhrDone=new Promise(resolve=>xhr.addEventListener('load',()=>resolve(xhr.responseText)));xhr.send();});await waitRoute('/xhr-old.m3u8');
  await page.evaluate(()=>{history.pushState({},'', '/b');history.pushState({},'', '/a');});await fulfill('/xhr-old.m3u8');check((await page.evaluate(()=>xhrDone)).startsWith('#EXTM3U'),'navigation does not change the original XHR response');check((await settle()).length===0,'late XHR from previous visit cannot populate the current page');
  await page.evaluate(()=>{globalThis.same=fetch('/same.m3u8').then(r=>r.text());history.replaceState({marker:1},'',location.href);});await fulfill('/same.m3u8');await page.evaluate(()=>same);check((await settle()).length===1,'same-URL state update retains the current request');
  check(await page.evaluate(()=>{try{history.pushState({},'', 'https://different.test/');return false;}catch(error){return error.name==='SecurityError'&&__udmMediaObserverV1.read().length===1;}}),'rejected cross-origin history update retains valid captures and original error');
  await page.evaluate(()=>{globalThis.slow=fetch('/slow.m3u8').then(r=>r.text());});const slow=await waitRoute('/slow.m3u8');slow.writeHead(200,{'Content-Type':'application/vnd.apple.mpegurl'});slow.write('#EXTM3U\n');await page.waitForTimeout(100);
  await page.evaluate(()=>{history.pushState({},'', '/b');history.pushState({},'', '/a');});slow.end('#EXTINF:2,\nslow.ts\n#EXT-X-ENDLIST\n');
  check((await page.evaluate(()=>slow)).includes('slow.ts'),'navigation during body read preserves the complete original response');check((await settle()).length===0,'navigation during body read invalidates the pending clone capture');
  await page.evaluate(()=>{history.pushState({},'', '/b');history.pushState({},'', '/a');globalThis.backResponse=fetch('/back.m3u8').then(r=>r.text());});await waitRoute('/back.m3u8');
  await page.evaluate(()=>history.back());await page.waitForURL(base+'/b');await page.evaluate(()=>history.forward());await page.waitForURL(base+'/a');await fulfill('/back.m3u8');await page.evaluate(()=>backResponse);check((await settle()).length===0,'back-forward route revisit cannot resurrect an earlier playlist request');
  const locked=await browser.newPage(),lockedErrors=[];locked.on('pageerror',error=>lockedErrors.push(error.message));await locked.goto(base+'/locked');await locked.evaluate(()=>{for(const name of ['pushState','replaceState'])Object.defineProperty(history,name,{value:history[name],writable:false,configurable:false});});await locked.addScriptTag({path:path.join(__dirname,'../browser',family,'page-observer.js')});await locked.evaluate(()=>{globalThis.result=fetch('/locked.m3u8').then(r=>r.text());});await fulfill('/locked.m3u8');await locked.evaluate(()=>result);await locked.waitForTimeout(100);check(!lockedErrors.length&&await locked.evaluate(()=>__udmMediaObserverV1.read().length===1),'read-only site history methods do not prevent ordinary playlist capture');await locked.close();
  await page.close();
 }
})().catch(error=>{checks.push({passed:false,error:error.stack});process.exitCode=1;console.error(error);}).finally(async()=>{fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({scope:'Both observer bundles executed in real isolated Edge with original fetch and XHR plus controlled SPA navigation.',checks},null,2));await browser?.close();server?.closeAllConnections();await new Promise(resolve=>server?server.close(resolve):resolve());console.log(checks.filter(c=>c.passed).length+' passed');});
