'use strict';
const {chromium}=require('playwright');
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict'),crypto=require('node:crypto'),{spawn,execFileSync}=require('node:child_process');
const project=path.resolve(__dirname,'..'),root=path.resolve(process.argv[2]||path.join(project,'benchmarks','browser-native-live-'+Date.now())),state=path.join(root,'udm-state/state.json'),results=[],instanceTag='e2e'+Date.now();
let context,fixture,page,desktop,worker;
const wait=ms=>new Promise(r=>setTimeout(r,ms));
async function until(fn,timeout=20000){const end=Date.now()+timeout;let value;while(Date.now()<end){value=await fn();if(value)return value;await wait(150);}throw Error('Timed out waiting for test condition');}
function jobs(){return JSON.parse(fs.readFileSync(state)).Downloads;}
function digest(file){return crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');}
function pass(name,detail={}){results.push({name,passed:true,...detail});console.log('PASS '+name);}
async function clickAX(session,name){const node=await until(async()=>{const tree=await session.send('Accessibility.getFullAXTree');return tree.nodes.find(n=>!n.ignored&&n.role?.value==='button'&&name(n.name?.value||''));});const {model}=await session.send('DOM.getBoxModel',{backendNodeId:node.backendDOMNodeId});const q=model.content;await page.mouse.click((q[0]+q[2])/2,(q[1]+q[5])/2);return node.name.value;}
(async()=>{
 fs.mkdirSync(root,{recursive:true});if(fs.existsSync(state))throw Error('Use a fresh output directory; existing test state will not be replaced.');
 fs.mkdirSync(path.dirname(state),{recursive:true});fs.mkdirSync(path.join(root,'downloads'),{recursive:true});
 fs.cpSync(path.join(project,'browser/chromium'),path.join(root,'test-extension'),{recursive:true});
 const manifestPath=path.join(root,'test-extension/manifest.json'),manifest=JSON.parse(fs.readFileSync(manifestPath));manifest.host_permissions.push('http://127.0.0.1/*');fs.writeFileSync(manifestPath,JSON.stringify(manifest,null,2));
 fs.writeFileSync(state,JSON.stringify({Schema:1,Settings:{DownloadFolder:path.join(root,'downloads'),Connections:8,Parallel:1,CategoryFolders:false,DuplicatePolicy:'Numbered',SkipBrowserFileInfo:true,PrefetchFileInfo:false,SuppressProgressDialog:true,SuppressCompletionDialog:true,Sound:false},Queues:[{Name:'Main queue',Enabled:true,Parallel:1}],Downloads:[],Projects:[]}));
 desktop=spawn(path.join(project,'release/UDM.exe'),['--background','--data-dir',path.dirname(state),'--instance-tag',instanceTag],{windowsHide:true,stdio:'ignore'});desktop.on('error',e=>{console.error(e);process.exitCode=1;});
 fixture=spawn(process.execPath,[path.join(project,'native/browser-fixture.cjs')],{windowsHide:true,stdio:['ignore','pipe','pipe']});let ready='',errors='';fixture.stdout.on('data',c=>ready+=c);fixture.stderr.on('data',c=>errors+=c);await until(()=>{if(fixture.exitCode!==null)throw Error(errors);return ready.includes('http://');},30000);
 context=await chromium.launchPersistentContext(path.join(root,'test-profile'),{channel:'chrome',headless:true,ignoreDefaultArgs:['--disable-extensions'],viewport:{width:1100,height:800},env:{...process.env,UDM_INSTANCE_TAG:instanceTag},args:['--enable-unsafe-extension-debugging','--autoplay-policy=no-user-gesture-required']});
 const browserCdp=await context.browser().newBrowserCDPSession();await browserCdp.send('Extensions.loadUnpacked',{path:path.join(root,'test-extension')});
 worker=context.serviceWorkers()[0]||await context.waitForEvent('serviceworker');
 await worker.evaluate(()=>{globalThis.udmTrace=[];const trace=(type,data)=>udmTrace.push({time:Date.now(),type,data});chrome.tabs.onUpdated.addListener((id,change)=>trace('tab',{id,change}));chrome.webRequest.onHeadersReceived.addListener(e=>trace('headers',{tabId:e.tabId,frameId:e.frameId,documentId:e.documentId,documentUrl:e.documentUrl,initiator:e.initiator,url:e.url}),{urls:['http://127.0.0.1/*']});chrome.storage.onChanged.addListener((changes,area)=>{if(area==='session')trace('storage',changes);});});
 const ping=await until(async()=>{try{return await worker.evaluate(()=>chrome.runtime.sendNativeMessage('com.udm.download_manager',{action:'ping'}));}catch{return false;}});assert.equal(ping.ok,true);pass('Real Chrome extension reaches isolated native UDM',{ping,browser:context.browser()?.version()});
 await until(()=>worker.evaluate(async()=>(await chrome.scripting.getRegisteredContentScripts()).some(x=>x.id==='udm-video-panels')));
 page=context.pages()[0]||await context.newPage();const pageErrors=[];page.on('pageerror',e=>pageErrors.push(e.message));
 await page.goto('http://127.0.0.1:43821/');await page.locator('video').first().waitFor();await page.waitForFunction(()=>document.querySelector('video').videoHeight===360);
 await until(()=>page.locator('[id^="udm-video-panel-"]').count());const session=await context.newCDPSession(page);
 await clickAX(session,n=>n==='Download this video with UDM');
 let tree=await session.send('Accessibility.getFullAXTree');await page.screenshot({path:path.join(root,'direct-panel.png')});
 const before=new Set(jobs().map(j=>j.Id));const chosen=await clickAX(session,n=>n.startsWith('360p · MP4'));
 const direct=await until(()=>{const j=jobs().find(j=>!before.has(j.Id));if(j?.Status==='Failed')throw Error(j.Error);return j?.Status==='Complete'&&j;},45000);
 const directPath=direct.FilePath||path.join(direct.Folder||path.join(root,'downloads'),direct.FileName);assert.equal(digest(directPath),digest(path.join(project,'native/browser-fixture-data/sample.mp4')));pass('Trusted video-panel click downloads a byte-identical MP4 through real native messaging',{chosen,filename:direct.FileName,bytes:fs.statSync(directPath).size});
 const beforeHls=new Set(jobs().map(j=>j.Id));await page.goto('http://127.0.0.1:43821/hls');await page.waitForFunction(()=>document.querySelector('video').videoHeight===360);await until(()=>page.locator('[id^="udm-video-panel-"]').count());await clickAX(session,n=>n==='Download this video with UDM');
 const hlsChoice=await clickAX(session,n=>n.startsWith('360p · HLS'));await page.screenshot({path:path.join(root,'hls-panel.png')});
 const hls=await until(()=>{const j=jobs().find(j=>!beforeHls.has(j.Id));if(j?.Status==='Failed')throw Error(j.Error);return j?.Status==='Complete'&&j;},60000);
 const hlsPath=hls.FilePath||path.join(hls.Folder||path.join(root,'downloads'),hls.FileName);
 const probe=JSON.parse(execFileSync(path.join(project,'release/tools/ffprobe.exe'),['-v','error','-show_streams','-show_format','-of','json',hlsPath],{windowsHide:true}));assert(probe.streams.some(s=>s.codec_type==='video'&&s.height===360));assert(probe.streams.some(s=>s.codec_type==='audio'));assert(Math.abs(Number(probe.format.duration)-8)<0.2);pass('Observed recorded HLS downloads and assembles playable 360p video with audio',{chosen:hlsChoice,filename:hls.FileName,streams:probe.streams.map(s=>({type:s.codec_type,codec:s.codec_name,height:s.height})),duration:probe.format.duration});
 assert.deepEqual(pageErrors,[]);pass('No page-script errors during real extension capture');
})().catch(e=>{results.push({passed:false,error:e.stack});console.error(e);process.exitCode=1;}).finally(async()=>{if(worker)fs.writeFileSync(path.join(root,"trace.json"),JSON.stringify(await worker.evaluate(async()=>({trace:globalThis.udmTrace,session:await chrome.storage.session.get(null)})),null,2));if(page)await page.screenshot({path:path.join(root,'browser-final.png')}).catch(()=>{});fs.writeFileSync(path.join(root,'extension-results.json'),JSON.stringify({time:new Date().toISOString(),scope:'Isolated real Chrome extension and real native desktop; localhost permission pre-granted in fixture manifest only',results},null,2));if(context)await context.close();if(fixture)fixture.kill();if(desktop)desktop.kill();});
