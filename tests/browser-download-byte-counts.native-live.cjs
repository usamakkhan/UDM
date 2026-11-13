'use strict';
const {chromium}=require('playwright'),fs=require('node:fs'),path=require('node:path'),http=require('node:http'),assert=require('node:assert/strict');
const root=path.resolve(process.argv[2]);if(fs.existsSync(root))throw Error('Use a fresh fixture directory');
let browser,server;const observations=[],checks=[];
const pause=ms=>new Promise(resolve=>setTimeout(resolve,ms));
async function until(read){const end=Date.now()+20000;while(Date.now()<end){const result=await read();if(result)return result;await pause(50);}throw Error('Byte-counter fixture timed out');}
(async()=>{
 const downloads=path.join(root,'downloads'),extension=path.join(root,'extension'),profile=path.join(root,'profile');
 fs.mkdirSync(downloads,{recursive:true});fs.mkdirSync(extension);fs.mkdirSync(path.join(profile,'Default'),{recursive:true});
 fs.writeFileSync(path.join(profile,'Default','Preferences'),JSON.stringify({download:{default_directory:downloads,prompt_for_download:false}}));
 fs.writeFileSync(path.join(extension,'manifest.json'),JSON.stringify({manifest_version:3,name:'UDM byte-counter fixture',version:'0.0.1',permissions:['downloads'],background:{service_worker:'worker.js'}}));
 const background=fs.readFileSync(path.join(__dirname,'../browser/chromium/background.js'),'utf8');
 const predicate=background.match(/function browserResponseReceived\(item\)\{[\s\S]*?\n\}/)?.[0];assert(predicate,'Production byte predicate missing');
 fs.writeFileSync(path.join(extension,'worker.js'),predicate+"\nglobalThis.observed=[];chrome.downloads.onCreated.addListener(item=>observed.push({id:item.id,event:'created',state:item.state,totalBytes:item.totalBytes,bytesReceived:item.bytesReceived,udmReceived:browserResponseReceived(item)}));");
 server=http.createServer((req,res)=>{
  if(req.url==='/empty'){res.writeHead(200,{'Content-Type':'text/plain','Content-Disposition':'attachment; filename="empty.txt"','Content-Length':'0'});res.end();return;}
  if(req.url==='/unknown'){res.writeHead(200,{'Content-Type':'text/plain','Content-Disposition':'attachment; filename="chunked.txt"'});res.write(Buffer.alloc(32768,65));setTimeout(()=>{if(!res.destroyed)res.end(Buffer.alloc(32768,66));},1200);return;}
  res.writeHead(404).end();
 });await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
 browser=await chromium.launchPersistentContext(profile,{channel:'msedge',headless:true,acceptDownloads:true,downloadsPath:downloads,ignoreDefaultArgs:['--disable-extensions'],args:['--enable-unsafe-extension-debugging']});
 const cdp=await browser.browser().newBrowserCDPSession();await cdp.send('Extensions.loadUnpacked',{path:extension});
 const worker=browser.serviceWorkers()[0]||await browser.waitForEvent('serviceworker');
 for(const [name,bytes] of [['empty',0],['unknown',65536]]){
  const id=await worker.evaluate(url=>chrome.downloads.download({url,saveAs:false}),`http://127.0.0.1:${server.address().port}/${name}`);
  const complete=await until(async()=>{const item=await worker.evaluate(async id=>{const item=(await chrome.downloads.search({id}))[0];return item&&{...item,udmReceived:browserResponseReceived(item)};},id);if(!item)return;observations.push({case:name,state:item.state,totalBytes:item.totalBytes,bytesReceived:item.bytesReceived,udmReceived:item.udmReceived});assert.notEqual(item.state,'interrupted',item.error);return item.state==='complete'&&item;});
  const relative=path.relative(root,complete.filename);assert(relative&&!relative.startsWith('..')&&!path.isAbsolute(relative),'Fixture output escaped its root');
  assert.equal(fs.statSync(complete.filename).size,bytes);assert.equal(complete.bytesReceived,bytes);
  checks.push({name:name+' response completed with expected browser/file byte count',passed:true,bytes});
 }
 const created=await worker.evaluate(()=>observed);observations.push(...created.map(row=>({...row,case:row.id===created[0]?.id?'empty':'unknown'})));
 assert(observations.some(row=>row.case==='unknown'&&row.state==='in_progress'&&[0,-1].includes(row.totalBytes)),'Unknown response was not observed before completion');
 checks.push({name:'Unknown response was observed before completion; raw counters retained without assuming zero proves empty',passed:true});
 assert(observations.filter(row=>row.case==='unknown'&&row.state==='in_progress'&&[0,-1].includes(row.totalBytes)).every(row=>row.udmReceived===false));
 assert(observations.some(row=>row.case==='unknown'&&row.state==='complete'&&row.udmReceived===true));
 checks.push({name:'Production UDM predicate retains zero/unknown eligibility and recognizes the positive received total',passed:true});
})().catch(error=>{checks.push({passed:false,error:error.stack});process.exitCode=1;}).finally(async()=>{
 if(browser)await browser.close();if(server){server.closeAllConnections();await new Promise(resolve=>server.close(resolve));}
 fs.mkdirSync(root,{recursive:true});fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({scope:'Real isolated Edge downloads API, synthetic empty and 64 KiB chunked responses; UDM and the personal browser were not loaded',observations,checks},null,2));
 console.log(JSON.stringify({checks,created:observations.filter(row=>row.event==='created')},null,2));
});
