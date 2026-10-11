'use strict';
const fs=require('node:fs'),path=require('node:path'),http=require('node:http'),assert=require('node:assert/strict'),{chromium}=require('playwright');
const root=path.resolve(process.argv[2]),checks=[];let server,context;
const pass=name=>{checks.push({name,passed:true});console.log('PASS '+name);};
(async()=>{
 assert(!fs.existsSync(root));fs.mkdirSync(root,{recursive:true});
 server=http.createServer((q,r)=>{r.setHeader('Content-Type','text/html');r.end('<!doctype html><title>Live collector</title><script>let a=document.createElement("a");a.href=["runtime",".zip"].join("");a.textContent="Runtime";document.documentElement.append(a);</script>');});await new Promise(r=>server.listen(0,'127.0.0.1',r));const url='http://127.0.0.1:'+server.address().port+'/page';
 const ext=path.join(root,'extension');fs.mkdirSync(ext);for(const name of ['grabber-rendered.js','navigation.js'])fs.copyFileSync(path.resolve(__dirname,'../browser/chromium',name),path.join(ext,name));
 fs.writeFileSync(path.join(ext,'manifest.json'),JSON.stringify({manifest_version:3,name:'UDM isolated rendered capture',version:'1.0',permissions:['scripting','tabs','webNavigation','storage'],host_permissions:['http://127.0.0.1/*'],background:{service_worker:'background.js'}}));
 fs.writeFileSync(path.join(ext,'background.js'),"importScripts('navigation.js','grabber-rendered.js'); const navigation=UdmNavigation.create(chrome); navigation.install(); const rendered=UdmRenderedGrabber.create(chrome,navigation);");
 context=await chromium.launchPersistentContext(path.join(root,'profile'),{channel:'msedge',headless:true,ignoreDefaultArgs:['--disable-extensions'],args:['--enable-unsafe-extension-debugging']});const cdp=await context.browser().newBrowserCDPSession();await cdp.send('Extensions.loadUnpacked',{path:ext});const worker=context.serviceWorkers()[0]||await context.waitForEvent('serviceworker');const page=await context.newPage();await page.goto(url);
 const tabId=await worker.evaluate(async url=>(await chrome.tabs.query({url}))[0].id,url);
 const result=await worker.evaluate(args=>rendered.capture(args),{tabId,url});assert(result.documentId&&result.references.some(x=>x.url.endsWith('/runtime.zip')));pass('Real isolated-world collector returns live JavaScript link with browser document ID');
 // Hold the real executeScript result while reloading the actual document.
 await worker.evaluate(()=>{globalThis.realExecute=chrome.scripting.executeScript.bind(chrome.scripting);chrome.scripting.executeScript=async args=>{const result=await realExecute(args);globalThis.captureHeld=true;await new Promise(r=>globalThis.releaseCapture=r);return result;};});
 const pending=worker.evaluate(args=>rendered.capture(args).then(()=>({accepted:true}),e=>({accepted:false,error:e.message})),{tabId,url});
 const until=Date.now()+10000;while(!await worker.evaluate(()=>!!globalThis.captureHeld)){assert(Date.now()<until);await new Promise(r=>setTimeout(r,50));}
 await page.reload();await worker.evaluate(()=>releaseCapture());const rejected=await pending;assert.equal(rejected.accepted,false);assert.match(rejected.error,/changed/);pass('Actual same-URL reload rejects the held old-document result');
 await worker.evaluate(()=>chrome.scripting.executeScript=realExecute);
 const fresh=await worker.evaluate(args=>rendered.capture(args),{tabId,url});assert.notEqual(fresh.documentId,result.documentId);pass('Fresh capture after reload uses the new document identity');
 fs.writeFileSync(path.join(root,'snapshot.json'),JSON.stringify(fresh,null,2));
})().catch(e=>{checks.push({passed:false,error:e.stack});console.error(e);process.exitCode=1;}).finally(async()=>{await context?.close();if(server){server.closeAllConnections();await new Promise(r=>server.close(r));}fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({checks},null,2));});
