'use strict';
// Opt-in live service diagnostic, isolated from the user's browser and download history.
const {chromium}=require('playwright'),fs=require('node:fs'),path=require('node:path'),{spawn,execFileSync}=require('node:child_process');
const assert=require('node:assert/strict');
const project=path.resolve(process.env.UDM_PROJECT||path.join(__dirname,'..')),root=path.resolve(process.argv[2]),id=process.argv[3],height=Number(process.argv[4]||1080);
if(!/^[\w-]{11}$/.test(id||'')||![360,720,1080].includes(height))throw Error('Supply output directory, public video ID and 360/720/1080.');
const url='https://www.youtube.com/watch?v='+id,tag='yt'+Date.now(),report={videoId:id,requestedHeight:height,startedUtc:new Date().toISOString(),scope:'Isolated browser profile; public video; original UDM capture; no resolver'},statePath=path.join(root,'state/state.json');let context,app,worker,page,clickClock;
const delay=ms=>new Promise(r=>setTimeout(r,ms));
function jobs(){try{return JSON.parse(fs.readFileSync(statePath)).Downloads;}catch{return [];}}
(async()=>{
 fs.mkdirSync(root,{recursive:true});if(fs.existsSync(statePath))throw Error('Output state already exists.');fs.mkdirSync(path.dirname(statePath));fs.mkdirSync(path.join(root,'downloads'));
 fs.writeFileSync(statePath,JSON.stringify({Schema:1,Settings:{DownloadFolder:path.join(root,'downloads'),Connections:8,Parallel:1,Retries:0,CategoryFolders:false,SkipBrowserFileInfo:true,PrefetchFileInfo:false,SuppressProgressDialog:true,SuppressCompletionDialog:true,Sound:false},Queues:[{Name:'Main queue',Enabled:true,Parallel:1}],Downloads:[],Projects:[]}));
 app=spawn(process.env.UDM_APP_EXE||path.join(project,'release-native/UDM.exe'),['--background','--data-dir',path.dirname(statePath),'--instance-tag',tag],{windowsHide:true,stdio:'ignore'});
 context=await chromium.launchPersistentContext(path.join(root,'profile'),{channel:process.env.UDM_TEST_BROWSER||'chrome',headless:true,ignoreDefaultArgs:['--disable-extensions'],viewport:{width:1200,height:800},env:{...process.env,UDM_INSTANCE_TAG:tag},args:['--enable-unsafe-extension-debugging','--autoplay-policy=no-user-gesture-required']});
 const cdp=await context.browser().newBrowserCDPSession();await cdp.send('Extensions.loadUnpacked',{path:path.join(project,'browser/chromium')});
 worker=context.serviceWorkers()[0]||await context.waitForEvent('serviceworker');const diagnostic=await worker.evaluate(()=>nativeRequest({action:'diagnostics'}));assert.equal(diagnostic.ok,true);assert.equal(path.resolve(diagnostic.dataDirectory).toLowerCase(),path.dirname(statePath).toLowerCase());assert.equal(diagnostic.downloads,0);report.nativeVersion=diagnostic.version;report.extensionVersion=await worker.evaluate(()=>chrome.runtime.getManifest().version);report.browser=process.env.UDM_TEST_BROWSER||'chrome';page=context.pages()[0]||await context.newPage();await page.goto(url,{waitUntil:'domcontentloaded',timeout:45000});
 for(let n=0;n<35;n++){await delay(1000);const current=new URL(page.url());if((current.hostname==='google.com'||current.hostname.endsWith('.google.com'))&&current.pathname.startsWith('/sorry')){report.blocker='Google unusual-traffic challenge before playback';report.outcome='Live test blocked before media capture';break;}const snapshot=await worker.evaluate(async ({url,id})=>{const tab=(await chrome.tabs.query({url:['https://www.youtube.com/*']})).find(t=>new URL(t.url).searchParams.get('v')===id);if(!tab)return {error:'No video tab'};let context;try{context=await availableMedia({url,tabId:tab.id},{});}catch(e){return {tabId:tab.id,error:e.message,capture:UdmStreamingCapture.diagnostics(tab.id)};}const session=await UdmStreamingCapture.session(context);return {tabId:tab.id,documentId:context.documentId,choices:context.choices.map(c=>({key:c.key,height:c.height,codec:c.codec,direct:!!c.videoUrl})),session:!!session,capture:UdmStreamingCapture.diagnostics(tab.id)};},{url,id});report.capture=snapshot;if(snapshot.choices?.length&&(snapshot.session||n>=12))break;}
 await page.screenshot({path:path.join(root,'playback.png')});if(report.blocker)return;
 if(!report.capture?.choices?.some(c=>c.height===height)){report.outcome='No requested quality exposed by live service';return;}
 if(process.env.UDM_TEST_PANEL==='1'){
  const session=await context.newCDPSession(page);let clicked=false,lastOpen=0;const begin=performance.now();
  try{while(performance.now()-begin<45000){
   const tree=await session.send('Accessibility.getFullAXTree');const buttons=tree.nodes.filter(n=>!n.ignored&&n.role?.value==='button');
   const choice=buttons.find(n=>new RegExp('\\b'+height+'p\\b').test(n.name?.value||''));
   const toggle=buttons.find(n=>n.name?.value==='Download this video with UDM');
   const target=choice||((toggle?.properties?.find(p=>p.name==='expanded')?.value?.value===false&&Date.now()-lastOpen>1500)?toggle:null);
   if(target){const {model}=await session.send('DOM.getBoxModel',{backendNodeId:target.backendDOMNodeId}),q=model.content;
    if(choice){report.panelChoice=choice.name.value;await page.screenshot({path:path.join(root,'choices.png')});report.clickStartedUtc=new Date().toISOString();clickClock=performance.now();}
    await page.mouse.click((q[0]+q[2])/2,(q[1]+q[5])/2);
    if(choice){clicked=true;break;}lastOpen=Date.now();
   }await delay(150);
  }}finally{await session.detach();}
  assert(clicked,'Requested quality was not clickable in the public video panel');
  for(let n=0;n<300;n++){const job=jobs().find(j=>j.SourceUrl===url);if(job){report.handoff={ok:true,id:job.Id};report.panelOpenToJobMs=performance.now()-begin;break;}await delay(100);}
  assert(report.handoff?.ok,'Panel click did not create the isolated download');report.handoffMethod='Real panel mouse click';
 }else{
 report.handoff=await worker.evaluate(async({url,height,tabId})=>{try{const context=await availableMedia({url,tabId},{}),choice=context.choices.find(c=>c.height===height);return await mediaHandoff({url,tabId,height,formatKey:choice.key,title:'UDM live capture test'},{});}catch(e){return {ok:false,error:e.message};}},{url,height,tabId:report.capture.tabId});
 report.handoffMethod='Background function';
 }
 report.captureAfter=await worker.evaluate(tabId=>UdmStreamingCapture.diagnostics(tabId),report.capture.tabId);
 if(!report.handoff.ok){report.outcome='Capture handoff rejected';return;}
 report.samples=[];const transferClock=clickClock??performance.now();
 for(let n=0;n<150;n++){await delay(1000);const job=jobs().find(j=>j.Id===report.handoff.id);if(!job)continue;report.samples.push({elapsedMs:performance.now()-transferClock,status:job.Status,persistedReceived:job.Received,persistedTransferredBytes:job.TransferredBytes});report.download={status:job.Status,received:job.Received,error:job.Error,sha256:job.Sha256,transferredBytes:job.TransferredBytes};if(job.Status==='Complete'){report.clickToObservedCompleteMs=performance.now()-transferClock;const file=path.join(job.Folder,job.FileName);const probe=JSON.parse(execFileSync(path.join(project,'release/tools/ffprobe.exe'),['-v','error','-show_streams','-show_format','-of','json',file],{windowsHide:true}));report.output={path:file,bytes:fs.statSync(file).size,duration:probe.format.duration,streams:probe.streams.map(s=>({type:s.codec_type,codec:s.codec_name,width:s.width,height:s.height}))};assert(probe.streams.some(s=>s.codec_type==='video'&&s.height===height),'Output height differs from requested quality');assert(probe.streams.some(s=>s.codec_type==='audio'),'Output has no audio');assert.equal(jobs().length,1,'Panel created duplicate records');report.outcome='Complete';return;}if(['Failed','Paused'].includes(job.Status)){report.outcome='Native transfer did not complete';return;}}
 report.outcome='Download timed out';
})().catch(error=>{report.outcome='Live test blocked';report.error=String(error.message).slice(0,600);}).finally(async()=>{try{if(context)await context.close();}finally{if(app)app.kill();if(report.outcome!=='Complete')process.exitCode=1;report.finishedUtc=new Date().toISOString();fs.writeFileSync(path.join(root,'result.json'),JSON.stringify(report,null,2));console.log(JSON.stringify(report,null,2));}});
