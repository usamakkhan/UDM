'use strict';
const fs=require('node:fs'),path=require('node:path'),http=require('node:http'),assert=require('node:assert/strict'),{chromium}=require('playwright');
const root=path.resolve(process.argv[2]),source=process.env.UDM_PROXY_SOURCE||path.resolve(__dirname,'../browser/chromium/chromium-proxy.js'),results=[];
let context,origin,proxy;
const listen=async label=>{const server=http.createServer((q,r)=>r.end(label));await new Promise(r=>server.listen(0,'127.0.0.1',r));return server;};
(async()=>{
 assert(!fs.existsSync(root));fs.mkdirSync(root,{recursive:true});origin=await listen('direct');proxy=await listen('proxy');
 const ext=path.join(root,'extension');fs.mkdirSync(ext);fs.copyFileSync(source,path.join(ext,'chromium-proxy.js'));
 fs.writeFileSync(path.join(ext,'manifest.json'),JSON.stringify({manifest_version:3,name:'UDM isolated proxy route verification',version:'1.0',permissions:['proxy'],host_permissions:['<all_urls>'],background:{service_worker:'background.js'}}));
 fs.writeFileSync(path.join(ext,'background.js'),"importScripts('chromium-proxy.js');");
 context=await chromium.launchPersistentContext(path.join(root,'profile'),{channel:'msedge',headless:true,ignoreDefaultArgs:['--disable-extensions'],args:['--enable-unsafe-extension-debugging']});
 const cdp=await context.browser().newBrowserCDPSession();await cdp.send('Extensions.loadUnpacked',{path:ext});const worker=context.serviceWorkers()[0]||await context.waitForEvent('serviceworker');
 const page=await context.newPage();
 const cases=[];
 for(const host of ['127.0.0.1','[::ffff:127.0.0.1]'])for(const subtract of [false,true])cases.push({host,bypassList:subtract?['<-loopback>']:[],expected:subtract?'proxy':'direct'});
 for(const [host,rule,expected] of [
  ['[::ffff:127.0.0.1]','127.0.0.0/8','direct'],['127.0.0.1','::ffff:127.0.0.0/104','direct'],
  ['[::ffff:127.0.0.1]','128.0.0.0/8','proxy'],['127.0.0.1','2001:db8::/32','proxy']
 ])cases.push({host,bypassList:['<-loopback>',rule],expected});
 for(const {host,bypassList,expected} of cases){
  const config={mode:'fixed_servers',rules:{singleProxy:{scheme:'http',host:'127.0.0.1',port:proxy.address().port},bypassList}};
  await worker.evaluate(config=>new Promise((resolve,reject)=>chrome.proxy.settings.set({value:config,scope:'regular'},()=>chrome.runtime.lastError?reject(Error(chrome.runtime.lastError.message)):resolve())),config);
  const url='http://'+host+':'+origin.address().port+'/case-'+results.length;
  const captured=await worker.evaluate(({config,url})=>UdmChromiumProxy.route(config,url),{config,url});
  await page.goto(url);const actual=(await page.locator('body').innerText()).trim();
  assert.equal(actual,expected);assert.equal(captured.type,expected==='proxy'?'http':'direct');
  results.push({url,bypassList,captured,actual,passed:true});console.log('PASS '+host+' bypass='+JSON.stringify(bypassList)+' uses '+actual);
 }
})().catch(e=>{results.push({passed:false,error:e.stack});process.exitCode=1;console.error(e);}).finally(async()=>{await context?.close();for(const s of [origin,proxy])if(s){s.closeAllConnections();await new Promise(r=>s.close(r));}fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({results},null,2));});

