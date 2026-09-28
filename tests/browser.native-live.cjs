'use strict';
const {chromium}=require('playwright');
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict'),crypto=require('node:crypto'),{spawn,execFileSync}=require('node:child_process');
const project=path.resolve(__dirname,'..'),root=path.resolve(process.argv[2]||path.join(project,'benchmarks','browser-native-live-'+Date.now())),state=path.join(root,'udm-state/state.json'),results=[],instanceTag='e2e'+Date.now(),browserChannel=process.env.UDM_TEST_BROWSER||'chrome';
const shortcutOnly=process.env.UDM_TEST_SHORTCUTS_ONLY==='1';
let context,fixture,page,desktop,worker;
const wait=ms=>new Promise(r=>setTimeout(r,ms));
async function until(fn,timeout=20000){const end=Date.now()+timeout;let value;while(Date.now()<end){value=await fn();if(value)return value;await wait(150);}throw Error('Timed out waiting for test condition');}
function jobs(){try{return JSON.parse(fs.readFileSync(state)).Downloads;}catch(e){if(['ENOENT','EACCES','EPERM','EBUSY'].includes(e.code))return [];throw e;}}
function digest(file){return crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');}
function pass(name,detail={}){results.push({name,passed:true,...detail});console.log('PASS '+name);}
async function clickAX(session,name,frameId){const node=await until(async()=>{const tree=await session.send('Accessibility.getFullAXTree',frameId?{frameId}:{});return tree.nodes.find(n=>!n.ignored&&n.role?.value==='button'&&name(n.name?.value||''));});const {model}=await session.send('DOM.getBoxModel',{backendNodeId:node.backendDOMNodeId});const q=model.content;await page.mouse.click((q[0]+q[2])/2,(q[1]+q[5])/2);return node.name.value;}
(async()=>{
 fs.mkdirSync(root,{recursive:true});if(fs.existsSync(state))throw Error('Use a fresh output directory; existing test state will not be replaced.');
 fs.mkdirSync(path.dirname(state),{recursive:true});fs.mkdirSync(path.join(root,'downloads'),{recursive:true});
 fs.cpSync(path.join(project,'browser/chromium'),path.join(root,'test-extension'),{recursive:true});
 if(process.env.UDM_NATIVE_HOST){const p=path.join(root,'test-extension/background.js');fs.writeFileSync(p,fs.readFileSync(p,'utf8').replace("'com.udm.download_manager'",JSON.stringify(process.env.UDM_NATIVE_HOST)));}
 const manifestPath=path.join(root,'test-extension/manifest.json'),manifest=JSON.parse(fs.readFileSync(manifestPath));manifest.host_permissions.push('http://127.0.0.1/*');manifest.permissions.push('cookies');manifest.optional_permissions=manifest.optional_permissions.filter(p=>p!=='cookies');fs.writeFileSync(manifestPath,JSON.stringify(manifest,null,2));
 fs.writeFileSync(state,JSON.stringify({Schema:1,Settings:{CaptureWebPlayers:process.env.UDM_TEST_AUTOPLAYER_ONLY==='1',CaptureForceKey:'Ctrl+Ins',CaptureBypassKey:'Alt',CaptureForceClick:false,CaptureExtensions:'udmtest zip mp4',DownloadFolder:path.join(root,'downloads'),Connections:8,Parallel:1,CategoryFolders:false,DuplicatePolicy:process.env.UDM_TEST_OVERWRITE==='1'?'Replace':'Numbered',SkipBrowserFileInfo:true,PrefetchFileInfo:false,SuppressProgressDialog:true,SuppressCompletionDialog:true,Sound:false},Queues:[{Name:'Main queue',Enabled:true,Parallel:1}],Downloads:[],Projects:[]}));
 desktop=spawn(process.env.UDM_APP_EXE||path.join(project,'release/UDM.exe'),['--background','--data-dir',path.dirname(state),'--instance-tag',instanceTag],{windowsHide:true,stdio:'ignore'});desktop.on('error',e=>{console.error(e);process.exitCode=1;});
 fixture=spawn(process.execPath,[path.join(project,'native/browser-fixture.cjs')],{windowsHide:true,stdio:['ignore','pipe','pipe']});let ready='',errors='';fixture.stdout.on('data',c=>ready+=c);fixture.stderr.on('data',c=>errors+=c);await until(()=>{if(fixture.exitCode!==null)throw Error(errors);return ready.includes('http://');},30000);
 context=await chromium.launchPersistentContext(path.join(root,'test-profile'),{channel:browserChannel,headless:true,downloadsPath:path.join(root,'browser-downloads'),ignoreDefaultArgs:['--disable-extensions'],viewport:{width:1100,height:800},env:{...process.env,UDM_INSTANCE_TAG:instanceTag},args:['--enable-unsafe-extension-debugging','--autoplay-policy=no-user-gesture-required']});
 const browserCdp=await context.browser().newBrowserCDPSession();fs.mkdirSync(path.join(root,'browser-downloads'),{recursive:true});await browserCdp.send('Browser.setDownloadBehavior',{behavior:'allow',downloadPath:path.join(root,'browser-downloads'),eventsEnabled:true});await browserCdp.send('Extensions.loadUnpacked',{path:path.join(root,'test-extension')});
 worker=context.serviceWorkers()[0]||await context.waitForEvent('serviceworker');
 await worker.evaluate(()=>{globalThis.udmTrace=[];const trace=(type,data)=>udmTrace.push({time:Date.now(),type,data});const observeKeys=keyCapture.update;keyCapture.update=async(message,sender)=>{try{const result=await observeKeys(message,sender);trace('key-lease',{active:!!message.keys,accepted:result.ok,tabId:sender.tab?.id});return result;}catch(e){trace('key-lease',{active:!!message.keys,error:e.message,tabId:sender.tab?.id});throw e;}};chrome.downloads.onCreated.addListener(item=>trace('download-created',item));chrome.downloads.onChanged.addListener(item=>trace('download-changed',item));chrome.tabs.onUpdated.addListener((id,change)=>trace('tab',{id,change}));chrome.webRequest.onHeadersReceived.addListener(e=>trace('headers',{tabId:e.tabId,frameId:e.frameId,documentId:e.documentId,documentUrl:e.documentUrl,initiator:e.initiator,url:e.url,statusCode:e.statusCode,headers:e.responseHeaders}),{urls:['http://127.0.0.1/*']},['responseHeaders']);chrome.runtime.onMessage.addListener((message,sender)=>{if(message.action==='capture-intent')trace('gesture',{intent:message.intent,url:message.url,tabId:sender.tab?.id});return false;});chrome.storage.onChanged.addListener((changes,area)=>{if(area==='session')trace('storage',changes);});});
 const ping=await until(async()=>{try{return await worker.evaluate(()=>nativeRequest({action:'ping'}));}catch{return false;}});assert.equal(ping.ok,true);const identity=await worker.evaluate(()=>nativeRequest({action:'diagnostics'}));assert.equal(path.resolve(identity.dataDirectory).toLowerCase(),path.dirname(state).toLowerCase(),'Native host must target only test history');assert.equal(identity.downloads,0);pass('Real '+browserChannel+' extension reaches isolated native UDM',{ping,browser:context.browser()?.version(),nativeVersion:identity.version});
 await until(()=>worker.evaluate(async()=>(await chrome.scripting.getRegisteredContentScripts()).some(x=>x.id==='udm-video-panels')));
 page=context.pages()[0]||await context.newPage();const pageErrors=[];page.on('pageerror',e=>pageErrors.push(e.message));const session=await context.newCDPSession(page);
 if(process.env.UDM_TEST_AUTOPLAYER_ONLY==='1'){await require('./player-auto.native-live.cjs')({worker,page,until,jobs,pass,project});return;}
 await require('./extension-parity.native-live.cjs')({worker,page,context,until,jobs,pass,root,state});
 const expectedAutomatic=Buffer.alloc(8*1024*1024);for(let i=0;i<expectedAutomatic.length;i++)expectedAutomatic[i]=(i*17+11)%251;
 if(!shortcutOnly){
 await page.goto('http://127.0.0.1:43821/');await page.locator('video').first().waitFor();await page.waitForFunction(()=>document.querySelector('video').videoHeight===360);
 await until(()=>page.locator('[id^="udm-video-panel-"]').count());await session.send('Network.enable');session.on('Network.responseReceived',e=>{if(e.response.url.includes('capture-fixture'))console.log('DOWNLOAD HTTP '+JSON.stringify({status:e.response.status,mime:e.response.mimeType,headers:e.response.headers}));});session.on('Network.loadingFailed',e=>console.log('NETWORK FAILURE '+JSON.stringify({error:e.errorText,reason:e.blockedReason}))); 
 await clickAX(session,n=>n==='Download this video with UDM');
 let tree=await session.send('Accessibility.getFullAXTree');await page.screenshot({path:path.join(root,'direct-panel.png')});
 const before=new Set(jobs().map(j=>j.Id));const chosen=await clickAX(session,n=>n.startsWith('360p · MP4'));
 const direct=await until(()=>{const j=jobs().find(j=>!before.has(j.Id));if(j?.Status==='Failed')throw Error(j.Error);return j?.Status==='Complete'&&j;},45000);
 const directPath=direct.FilePath||path.join(direct.Folder||path.join(root,'downloads'),direct.FileName);assert.equal(digest(directPath),digest(path.join(project,'native/browser-fixture-data/sample.mp4')));pass('Trusted video-panel click downloads a byte-identical MP4 through real native messaging',{chosen,filename:direct.FileName,bytes:fs.statSync(directPath).size});
 const beforeHls=new Set(jobs().map(j=>j.Id));await page.goto('http://127.0.0.1:43821/hls');await page.waitForFunction(()=>document.querySelector('video').videoHeight===360);await until(()=>page.locator('[id^="udm-video-panel-"]').count());await clickAX(session,n=>n==='Download this video with UDM');
 const hlsChoice=await clickAX(session,n=>n.includes('MP4 · 360p'));await page.screenshot({path:path.join(root,'hls-panel.png')});
 const hls=await until(()=>{const j=jobs().find(j=>!beforeHls.has(j.Id));if(j?.Status==='Failed')throw Error(j.Error);return j?.Status==='Complete'&&j;},60000);
 const hlsPath=hls.FilePath||path.join(hls.Folder||path.join(root,'downloads'),hls.FileName);
 const probe=JSON.parse(execFileSync(path.join(project,'release/tools/ffprobe.exe'),['-v','error','-show_streams','-show_format','-of','json',hlsPath],{windowsHide:true}));assert(probe.streams.some(s=>s.codec_type==='video'&&s.height===360));assert(probe.streams.some(s=>s.codec_type==='audio'));assert(Math.abs(Number(probe.format.duration)-8)<0.2);pass('Observed recorded HLS downloads and assembles playable 360p video with audio',{chosen:hlsChoice,filename:hls.FileName,streams:probe.streams.map(s=>({type:s.codec_type,codec:s.codec_name,height:s.height})),duration:probe.format.duration});
 if(process.env.UDM_TEST_LIVE_HLS==='1')await require('./live-hls.native-live.cjs')({page,session,clickAX,until,jobs,pass,project,root});
 // A real Chrome Downloads API event must reach native UDM before cancellation.
 const captureUrl='http://127.0.0.1:43821/capture-fixture.udmtest';
 await worker.evaluate(async()=>{const current=await chrome.storage.local.get('settings');await chrome.storage.local.set({settings:{...current.settings,capture:true,extensions:['udmtest']}});});
 await page.goto('http://127.0.0.1:43821/download');await page.getByRole('link',{name:'Download fixture file'}).click();
 const browserDownload=await until(async()=>{const items=await worker.evaluate(url=>chrome.downloads.search({url}),captureUrl);return items[0]?.id;});
 const autoJob=await until(()=>{const j=jobs().find(j=>j.Url===captureUrl);if(j?.Status==='Failed')throw Error(j.Error);return j?.Status==='Complete'&&j;},45000);
 const browserItem=await until(async()=>{const [item]=await worker.evaluate(id=>chrome.downloads.search({id}),browserDownload);return item?.state==='interrupted'&&item;});assert.equal(browserItem.error,'USER_CANCELED');
 assert.equal(digest(path.join(autoJob.Folder,autoJob.FileName)),crypto.createHash('sha256').update(expectedAutomatic).digest('hex'));
 if(process.env.UDM_TEST_OVERWRITE==='1'){
  const originalPath=path.join(autoJob.Folder,autoJob.FileName),beforeIds=new Set(jobs().map(j=>j.Id));
  await page.goto('http://127.0.0.1:43821/download');await page.getByRole('link',{name:'Download fixture file'}).click();
  const overwritten=await until(()=>{const j=jobs().find(j=>j.Url===captureUrl&&!beforeIds.has(j.Id));if(j?.Status==='Failed')throw Error(j.Error);return j?.Status==='Complete'&&j;},45000);
  assert.equal(path.join(overwritten.Folder,overwritten.FileName),originalPath);assert.equal(digest(originalPath),crypto.createHash('sha256').update(expectedAutomatic).digest('hex'));
  const previous=jobs().find(j=>j.Id===autoJob.Id);assert.equal(previous.PreviousVersionOf,overwritten.Id);assert.notEqual(path.join(previous.Folder,previous.FileName),originalPath);assert.equal(digest(path.join(previous.Folder,previous.FileName)),digest(originalPath));
  pass('Second browser capture honors remembered overwrite at the original filename and retains the previous file');
 }
 await worker.evaluate(async()=>{const current=await chrome.storage.local.get('settings');await chrome.storage.local.set({settings:{...current.settings,capture:false}});});pass('Automatic '+browserChannel+' file capture hands off to native UDM and cancels the browser transfer',{bytes:expectedAutomatic.length,browserState:browserItem.state,nativeState:autoJob.Status});

 }
 // Exercise the real trusted gesture, background event and native transfer together.
 async function shortcut(name,url,key,capture){
  await worker.evaluate(async capture=>{const current=await chrome.storage.local.get('settings');await chrome.storage.local.set({settings:{...current.settings,capture,extensions:['udmtest']}});},capture);
  await page.goto('http://127.0.0.1:43821/download');
  await page.waitForTimeout(250);
  await page.locator('a').evaluate((a,value)=>{a.href=value.url;a.textContent=value.name;},{url,name});
  const keys=Array.isArray(key)?key:[key];for(const k of keys)await page.keyboard.down(k);try{await page.getByRole('link',{name,exact:true}).click();}finally{for(const k of [...keys].reverse())await page.keyboard.up(k);}
  return until(async()=>{const items=await worker.evaluate(url=>chrome.downloads.search({url}),url);return items[0]?.id;});
 }
 const forceUrl='http://127.0.0.1:43821/capture-fixture.udmforce?test=force';
 const fixtureProbe=await fetch(forceUrl,{headers:{Range:'bytes=0-0'}});assert.equal(fixtureProbe.status,206,'Shortcut fixture HTTP status');assert.equal((await fixtureProbe.arrayBuffer()).byteLength,1,'Shortcut fixture byte range');
 const forcedId=await shortcut('Force fixture download',forceUrl,['Control','Insert'],true);
 const forced=await until(()=>{const j=jobs().find(j=>j.Url===forceUrl);if(j?.Status==='Failed')throw Error(j.Error);return j?.Status==='Complete'&&j;},45000);
 const forcedBrowser=await until(async()=>{const [item]=await worker.evaluate(id=>chrome.downloads.search({id}),forcedId);return item?.state==='interrupted'&&item;});assert.equal(forcedBrowser.error,'USER_CANCELED');
 assert.equal(digest(path.join(forced.Folder,forced.FileName)),crypto.createHash('sha256').update(expectedAutomatic).digest('hex'));
 pass('Trusted Ctrl+Insert click bypasses both file-type filters and completes the native download',{bytes:forced.Size});
 const bypassUrl='http://127.0.0.1:43821/capture-fixture.udmtest?test=bypass';
 const bypassId=await shortcut('Keep fixture in browser',bypassUrl,'Alt',true);
 await until(async()=>{const [item]=await worker.evaluate(id=>chrome.downloads.search({id}),bypassId);return item?.state==='complete';},30000);assert(!jobs().some(j=>j.Url===bypassUrl));
 pass('Trusted Alt-click keeps a normally captured file in the browser without creating a UDM job');
 const heldUrl='http://127.0.0.1:43821/capture-fixture.udmforce?test=held';
 await page.goto('http://127.0.0.1:43821/download');await page.waitForTimeout(400);
 await page.evaluate(url=>{const b=document.createElement('button');b.textContent='Delayed download';b.onclick=()=>setTimeout(()=>{const a=document.querySelector('a');a.href=url;a.click();},1500);document.body.append(b);},heldUrl);
 await page.getByRole('button',{name:'Delayed download'}).click();await page.keyboard.down('Control');await page.keyboard.down('Insert');
 try{const held=await until(()=>{const j=jobs().find(j=>j.Url===heldUrl);if(j?.Status==='Failed')throw Error(j.Error);return j?.Status==='Complete'&&j;},45000);assert.equal(digest(path.join(held.Folder,held.FileName)),crypto.createHash('sha256').update(expectedAutomatic).digest('hex'));}
 finally{await page.keyboard.up('Insert');await page.keyboard.up('Control');}
 pass('Holding Ctrl+Insert captures a delayed script download without any link click');
 const disabledUrl='http://127.0.0.1:43821/capture-fixture.udmforce?test=disabled';
 const disabledId=await shortcut('Capture disabled fixture',disabledUrl,['Control','Insert'],false);
 await until(async()=>{const [item]=await worker.evaluate(id=>chrome.downloads.search({id}),disabledId);return item?.state==='complete';},30000);assert(!jobs().some(j=>j.Url===disabledUrl));
 pass('Trusted force gesture cannot enable automatic capture when the browser opt-in is off');

 if(!shortcutOnly){
 // Real iframe navigation leaves the top-level tab intact and reuses its frame ID.
 await page.goto('http://127.0.0.1:43821/isolation');let embedded=await until(()=>page.frame({name:'player'}));await embedded.waitForFunction(()=>document.querySelector('video').videoHeight===360);
 const oldCapture=await until(()=>worker.evaluate(async()=>Object.values(await chrome.storage.session.get(null)).filter(Array.isArray).flat().find(x=>x.url?.includes('master-before.m3u8'))));assert(oldCapture.documentId);
 await embedded.goto('http://127.0.0.1:43821/hls?catalog=after');await embedded.waitForFunction(()=>document.querySelector('video').videoHeight===360);
 const newCapture=await until(()=>worker.evaluate(async()=>Object.values(await chrome.storage.session.get(null)).filter(Array.isArray).flat().find(x=>x.url?.includes('master-after.m3u8'))));assert.equal(newCapture.frameId,oldCapture.frameId);assert.notEqual(newCapture.documentId,oldCapture.documentId);
 const frameTree=await session.send('Page.getFrameTree'),embeddedId=frameTree.frameTree.childFrames.find(f=>f.frame.name==='player').frame.id;
 await clickAX(session,n=>n==='Download this video with UDM',embeddedId);
 const menu=await until(async()=>{const ax=await session.send('Accessibility.getFullAXTree',{frameId:embeddedId});const labels=ax.nodes.filter(n=>!n.ignored&&n.role?.value==='button').map(n=>[n.name?.value,n.description?.value].filter(Boolean).join(' '));return labels.some(n=>n.includes('master-after.m3u8'))&&labels;});
 assert(!menu.some(n=>n.includes('master-before.m3u8')));await page.screenshot({path:path.join(root,'frame-navigation-panel.png')});pass('Real iframe navigation retains the frame ID but excludes previous-document playlists',{frameId:newCapture.frameId,oldDocument:oldCapture.documentId,newDocument:newCapture.documentId});
 }
 assert.deepEqual(pageErrors,[]);pass('No page-script errors during real extension capture');
})().catch(e=>{results.push({passed:false,error:e.stack});console.error(e);process.exitCode=1;}).finally(async()=>{
 try{if(worker)fs.writeFileSync(path.join(root,'trace.json'),JSON.stringify(await worker.evaluate(async()=>({trace:globalThis.udmTrace,session:await chrome.storage.session.get(null),local:await chrome.storage.local.get(null),intents:Array.from(captureIntents.values())})),null,2));}catch(e){console.error('Trace capture:',e.message);}
 try{if(page)await page.screenshot({path:path.join(root,'browser-final.png')});}catch{}
 try{fs.writeFileSync(path.join(root,'extension-results.json'),JSON.stringify({time:new Date().toISOString(),browser:browserChannel,extensionVersion:JSON.parse(fs.readFileSync(path.join(root,'test-extension/manifest.json'))).version,scope:'Isolated real browser extension and real native desktop; localhost and cookie permission pre-granted in the isolated fixture manifest',results},null,2));}finally{try{if(context)await context.close();}finally{if(fixture)fixture.kill();if(desktop)desktop.kill();}}
});
