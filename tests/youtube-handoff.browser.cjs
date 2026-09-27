'use strict';
const {chromium}=require('playwright');
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict'),{spawn,execFileSync}=require('node:child_process');
const project=path.resolve(__dirname,'..'),out=process.argv[2],channel=process.argv[3]||'msedge',tag='ythandoff'+Date.now(),url='https://www.youtube.com/watch?v=BewnzhHlQuk';
const download=process.argv[4]==='download';
let context,desktop,worker,page;
const report={timestamp:new Date().toISOString(),scope:'Fresh '+channel+' profile and isolated native history',stages:[]};
const sleep=ms=>new Promise(r=>setTimeout(r,ms));
const phase=name=>{report.phase=name;console.log('PHASE '+name);};
async function bounded(promise,name,ms=25000){phase(name);let timer;try{return await Promise.race([promise,new Promise((_,reject)=>{timer=setTimeout(()=>reject(Error(name+' timed out')),ms);})]);}finally{clearTimeout(timer);}}
async function until(fn,ms=30000){const end=Date.now()+ms;while(Date.now()<end){const value=await fn();if(value)return value;await sleep(250);}throw Error('Test condition timed out');}
(async()=>{
 assert(out&&!fs.existsSync(out),'Use a fresh output directory');fs.mkdirSync(path.join(out,'state'),{recursive:true});
 fs.writeFileSync(path.join(out,'state/state.json'),JSON.stringify({Schema:1,Settings:{DownloadFolder:path.join(out,'downloads'),SkipBrowserFileInfo:download,PrefetchFileInfo:false,SuppressProgressDialog:true,SuppressCompletionDialog:true,Sound:false},Downloads:[],Queues:[{Name:'Main queue',Enabled:true,Parallel:1}],Projects:[]}));
 desktop=spawn(path.join(project,'release/UDM.exe'),['--background','--data-dir',path.join(out,'state'),'--instance-tag',tag],{windowsHide:true,stdio:'ignore'});
 context=await bounded(chromium.launchPersistentContext(path.join(out,'profile'),{channel,headless:true,ignoreDefaultArgs:['--disable-extensions'],viewport:{width:1100,height:800},env:{...process.env,UDM_INSTANCE_TAG:tag},args:['--enable-unsafe-extension-debugging','--autoplay-policy=no-user-gesture-required']}),'Launch isolated '+channel);
 const control=await context.browser().newBrowserCDPSession();await bounded(control.send('Extensions.loadUnpacked',{path:path.join(project,'browser/chromium')}),'Load UDM');
 worker=context.serviceWorkers().find(w=>w.url().includes('kahfappnpjdcboccpnhinkcobcdgbdpl'))||await context.waitForEvent('serviceworker',{predicate:w=>w.url().includes('kahfappnpjdcboccpnhinkcobcdgbdpl'),timeout:15000});
 report.version=await bounded(worker.evaluate(()=>chrome.runtime.getManifest().version),'Check worker version');
 const identity=await bounded(worker.evaluate(()=>chrome.runtime.sendNativeMessage('com.udm.download_manager',{action:'diagnostics'})),'Native diagnostics',15000);
 assert.equal(path.resolve(identity.dataDirectory).toLowerCase(),path.join(out,'state').toLowerCase());assert.equal(identity.downloads,0);report.nativeConnection=true;
 await worker.evaluate(()=>{globalThis.handoffTrace=[];for(const name of ['mediaContext','availableMedia','sabrFor','mediaHandoff']){const fn=globalThis[name];globalThis[name]=async(...args)=>{const start=Date.now();handoffTrace.push({name,event:'start'});try{const v=await fn(...args);handoffTrace.push({name,event:'end',milliseconds:Date.now()-start,available:name==='sabrFor'?!!v:undefined});return v;}catch(e){handoffTrace.push({name,event:'error',message:e.message});throw e;}}}});
 phase('Load public YouTube page');page=await context.newPage();await page.goto(url,{waitUntil:'domcontentloaded',timeout:45000});
 await until(async()=>{try{return await page.evaluate(()=>{const p=document.getElementById('movie_player');const skip=document.querySelector('.ytp-skip-ad-button,.ytp-ad-skip-button-modern,.ytp-ad-skip-button');if(skip)skip.click();return p?.getVideoData?.().video_id==='BewnzhHlQuk'&&!p.classList.contains('ad-showing')&&document.querySelector('video')?.readyState>=2;});}catch{return false;}},45000);
 if(process.argv[5]==='warm'){phase('Wait for ordinary browser playback to settle');await sleep(15000);}
 report.capture=await bounded(worker.evaluate(async url=>{
  const tab=(await chrome.tabs.query({url}))[0];
  const rows=await chrome.scripting.executeScript({target:{tabId:tab.id},world:'MAIN',func:()=>{const id=new URL(location.href).searchParams.get('v');const p=document.getElementById('movie_player');return {videoId:id,playerId:p?.getVideoData?.()?.video_id,sabr:globalThis.__udmCaptureV1?.diagnostics?.(),session:!!globalThis.__udmCaptureV1?.session?.(id),resources:performance.getEntriesByType('resource').filter(e=>{try{return new URL(e.name).hostname.endsWith('.googlevideo.com');}catch{return false;}}).slice(-8).map(e=>{const u=new URL(e.name);return {path:u.pathname,keys:[...u.searchParams.keys()]};})};}});
  const context=await availableMedia({url,tabId:tab.id},{id:chrome.runtime.id});const choice=context.choices.find(c=>c.height===1080);const session=choice?await sabrFor(context,choice):null;
  let sessionMetadata=null;if(session){const bytes=Uint8Array.from(atob(session.body),x=>x.charCodeAt(0));const fields=__udmUmpV1.fields(bytes);const sc=fields.find(f=>f.id===19&&f.wire===2);const token=sc?__udmUmpV1.fields(sc.value).find(f=>f.id===2&&f.wire===2):null;sessionMetadata={source:session.captureSource||'page',ageMilliseconds:Date.now()-session.capturedAt,hasBrowserAttestation:!!token?.value?.length,attestationBytes:token?.value?.length||0};}
  return {page:rows[0]?.result,browser:UdmStreamingCapture.diagnostics(tab.id),sessionMetadata};
 },url),'Read safe capture diagnostics');
 const result=await bounded(worker.evaluate(async url=>{
  const tab=(await chrome.tabs.query({url}))[0];const sender={id:chrome.runtime.id};const ctx=await availableMedia({url,tabId:tab.id},sender);
  const choice=ctx.choices.find(c=>c.height===1080);if(!choice)return {error:'1080p unavailable'};
  const request={action:'media',url,tabId:tab.id,height:choice.height,formatKey:choice.key,title:'UDM Edge isolated handoff test'};
  return Promise.race([mediaHandoff(request,sender).then(v=>({ok:v.ok})).catch(e=>({error:e.message})),new Promise(r=>setTimeout(()=>r({error:'Handoff exceeded 30 seconds'}),30000))]);
 },url),'Capture and handoff',40000);
 report.result=result;report.stages=await worker.evaluate(()=>handoffTrace);
 const state=JSON.parse(fs.readFileSync(path.join(out,'state/state.json'),'utf8').replace(/^\uFEFF/,''));report.jobs=state.Downloads.map(j=>({status:j.Status,height:j.MediaHeight,received:j.Received,error:j.Error}));
 assert.equal(result.ok,true,JSON.stringify(result));assert.equal(report.jobs.length,1);assert.equal(report.jobs[0].height,1080);console.log('PASS real YouTube 1080p handoff in fresh '+channel+' profile');
 if(download){
  phase('Download captured video and audio');
  const job=await until(()=>{const value=JSON.parse(fs.readFileSync(path.join(out,'state/state.json'),'utf8').replace(/^\uFEFF/,''));const j=value.Downloads[0];if(j?.Status==='Failed')throw Error(j.Error);return j?.Status==='Complete'&&j;},120000);
  const target=job.FilePath||path.join(job.Folder,job.FileName);
  const probe=JSON.parse(execFileSync(path.join(project,'release/tools/ffprobe.exe'),['-v','error','-show_streams','-show_format','-of','json',target],{windowsHide:true}));
  report.download={bytes:fs.statSync(target).size,duration:Number(probe.format.duration),streams:probe.streams.map(s=>({type:s.codec_type,codec:s.codec_name,width:s.width,height:s.height}))};
  assert(probe.streams.some(s=>s.codec_type==='video'&&s.height===job.MediaPixelHeight));assert(probe.streams.some(s=>s.codec_type==='audio'));assert(Number(probe.format.duration)>60);console.log('PASS actual captured video completes with verified video and audio tracks');
 }
})().catch(e=>{report.error=e.message;process.exitCode=1;console.error(e.message);}).finally(async()=>{
 try{const state=JSON.parse(fs.readFileSync(path.join(out,'state/state.json'),'utf8').replace(/^\uFEFF/,''));report.jobsAtHandoff=report.jobs;report.jobs=state.Downloads.map(j=>({status:j.Status,height:j.MediaHeight,received:j.Received,error:j.Error}));}catch{}
 try{if(worker)report.stages=await bounded(worker.evaluate(()=>globalThis.handoffTrace||[]),'Collect phase log',5000);}catch{}
 try{if(page)await page.screenshot({path:path.join(out,'page.png')});}catch{}
 if(out){fs.mkdirSync(out,{recursive:true});fs.writeFileSync(path.join(out,'result.json'),JSON.stringify(report,null,2));}console.log(JSON.stringify(report,null,2));
 try{await bounded(context?.close(),'Close test browser',10000);}catch(e){console.error(e.message);}finally{desktop?.kill();}
});
