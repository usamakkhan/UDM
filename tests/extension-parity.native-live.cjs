'use strict';
const assert=require('node:assert/strict'),http=require('node:http'),fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
module.exports=async({worker,page,context,until,jobs,pass,root,state})=>{
 await page.goto('http://127.0.0.1:43821/embed');await page.locator('[id^="udm-video-panel-"]').waitFor();
 const tabId=await worker.evaluate(async()=>{const tabs=await chrome.tabs.query({});return tabs.find(t=>t.url==='http://127.0.0.1:43821/embed').id;});
 await worker.evaluate(async id=>browserControls.toggle(await chrome.tabs.get(id)),tabId);
 await page.waitForFunction(()=>getComputedStyle(document.querySelector('[id^="udm-video-panel-"]')).display==='none');
 await page.reload();await page.locator('[id^="udm-video-panel-"]').waitFor({state:'attached'});
 await page.waitForFunction(()=>getComputedStyle(document.querySelector('[id^="udm-video-panel-"]')).display==='none');
 const blocked=await worker.evaluate(async id=>{try{await handoff({url:'http://127.0.0.1:43821/sample.mp4',tabId:id});return false;}catch(e){return /disabled/.test(e.message);}},tabId);assert(blocked);
 await worker.evaluate(async id=>browserControls.toggle(await chrome.tabs.get(id)),tabId);await page.locator('[id^="udm-video-panel-"]').waitFor();
 pass('Per-tab disable hides video controls across refresh and rejects handoff; enable restores them');
 await page.evaluate(()=>{for(let i=1;i<=2;i++){const a=document.createElement('a');a.href='/capture-fixture.zip?batch='+i;a.textContent='Fixture archive '+i;document.body.append(a);}});
 const opened=context.waitForEvent('page');await worker.evaluate(async id=>browserControls.collect(await chrome.tabs.get(id),false),tabId);const links=await opened;
 await links.getByLabel('Download later',{exact:true}).check();await links.getByRole('button',{name:'Add selected to UDM',exact:true}).click();await links.getByRole('status').filter({hasText:'2 links added'}).waitFor();
 const batch=jobs().filter(j=>j.Url.includes('?batch='));assert.equal(batch.length,2);assert(batch.every(j=>j.Status==='Paused'));await links.screenshot({path:path.join(root,'link-selection.png')});await links.close();
 pass('Real link-selection window sends two Download later jobs through the persistent native host');
 const body=Buffer.alloc(2*1024*1024);for(let i=0;i<body.length;i++)body[i]=(i*13+5)%251;
 let authorized=0,rejected=0;
 const server=http.createServer((req,res)=>{
  res.setHeader('Access-Control-Allow-Origin','http://127.0.0.1:43821');res.setHeader('Access-Control-Allow-Headers','authorization');res.setHeader('Access-Control-Allow-Methods','GET,HEAD,OPTIONS');
  if(req.method==='OPTIONS'){res.writeHead(204);return res.end();}
  if(req.headers.authorization!=='Bearer udm-fixture'||req.headers.origin!=='http://127.0.0.1:43821'){rejected++;res.writeHead(403);return res.end();}authorized++;
  let a=0,b=body.length-1;const range=/^bytes=(\d+)-(\d*)$/.exec(req.headers.range||'');if(range){a=+range[1];if(range[2])b=Math.min(+range[2],b);res.statusCode=206;res.setHeader('Content-Range',`bytes ${a}-${b}/${body.length}`);}
  res.setHeader('Content-Length',b-a+1);res.setHeader('Content-Type','application/octet-stream');res.setHeader('Accept-Ranges','bytes');res.setHeader('ETag','"udm-header-fixture"');res.end(req.method==='HEAD'?undefined:body.subarray(a,b+1));
 });
 await new Promise(r=>server.listen(0,'127.0.0.1',r));
 try{
  const url='http://127.0.0.1:'+server.address().port+'/protected.udmtest';
  await worker.evaluate(async()=>{const r=await chrome.storage.local.get('settings');await chrome.storage.local.set({settings:{...r.settings,cookies:true}});});
  page.on('console',msg=>{if(msg.type()==='error')console.log('BROWSER CONSOLE '+msg.text());});
  assert.equal(await page.evaluate(async u=>(await fetch(u,{headers:{Authorization:'Bearer udm-fixture'}})).status,url),200);
  await until(()=>worker.evaluate(({url,tabId})=>!!requestContext.resolve({url,tabId},true)?.headers.Authorization,{url,tabId}));
  const reply=await worker.evaluate(({url,tabId})=>handoff({url,tabId,frameId:0,referrer:'http://127.0.0.1:43821/embed',filename:'header-fixture.bin'}),{url,tabId});assert(reply.ok);
  const job=await until(()=>{const j=jobs().find(j=>j.Url===url);if(j?.Status==='Failed')throw Error(j.Error);return j?.Status==='Complete'&&j;},45000);
  assert.equal(crypto.createHash('sha256').update(fs.readFileSync(path.join(job.Folder,job.FileName))).digest('hex'),crypto.createHash('sha256').update(body).digest('hex'));
  assert(!fs.readFileSync(state,'utf8').includes('Bearer udm-fixture'));assert(authorized>=3);assert.equal(rejected,0);
  pass('Observed Authorization and Origin reach native byte-range downloads and remain encrypted in saved history',{requests:authorized,bytes:body.length});
 }finally{server.closeAllConnections();await new Promise(r=>server.close(r));await worker.evaluate(async()=>{const r=await chrome.storage.local.get('settings');await chrome.storage.local.set({settings:{...r.settings,cookies:false}});});}
 const connection=await worker.evaluate(()=>nativeClient.diagnostics());assert.equal(connection.connections,1);assert.equal(connection.persistent,1);assert(connection.completed>=6);
 pass('Browser reuses one native host for settings, multiple downloads and acknowledgements',{connection});
};
