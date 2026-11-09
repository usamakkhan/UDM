'use strict';
const {chromium}=require('playwright');
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict'),crypto=require('node:crypto'),{spawn,execFileSync}=require('node:child_process');
const project=path.resolve(process.env.UDM_TEST_PROJECT||path.join(__dirname,'..')),out=process.argv[2]&&path.resolve(process.argv[2]),channel=process.argv[3]||'msedge',tag='ythandoff'+Date.now(),url='https://www.youtube.com/watch?v=BewnzhHlQuk';
const exe=path.resolve(process.env.UDM_APP_EXE||path.join(project,'release/UDM.exe'));
const hostName='com.udm.'+tag;
const browserKey={msedge:'Microsoft\\Edge',chrome:'Google\\Chrome'}[channel];
const registry='HKCU\\Software\\'+browserKey+'\\NativeMessagingHosts\\'+hostName;
const download=process.argv[4]==='download',prefetch=process.argv[4]==='prefetch',recover=prefetch||process.argv[4]==='recover';
const coldDialog=process.argv[4]==='cold-dialog';
const targetHeight=Number(process.env.UDM_YOUTUBE_TEST_HEIGHT||'1080');
let context,desktop,worker,page,ownedOutput=false,registered=false;
const report={timestamp:new Date().toISOString(),scope:'Fresh '+channel+' profile and isolated native history',stages:[]};
const sleep=ms=>new Promise(r=>setTimeout(r,ms));
const phase=name=>{report.phase=name;console.log('PHASE '+name);};
async function bounded(promise,name,ms=25000){phase(name);let timer;try{return await Promise.race([promise,new Promise((_,reject)=>{timer=setTimeout(()=>reject(Error(name+' timed out')),ms);})]);}finally{clearTimeout(timer);}}
async function until(fn,ms=30000){const end=Date.now()+ms;while(Date.now()<end){const value=await fn();if(value)return value;await sleep(250);}throw Error('Test condition timed out');}
const readJson=file=>JSON.parse(fs.readFileSync(file,'utf8').replace(/^\uFEFF/,''));
const readState=()=>readJson(path.join(out,'state/state.json'));
function recoverySeed(settings){
 const seed=path.resolve(process.env.UDM_STREAM_RECOVERY_SEED||'');
 const seedReport=readJson(path.join(seed,'result.json'));
 const state=readJson(path.join(seed,'state/state.json'));
 assert.equal(seedReport.fixtureRegistrationRemoved,true,'Use a completed isolated browser fixture as the seed');
 assert.equal(state.Downloads.length,1);const job=state.Downloads[0];
 assert.equal(job.SourceUrl,url);assert.equal(job.MediaHeight,targetHeight);assert.equal(job.SabrSessionRefreshRequired,true);
 assert.match(job.Id,/^[a-f0-9]{32}$/);assert(job.SabrRetainedBytes>0);assert.equal(path.basename(job.FileName),job.FileName);
 const source=path.join(seed,'state/media',job.Id),media=path.join(out,'state/media',job.Id);
 const indexPath=path.join(source,'stream-cache.json'),index=readJson(indexPath);
 fs.mkdirSync(media,{recursive:true});fs.copyFileSync(indexPath,path.join(media,'stream-cache.json'));
 let copied=0;
 for(const [key,row] of Object.entries(index.parts)){
  assert.match(key,/^(video|audio)-(init|\d{1,10})$/);
  const name='retained-'+key+'.sabr',original=path.join(source,name),destination=path.join(media,name);
  assert(fs.lstatSync(original).isFile());const bytes=fs.readFileSync(original);
  assert.equal(bytes.length,row.bytes);assert.equal(crypto.createHash('sha256').update(bytes).digest('hex'),row.sha256.toLowerCase());
  fs.copyFileSync(original,destination);copied+=row.bytes;
 }
 assert.equal(copied,job.SabrRetainedBytes);job.Folder=settings.DownloadFolder;job.MediaFolder=media;
 for(const track of [job.Video,job.Audio]){assert.equal(path.basename(track.FileName),track.FileName);track.Folder=media;}
 delete job.ProtectedRefreshOffer;
 report.seedBytes=copied;report.seedTimelineMs=job.StreamCompletedMs;
 fs.writeFileSync(path.join(out,'.udm-live-recovery-fixture'),'owned fixture');
 return job;
}
async function waitForPlayback(){
 await until(async()=>{try{return await page.evaluate(id=>{const p=document.getElementById('movie_player');const skip=document.querySelector('.ytp-skip-ad-button,.ytp-ad-skip-button-modern,.ytp-ad-skip-button');if(skip)skip.click();return p?.getVideoData?.().video_id===id&&!p.classList.contains('ad-showing')&&document.querySelector('video')?.readyState>=2;},new URL(url).searchParams.get('v'));}catch{return false;}},45000);
}
async function clickPanelButton(cdp,matches){
  const node=await until(async()=>{
   const {nodes}=await cdp.send('Accessibility.getFullAXTree');
   return nodes.find(n=>!n.ignored&&n.role?.value==='button'&&matches(n.name?.value||''));
  },25000);
  const {model:{content}}=await cdp.send('DOM.getBoxModel',{backendNodeId:node.backendDOMNodeId});
  const epochMs=Date.now();
  await page.mouse.click((content[0]+content[2])/2,(content[1]+content[5])/2);
  return {epochMs,label:node.name.value};
}
async function observeColdDialog(){
 const cdp=await context.newCDPSession(page);
 phase('Open the public video panel');
 const opened=await clickPanelButton(cdp,name=>name==='Download this video with UDM');
 const quality=new RegExp('^'+targetHeight+'p\\b');
 const selected=await clickPanelButton(cdp,name=>quality.test(name)&&name.includes('MP4'));
 report.formatLabel=selected.label;
 report.panelToSelectionMs=selected.epochMs-opened.epochMs;
 phase('Observe native File Info and prefetch');
 const evidence=await until(()=>{
  try{return readJson(path.join(out,'file-info-result.json'));}catch(e){if(e.code==='ENOENT'||e instanceof SyntaxError)return null;throw e;}
 },165000);
 assert(['Complete','Held'].includes(evidence.status),evidence.error||'File Info outcome missing');
 const jobs=readState().Downloads;
 assert.equal(jobs.length,1);
 const job=jobs[0];
 assert.equal(job.MediaHeight,targetHeight);
 report.fileInfo=evidence;
 report.selectionToNativeAdmissionMs=evidence.admissionEpochMs-selected.epochMs;
 report.selectionToActionableMs=evidence.actionableEpochMs-selected.epochMs;
 if(evidence.firstReceivedObservationEpochMs)report.selectionToFirstReceivedMs=evidence.firstReceivedObservationEpochMs-selected.epochMs;
 if(evidence.status==='Held'){
  assert.equal(job.SabrSessionRefreshRequired,true);
  assert.equal(evidence.held.retainedBytes,job.SabrRetainedBytes||0);
  assert.equal(evidence.sizeBeforeStart,-1);
  assert(!fs.existsSync(path.join(job.Folder,job.FileName)));
  report.selectionToRefreshVisibleMs=evidence.held.refreshVisibleEpochMs-selected.epochMs;
  console.log('PASS public selection opened File Info and preserved the held checkpoint');
 }else{
  assert.equal(evidence.sizeBeforeStart,job.Size);
  report.selectionToExactSizeMs=evidence.exactVisibleEpochMs-selected.epochMs;
  await verifyComplete(job);
 }
}
async function verifyComplete(job){
 const target=job.FilePath||path.join(job.Folder,job.FileName);
 const ffprobe=process.env.UDM_FFPROBE||path.join(path.dirname(exe),'tools/ffprobe.exe');
 const ffmpeg=path.join(path.dirname(ffprobe),'ffmpeg.exe');
 const probe=JSON.parse(execFileSync(ffprobe,['-v','error','-show_streams','-show_format','-of','json',target],{windowsHide:true}));
 report.download={bytes:fs.statSync(target).size,duration:Number(probe.format.duration),streams:probe.streams.map(s=>({type:s.codec_type,codec:s.codec_name,width:s.width,height:s.height}))};
 assert.equal(report.download.bytes,job.Size);
 assert(probe.streams.some(s=>s.codec_type==='video'&&s.height===job.MediaPixelHeight));assert(probe.streams.some(s=>s.codec_type==='audio'));assert(Number(probe.format.duration)>60);
 const digest=crypto.createHash('sha256');
 for await(const chunk of fs.createReadStream(target))digest.update(chunk);
 report.download.sha256=digest.digest('hex');assert.equal(report.download.sha256,job.Sha256.toLowerCase());
 const expected=Number(job.StreamDurationMs||job.MediaLengthMs||0)/1000;
 if(expected>0)assert(Math.abs(report.download.duration-expected)<2,'Published duration differs from the selected source');
 report.download.decodedSeconds={};
 for(const [track,map] of [['video','0:v:0'],['audio','0:a:0']]){
  const progress=execFileSync(ffmpeg,['-v','error','-xerror','-nostdin','-nostats','-progress','pipe:1','-i',target,'-map',map,'-f','null','-'],{encoding:'utf8',windowsHide:true,timeout:300000});
  const ends=Array.from(progress.matchAll(/^out_time_us=(\d+)\r?$/gm),m=>Number(m[1])/1e6);
  assert(ends.length>0&&/^progress=end\r?$/m.test(progress),`${track} decode produced no end timestamp`);
  report.download.decodedSeconds[track]=ends.at(-1);
  assert(ends.at(-1)>=(expected||report.download.duration)-2,`${track} decode ended early`);
 }
 report.download.decodedToEnd=true;console.log('PASS actual captured video and audio decode through EOF');
}
async function refreshAndResume(seed,attempts){
 report.refresh=[];
 const clickRecoveryPanel=process.env.UDM_STREAM_RECOVERY_PANEL==='1';
 const seek=process.env.UDM_STREAM_RECOVERY_SEEK==='1';
 const expectSerial=process.env.UDM_STREAM_RECOVERY_EXPECT_SERIAL==='1';
 const allowHeld=process.env.UDM_STREAM_RECOVERY_ALLOW_HELD==='1';
 let cursor=seed.StreamCompletedMs;
 if(seek){
  cursor=0;
  for(const range of seed.SabrRanges||[]){
   if(range.Start!==cursor)break;
   cursor+=range.Done;
  }
  assert(cursor>=1000,'Expected a verified zero-based recovery prefix');
 }
 for(let i=1;i<=attempts;i++){
  phase(`Open recovery page ${i}`);
  const markerPath=path.join(out,`refresh-page-${i}.txt`);
  const {marker,page:recoveryPage}=await until(()=>fs.existsSync(markerPath)&&readJson(markerPath),15000);
  assert.match(marker,/^#udm-stream-refresh=[a-f0-9]{32}\.v\d{1,6}$/);
  if(page)await page.close();page=await context.newPage();
  const target=new URL(seek?recoveryPage:url);
  if(seek)assert.equal(target.searchParams.get('t'),`${Math.floor(cursor/1000)}s`);
  target.hash=marker;
  const address=target.href;
  await page.goto(address,{waitUntil:'domcontentloaded',timeout:35000});
  phase(`Wait for recovery playback ${i}`);await waitForPlayback();
  if(seek){
   const seconds=cursor/1000;
   phase(`Play at the retained cursor ${i}`);
   await until(()=>page.evaluate(time=>{const video=document.querySelector('video');return !video.seeking&&video.readyState>=2&&video.currentTime>=time-1;},seconds),25000);
  }
  if(process.argv[5]==='warm')await sleep(6000);
  let result;
  if(clickRecoveryPanel){
   phase(`Request the saved quality with one panel click ${i}`);
   const workerUrl=worker.url();
   worker=context.serviceWorkers().find(current=>current.url()===workerUrl)||
    await context.waitForEvent('serviceworker',{predicate:current=>current.url()===workerUrl,timeout:15000});
   await worker.evaluate(()=>{
    globalThis.recoverySubmissions=0;
    globalThis.recoveryCaptureTiming=[];
    globalThis.recoveryHandoffOriginal??=globalThis.mediaHandoff;
    globalThis.mediaHandoff=(...args)=>{recoverySubmissions++;return recoveryHandoffOriginal(...args);};
    globalThis.recoveryReadOriginal??=globalThis.readSabrSession;
    globalThis.readSabrSession=async context=>{
     const selection=await recoveryReadOriginal(context);
     if(selection&&recoveryCaptureTiming.length<8){
      const now=Date.now(),page=context.snapshot?.streamSession;
      recoveryCaptureTiming.push({source:selection.captureSource||'page',
       selectedAgeMs:now-selection.capturedAt,bundledAgeMs:page?now-page.capturedAt:null});
     }
     return selection;
    };
   });
   const cdp=await context.newCDPSession(page);
   const selected=await clickPanelButton(cdp,name=>name==='Download this video with UDM');
   await until(()=>{
    try{return !!readState().Downloads[0]?.ProtectedRefreshOffer;}
    catch(e){if(['ENOENT','EPERM','EBUSY','EACCES'].includes(e.code))return false;throw e;}
   },25000);
   const submissions=await worker.evaluate(()=>recoverySubmissions);
   assert.equal(submissions,1,'One panel click must submit exactly one replacement');
   result={ok:true,refreshPending:true,clicks:1,milliseconds:Date.now()-selected.epochMs,
    captureTiming:await worker.evaluate(()=>recoveryCaptureTiming)};
   await cdp.detach();
  }else result=await bounded(worker.evaluate(async ({address,format,height})=>{
   const marker=new URL(address).hash;
   const tab=(await chrome.tabs.query({})).find(t=>youtubeId(t.url)===youtubeId(address)&&new URL(t.url).hash===marker);
   if(!tab)return {error:'The isolated recovery tab did not keep the marker'};
   const sender={id:chrome.runtime.id};const message={action:'media',url:tab.url,tabId:tab.id,height};
   const current=await availableMedia(message,sender);
   const choice=current.choices.find(c=>c.height===height&&c.formatId===format);
   if(!choice)return {error:'The original representation is unavailable'};
   try{const reply=await mediaHandoff({...message,formatKey:choice.key},sender);return {ok:reply.ok,refreshPending:reply.refreshPending};}
   catch(e){return {error:e.message};}
  },{address,format:seed.MediaFormatId,height:seed.MediaHeight}),`Capture replacement ${i}`,25000);
  report.refresh.push({offer:result,...(seek?{playbackCursorMs:cursor}:{})});
  assert.equal(result.ok,true,JSON.stringify(result));assert.equal(result.refreshPending,true,JSON.stringify(result));
  const before=readState().Downloads;
  assert.equal(before.length,1);assert.equal(before[0].Id,seed.Id);assert.equal(before[0].SabrSessionRefreshRequired,true);
  fs.writeFileSync(path.join(out,`save-and-resume-${i}`),'Apply reviewed session');
  phase(`Resume verified download ${i}`);
  const resultPath=path.join(out,`refresh-result-${i}.json`);
  const resumed=await until(()=>fs.existsSync(resultPath)&&readJson(resultPath),155000);
  report.refresh.at(-1).resume=resumed;
  if(prefetch&&resumed.status!=='Complete')throw Error(resumed.error||'File Info did not complete the verified prefetch');
  assert(resumed.retainedBefore>=report.seedBytes,'Verified seed segments were lost');
  assert.equal(resumed.reused,resumed.retainedBefore,'Resume must reuse the complete verified checkpoint');
  const jobs=readState().Downloads;assert.equal(jobs.length,1);assert.equal(jobs[0].Id,seed.Id);
  if(expectSerial){
   assert.equal(resumed.configuredConnections,seed.Connections);
   assert.equal(resumed.effectiveConnections,1);
   assert.equal(resumed.peakRequests,1);
   assert.equal(jobs[0].SabrSerialRecovery,true);
   assert(resumed.timelineMs>cursor,'The fresh single-connection attempt must advance the verified timeline');
  }
  if(resumed.status==='Complete'){
   await verifyComplete(jobs[0]);
   if(prefetch){
    assert.equal(report.download.bytes,resumed.sizeBeforeStart);
    assert.equal(report.download.sha256,resumed.sha256BeforeStart.toLowerCase());
   }
   return;
  }
  if(!resumed.refreshRequired)throw Error(resumed.error||'The transfer failed without a refresh hold');
  if(allowHeld&&i===attempts){
   assert(resumed.timelineMs>cursor,'The final held attempt must add verified playback coverage');
   report.outcome='Held with verified progress';return;
  }
  cursor=resumed.timelineMs;
 }
 throw Error('Public transfer still needs a fresh browser session');
}
(async()=>{
 assert(browserKey,'Use the msedge or chrome browser channel');
 assert(Number.isInteger(targetHeight)&&targetHeight>0,'Expected a positive video quality');
 assert(out&&!fs.existsSync(out),'Use a fresh output directory');fs.mkdirSync(path.join(out,'state'),{recursive:true});ownedOutput=true;
 const settings={DownloadFolder:path.join(out,'downloads'),SkipBrowserFileInfo:download,PrefetchFileInfo:prefetch||coldDialog,SuppressProgressDialog:true,SuppressCompletionDialog:true,Sound:false,CloseToTray:false,ClipboardMonitor:false};
 const seed=recover?recoverySeed(settings):null;
 const attempts=Number(process.env.UDM_STREAM_RECOVERY_ATTEMPTS||'3');
 fs.writeFileSync(path.join(out,'state/state.json'),JSON.stringify({Schema:1,Settings:settings,Downloads:seed?[seed]:[],Queues:[{Name:'Main queue',Enabled:true,Parallel:1}],Projects:[]}));
 const extension=path.resolve(out,'extension');
 fs.cpSync(path.join(project,'browser/chromium'),extension,{recursive:true});
 const manifest=JSON.parse(fs.readFileSync(path.join(extension,'manifest.json')));
 const id=crypto.createHash('sha256').update(Buffer.from(manifest.key,'base64')).digest('hex').slice(0,32).replace(/[0-9a-f]/g,c=>String.fromCharCode(97+parseInt(c,16)));
 const nativeHost=path.join(path.dirname(exe),'Udm.NativeHost.exe');
 assert(fs.existsSync(exe)&&fs.existsSync(nativeHost),'Stage UDM.exe and the matching native host together');
 const hostFile=path.resolve(out,'host.json');
 fs.writeFileSync(hostFile,JSON.stringify({name:hostName,description:'Isolated UDM YouTube fixture',path:nativeHost,type:'stdio',allowed_origins:['chrome-extension://'+id+'/']}));
 const background=path.join(extension,'background.js');
 const script=fs.readFileSync(background,'utf8');
 assert(script.includes("'com.udm.download_manager'"),'Native host identifier was not found');
 fs.writeFileSync(background,script.replace("'com.udm.download_manager'",JSON.stringify(hostName)));
 let exists=false;
 try{execFileSync('reg.exe',['query',registry],{windowsHide:true,stdio:'ignore'});exists=true;}catch{}
 assert(!exists,'Fixture registry name is already in use');
 execFileSync('reg.exe',['add',registry,'/ve','/t','REG_SZ','/d',hostFile,'/f'],{windowsHide:true,stdio:'ignore'});registered=true;
 if(recover){
  const driver=path.resolve(process.env.UDM_STREAM_RECOVERY_DRIVER||'');
  assert(Number.isInteger(attempts)&&attempts>=1&&attempts<=5);assert(fs.statSync(driver).isFile());
  if(prefetch)assert.equal(attempts,1,'File Info fixture expects one final replacement');
  desktop=spawn(driver,prefetch?['--browser-prefetch',out]:['--browser-refresh',out,String(attempts)],{windowsHide:true,stdio:'ignore',env:{...process.env,UDM_INSTANCE_TAG:tag}});
 }else if(coldDialog){
  const driver=process.env.UDM_FILE_INFO_TEST_EXE;
  assert(driver&&fs.statSync(driver).isFile(),'Stage the File Info test executable');
  fs.writeFileSync(path.join(out,'.udm-live-file-info-fixture'),'isolated public File Info');
  desktop=spawn(driver,['--browser-cold-file-info',out],{windowsHide:true,stdio:'ignore',
   env:{...process.env,UDM_INSTANCE_TAG:tag,UDM_FILE_INFO_OBSERVE_HELD:'1',UDM_FILE_INFO_TRACE:'1'}});
 }else desktop=spawn(exe,['--background','--data-dir',path.resolve(out,'state'),'--instance-tag',tag],{windowsHide:true,stdio:'ignore'});
 context=await bounded(chromium.launchPersistentContext(path.join(out,'profile'),{channel,headless:true,ignoreDefaultArgs:['--disable-extensions'],viewport:{width:1100,height:800},env:{...process.env,UDM_INSTANCE_TAG:tag},args:['--enable-unsafe-extension-debugging','--autoplay-policy=no-user-gesture-required']}),'Launch isolated '+channel);
 const control=await context.browser().newBrowserCDPSession();await bounded(control.send('Extensions.loadUnpacked',{path:extension}),'Load UDM');
 worker=context.serviceWorkers().find(w=>w.url().includes(id))||await context.waitForEvent('serviceworker',{predicate:w=>w.url().includes(id),timeout:15000});
 report.version=await bounded(worker.evaluate(()=>chrome.runtime.getManifest().version),'Check worker version');
 assert.equal(report.version,manifest.version);
 const identity=await bounded(worker.evaluate(host=>chrome.runtime.sendNativeMessage(host,{action:'diagnostics'}),hostName),'Native diagnostics',15000);
 assert.equal(path.resolve(identity.dataDirectory).toLowerCase(),path.resolve(out,'state').toLowerCase());assert.equal(identity.downloads,seed?1:0);
 if(process.env.UDM_EXPECT_NATIVE_VERSION)assert.equal(identity.version,process.env.UDM_EXPECT_NATIVE_VERSION);
 report.nativeConnection=true;report.nativeVersion=identity.version;
 if(seed){await refreshAndResume(seed,attempts);return;}
 await worker.evaluate(()=>{globalThis.handoffTrace=[];for(const name of ['mediaContext','availableMedia','sabrFor','mediaHandoff']){const fn=globalThis[name];globalThis[name]=async(...args)=>{const start=Date.now();handoffTrace.push({name,event:'start'});try{const v=await fn(...args);handoffTrace.push({name,event:'end',milliseconds:Date.now()-start,available:name==='sabrFor'?!!v:undefined});return v;}catch(e){handoffTrace.push({name,event:'error',message:e.message});throw e;}}}});
 phase('Load public YouTube page');page=await context.newPage();await page.goto(url,{waitUntil:'domcontentloaded',timeout:45000});
 await waitForPlayback();
 if(coldDialog){await observeColdDialog();return;}
 if(process.argv[5]==='warm'){phase('Wait for ordinary browser playback to settle');await sleep(15000);}
 report.capture=await bounded(worker.evaluate(async ({url,height})=>{
  const tab=(await chrome.tabs.query({url}))[0];
  const rows=await chrome.scripting.executeScript({target:{tabId:tab.id},world:'MAIN',func:()=>{const id=new URL(location.href).searchParams.get('v');const p=document.getElementById('movie_player');return {videoId:id,playerId:p?.getVideoData?.()?.video_id,sabr:globalThis.__udmCaptureV1?.diagnostics?.(),session:!!globalThis.__udmCaptureV1?.session?.(id),resources:performance.getEntriesByType('resource').filter(e=>{try{return new URL(e.name).hostname.endsWith('.googlevideo.com');}catch{return false;}}).slice(-8).map(e=>{const u=new URL(e.name);return {path:u.pathname,keys:[...u.searchParams.keys()]};})};}});
  const context=await availableMedia({url,tabId:tab.id},{id:chrome.runtime.id});const choice=context.choices.find(c=>c.height===height);const session=choice?await sabrFor(context,choice):null;
  let sessionMetadata=null;if(session){const bytes=Uint8Array.from(atob(session.body),x=>x.charCodeAt(0));const fields=__udmUmpV1.fields(bytes);const sc=fields.find(f=>f.id===19&&f.wire===2);const token=sc?__udmUmpV1.fields(sc.value).find(f=>f.id===2&&f.wire===2):null;sessionMetadata={source:session.captureSource||'page',ageMilliseconds:Date.now()-session.capturedAt,hasBrowserAttestation:!!token?.value?.length,attestationBytes:token?.value?.length||0};}
  return {page:rows[0]?.result,browser:UdmStreamingCapture.diagnostics(tab.id),sessionMetadata};
 },{url,height:targetHeight}),'Read safe capture diagnostics');
 const selectedAt=performance.now();
 const result=await bounded(worker.evaluate(async ({url,height})=>{
  const tab=(await chrome.tabs.query({url}))[0];const sender={id:chrome.runtime.id};const ctx=await availableMedia({url,tabId:tab.id},sender);
  const choice=ctx.choices.find(c=>c.height===height);if(!choice)return {error:height+'p unavailable'};
  const request={action:'media',url,tabId:tab.id,height:choice.height,formatKey:choice.key,title:'UDM Edge isolated handoff test'};
  return Promise.race([mediaHandoff(request,sender).then(v=>({ok:v.ok})).catch(e=>({error:e.message})),new Promise(r=>setTimeout(()=>r({error:'Handoff exceeded 30 seconds'}),30000))]);
 },{url,height:targetHeight}),'Capture and handoff',40000);
 report.handoffMs=Math.round(performance.now()-selectedAt);report.result=result;report.stages=await worker.evaluate(()=>handoffTrace);
 const state=JSON.parse(fs.readFileSync(path.join(out,'state/state.json'),'utf8').replace(/^\uFEFF/,''));report.jobs=state.Downloads.map(j=>({status:j.Status,height:j.MediaHeight,received:j.Received,error:j.Error}));
 assert.equal(result.ok,true,JSON.stringify(result));assert.equal(report.jobs.length,1);assert.equal(report.jobs[0].height,targetHeight);console.log(`PASS real YouTube ${targetHeight}p handoff in fresh ${channel} profile`);
 if(download){
  phase('Download captured video and audio');
  const job=await until(()=>{const value=JSON.parse(fs.readFileSync(path.join(out,'state/state.json'),'utf8').replace(/^\uFEFF/,''));const j=value.Downloads[0];if(j?.Status==='Failed')throw Error(j.Error);return j?.Status==='Complete'&&j;},120000);
  await verifyComplete(job);
 }
})().catch(e=>{report.error=e.message;report.failurePhase=report.phase;process.exitCode=1;console.error(e.message);}).finally(async()=>{
 try{const state=JSON.parse(fs.readFileSync(path.join(out,'state/state.json'),'utf8').replace(/^\uFEFF/,''));report.jobsAtHandoff=report.jobs;report.jobs=state.Downloads.map(j=>({status:j.Status,height:j.MediaHeight,received:j.Received,error:j.Error}));}catch{}
 try{if(worker)report.stages=await bounded(worker.evaluate(()=>globalThis.handoffTrace||[]),'Collect phase log',5000);}catch{}
 try{if(page)await page.screenshot({path:path.join(out,'page.png')});}catch{}
 try{if(context)await bounded(context.close(),'Close test browser',10000);}catch(e){console.error(e.message);}finally{desktop?.kill();}
 if(ownedOutput&&recover)for(const name of fs.readdirSync(out))if(/^refresh-page-\d+\.txt$/.test(name))fs.rmSync(path.join(out,name),{force:true});
 if(registered){
  try{
   execFileSync('reg.exe',['delete',registry,'/f'],{windowsHide:true,stdio:'ignore'});
   report.fixtureRegistrationRemoved=true;
  }catch(e){report.cleanupError=e.message;process.exitCode=1;}
 }
 if(ownedOutput)fs.writeFileSync(path.join(out,'result.json'),JSON.stringify(report,null,2));
 console.log(JSON.stringify(report,null,2));
});
