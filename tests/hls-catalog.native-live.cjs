'use strict';
const {chromium}=require('playwright'),fs=require('node:fs'),path=require('node:path'),http=require('node:http'),assert=require('node:assert/strict'),crypto=require('node:crypto'),{spawn,execFileSync}=require('node:child_process');
const {createTrace,headers:traceHeaders}=require('./loopback-http-diagnostics.cjs');
const {summarizeNetlog}=require('./loopback-netlog.cjs');
const project=path.resolve(__dirname,'..'),root=path.resolve(process.argv[2]),media=path.resolve(process.env.UDM_TEST_MEDIA_ROOT||path.join(root,'media')),data=path.join(root,'state'),state=path.join(data,'state.json'),exe=process.env.UDM_APP_EXE||path.join(project,'release-native/UDM.exe'),ff=process.env.UDM_FFMPEG||'D:\\UDM\\release\\tools\\ffmpeg.exe',fp=path.join(path.dirname(ff),'ffprobe.exe'),tag='audio'+crypto.randomBytes(8).toString('hex'),results=[],requests=[];
const reuseMedia=!!process.env.UDM_TEST_MEDIA_ROOT,mediaHashes=new Map();
function snapshotMedia(folder){for(const entry of fs.readdirSync(folder,{withFileTypes:true})){const file=path.join(folder,entry.name);if(entry.isDirectory())snapshotMedia(file);else if(entry.isFile())mediaHashes.set(file,crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex'));else throw Error('Unexpected media fixture entry');}}
// Reject re-use before the async finally block can overwrite previous results.
if(fs.existsSync(state)||fs.existsSync(path.join(root,'results.json')))throw Error('Use a new test folder');
const fileInfoDriver=process.env.UDM_FILE_INFO_TEST_EXE;
const coldDesktop=process.env.UDM_TEST_COLD_DESKTOP==='1';
const warmProduction=process.env.UDM_TEST_WARM_PRODUCTION==='1';
const retryAdmissionOnce=process.env.UDM_TEST_RETRY_ADMISSION_ONCE==='1';
const retryDecisionWriteOnce=process.env.UDM_TEST_RETRY_START_WRITE_ONCE==='1';
const previewOnly=process.env.UDM_TEST_PREVIEW_ONLY==='1',httpDiagnostics=process.env.UDM_TEST_HTTP_DIAGNOSTICS==='1';
const previewMalformedResponse=process.env.UDM_TEST_PREVIEW_MALFORMED==='1';
for(const name of ['UDM_TEST_PREVIEW_ONLY','UDM_TEST_HTTP_DIAGNOSTICS','UDM_TEST_PREVIEW_MALFORMED'])if(process.env[name]!==undefined&&process.env[name]!=='1')throw Error('Unset '+name+' or set it to 1');
if(previewMalformedResponse&&(!previewOnly||!httpDiagnostics))throw Error('Malformed preview requires preview-only HTTP diagnostics');
if(previewOnly&&(!coldDesktop||!warmProduction||retryAdmissionOnce||retryDecisionWriteOnce))throw Error('Preview-only requires a ready isolated production desktop without download fault injection');
if(retryDecisionWriteOnce&&!coldDesktop)throw Error('Decision write fault requires an isolated production desktop');
if(retryAdmissionOnce&&(!coldDesktop||!warmProduction))throw Error('Admission retry fault requires a ready isolated production desktop');
const externalActionable=process.env.UDM_FILE_INFO_EXTERNAL_OBSERVE==='1';
const privateAppSource=path.resolve(process.env.UDM_TEST_PRIVATE_APP_SOURCE||exe);
if(process.env.UDM_TEST_PRIVATE_APP_SOURCE&&!coldDesktop)throw Error('Private app override requires private production staging');
if(warmProduction&&!coldDesktop)throw Error('Warm production observation requires private production staging');
if(externalActionable&&!fileInfoDriver)throw Error('External component observation requires its owned fixture');
if(coldDesktop&&fileInfoDriver)throw Error('Choose either the presentation fixture or the production cold desktop.');
const fileInfoEnabled=!!fileInfoDriver||coldDesktop;
const startWhileReceiving=process.env.UDM_FILE_INFO_START_RECEIVING==='1';
if(process.env.UDM_FILE_INFO_START_RECEIVING!==undefined&&!startWhileReceiving)throw Error('Unset UDM_FILE_INFO_START_RECEIVING or set it to 1');
const segmentDelayMs=Number(process.env.UDM_TEST_SEGMENT_DELAY_MS||0);
if(startWhileReceiving&&!fileInfoDriver)throw Error('Receiving confirmation requires the native presentation fixture');
if(!Number.isInteger(segmentDelayMs)||segmentDelayMs<0||segmentDelayMs>10000)throw Error('Invalid fixture segment delay');
const coldNative=path.resolve(process.env.UDM_TEST_COLD_NATIVE_DIR||path.join(root,'native'));
const fileInfoFormat=process.env.UDM_FILE_INFO_FORMAT||'MP4';
let firstSegmentEpochMs=0,firstSegmentCatalog,httpTrace;
const browserEvents=[],serverEvents=[],pageErrors=[];
const privateNetlog=path.join(root,'private-netlog.json');let netlogSummaryWritten=false;
const segmentRequests=[];
const browserSource=path.join(process.env.UDM_TEST_PROJECT||project,'browser/chromium');
const expectedVersion=process.env.UDM_EXPECT_NATIVE_VERSION||fs.readFileSync(path.join(project,'native/Version.hpp'),'utf8').match(/UDM_VERSION_STRING "([^"]+)"/)[1];
const wait=ms=>new Promise(r=>setTimeout(r,ms));let browser,page,worker,desktop,observer,observerError='',server,base;const hostName='com.udm.hlscatalog'+Date.now(),registry='HKCU\\Software\\Microsoft\\Edge\\NativeMessagingHosts\\'+hostName;let registered=false,coldNativeCreated=false;
async function until(fn,timeout=30000){const end=Date.now()+timeout;while(Date.now()<end){const v=await fn();if(v)return v;await wait(100);}throw Error('Audio fixture timed out');}
function jobs(){try{return JSON.parse(fs.readFileSync(state)).Downloads;}catch{return [];}}
function pass(name,detail={}){results.push({name,passed:true,...detail});console.log('PASS '+name);}
const run=args=>execFileSync(ff,['-hide_banner','-loglevel','error','-nostdin','-y',...args],{windowsHide:true,maxBuffer:16e6});
async function clickAX(session,role,name){
 const node=await until(async()=>{const tree=await session.send('Accessibility.getFullAXTree');return tree.nodes.find(n=>!n.ignored&&n.role?.value===role&&name(n.name?.value||''));});
 const {model}=await session.send('DOM.getBoxModel',{backendNodeId:node.backendDOMNodeId});
 const q=model.content,clickStartedAt=Date.now();
 await page.mouse.click((q[0]+q[2])/2,(q[1]+q[5])/2);
 return clickStartedAt;
}
function frequency(file){const raw=run(['-i',file,'-map','0:a:0','-ss','2','-t','2','-ar','48000','-ac','1','-f','s16le','pipe:1']);let crossings=0;for(let i=2;i<raw.length;i+=2)if(raw.readInt16LE(i-2)<=0&&raw.readInt16LE(i)>0)crossings++;return crossings/(raw.length/2/48000);}
async function choose(kind,{language='es',container='MP4',back=false,audioOnly=false,subtitles=false,height=360}={}){
 if(previewOnly){
  await page.goto(base+'/'+kind);await page.waitForFunction(()=>document.querySelector('video').videoHeight===360&&document.body.dataset.ready==='yes');
  assert.equal(jobs().length,0,'Preview readiness is not a download admission');
  pass('Preview bootstrap reaches a decoded 360p frame without selecting or downloading a file');return;
 }
 const before=new Set(jobs().map(x=>x.Id)),requestStart=requests.length;await page.goto(base+'/'+kind);await page.waitForFunction(()=>document.querySelector('video').videoHeight===360&&document.body.dataset.ready==='yes');await until(()=>page.locator('[id^="udm-video-panel-"]').count());const session=await browser.newCDPSession(page);
 const began=performance.now();await clickAX(session,'button',n=>n==='Download this video with UDM');
 await until(async()=>{const ax=await session.send('Accessibility.getFullAXTree');return ax.nodes.some(n=>!n.ignored&&n.role?.value==='button'&&n.name?.value?.includes('MP4 · '+height+'p'));});
 const formatTree=await session.send('Accessibility.getFullAXTree');
 const optionsPrefix='Audio/subtitle options for ';
 const qualities=formatTree.nodes.filter(n=>!n.ignored&&n.role?.value==='button'&&!n.name?.value?.startsWith(optionsPrefix)&&/ · (?:MP4|TS) · \d+p/.test(n.name?.value||''));
 if(kind!=='flat'){
  const labels=qualities.map(n=>n.name.value);
  assert.equal(qualities.length,4,JSON.stringify(labels));
  assert.equal(qualities.filter(n=>n.name.value.includes('720p')).length,2);
  assert.equal(qualities.filter(n=>n.name.value.includes('360p')).length,2);
 }
 pass(kind+' panel exposes each actual quality/container once',{elapsedMs:Math.round(performance.now()-began),labels:qualities.map(n=>n.name.value)});
 const chooseOptions=n=>audioOnly?n==='Audio only (M4A)…':n.startsWith(optionsPrefix)&&n.includes(container+' · '+height+'p');
 await page.screenshot({path:path.join(root,kind+'-'+container+'-'+height+'-catalog.png')});await clickAX(session,'button',chooseOptions);
 let tree=await session.send('Accessibility.getFullAXTree'),select=tree.nodes.find(n=>!n.ignored&&n.role?.value==='combobox'&&n.name?.value==='Audio track');assert(select);assert.match(select.value?.value||'',/English.*default/);assert.equal(jobs().filter(x=>!before.has(x.Id)).length,0);pass(kind+' '+container+' presents declared default audio before creating a download');
 if(back){await clickAX(session,'button',n=>n==='Back');tree=await session.send('Accessibility.getFullAXTree');assert(tree.nodes.some(n=>!n.ignored&&n.name?.value?.includes(container+' · '+height+'p')));assert.equal(jobs().filter(x=>!before.has(x.Id)).length,0);pass('Back returns to video qualities without starting a download');await clickAX(session,'button',chooseOptions);}
 if(language==='es'){await clickAX(session,'combobox',n=>n==='Audio track');await page.keyboard.press('End');await page.keyboard.press('Enter');tree=await session.send('Accessibility.getFullAXTree');select=tree.nodes.find(n=>!n.ignored&&n.role?.value==='combobox'&&n.name?.value==='Audio track');assert.match(select.value?.value||'',/Español/);}
 if(subtitles){await clickAX(session,'combobox',n=>n==='Subtitles');await page.keyboard.press('End');await page.keyboard.press('Enter');}
 await page.screenshot({path:path.join(root,kind+'-'+container+'-'+language+'-selection.png')});
 if(coldDesktop&&!warmProduction){
  const before=await worker.evaluate(()=>nativeRequest({action:'preferences-if-running'}));
  assert.equal(before.ok,false,'The desktop must still be closed after opening the quality and audio menus');
 }
 const selectedAt=await clickAX(session,'button',n=>n===(audioOnly?'Download audio (M4A)':'Download video'));
 let job;try{job=await until(()=>{if(observer&&observer.exitCode!==null&&observer.exitCode!==0)throw Error(observerError||'Cold File Info observer failed');const j=jobs().find(x=>!before.has(x.Id));if(j?.Status==='Failed')throw Error(j.Error);return j?.Status==='Complete'&&j;},60000);}catch(error){
  const diagnostic=await worker.evaluate(async()=>({lastError:(await chrome.storage.local.get('lastError')).lastError,connection:nativeClient.diagnostics()})).catch(e=>({observerError:e.message}));
  fs.writeFileSync(path.join(root,'failed-selection.json'),JSON.stringify({kind,language,container,audioOnly,height,diagnostic,jobs:jobs().map(j=>({id:j.Id,status:j.Status,error:j.Error,fileName:j.FileName})),accessibility:await session.send('Accessibility.getFullAXTree')},null,2));
  await page.screenshot({path:path.join(root,'failed-selection.png')});throw error;
 }const file=path.join(job.Folder,job.FileName);
 run(['-xerror','-i',file,'-f','null','-']);
 if(fileInfoEnabled){
  const evidence=await until(()=>{try{return JSON.parse(fs.readFileSync(path.join(root,'file-info-result.json')));}catch{return false;}});
  if(fileInfoDriver)assert.equal(evidence.status,'Complete',evidence.error);
  if(startWhileReceiving){
   assert.equal(evidence.startWhileReceiving,true);
   assert.equal(evidence.sizeBeforeStart,-1);
   assert.equal(evidence.exactVisibleEpochMs,0);
   assert(evidence.actionableEpochMs<=evidence.startEpochMs);
   assert.equal(job.ContinuedMediaPrefetch,true);
   assert(job.PrefetchedBytesAtStart>0);
  }else{
   assert.equal(evidence.sizeBeforeStart,fs.statSync(file).size);
   assert.equal(evidence.sha256BeforeStart,crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex'));
   assert(evidence.actionableEpochMs<=evidence.exactVisibleEpochMs);
  }
  if(fileInfoDriver)assert(evidence.firstReceivedObservationEpochMs<=evidence.startEpochMs);
  if(fileInfoDriver&&process.env.UDM_FILE_INFO_TRACE){
   const expected=['pending-offer-ready','offer-restored','dialog-owner-ready','file-info-entry',
    'file-info-controls-start','file-info-controls-ready','owned-theme-ready','owned-show-returned',
    'owned-paint-returned','owned-foreground-returned','first-prefetch-pulse','first-prefetch-returned'];
   let last=-Infinity;
   for(const name of expected){
    const stamp=evidence.trace.find(row=>row.stage===name)?.msAfterAdmission;
    assert(Number.isFinite(stamp)&&stamp>=last,'Missing or out-of-order native stage: '+name);
    last=stamp;
   }
   assert(evidence.trace.some(row=>row.stage==='ui-dispatch'));
   assert(evidence.trace.some(row=>row.stage==='actionable-observed'));
  }
  const prefetch=evidence.trace?.find(row=>row.stage==='first-prefetch-pulse');
  const checkpointsBeforeFirstSegment=prefetch?evidence.trace.filter(row=>row.stage==='catalog-temporary-flushed'&&
   row.epochMs>=prefetch.epochMs&&row.epochMs<=firstSegmentEpochMs).length:null;
  assert(firstSegmentEpochMs>0&&firstSegmentEpochMs<=(evidence.firstReceivedObservationEpochMs??evidence.exactVisibleEpochMs));
  assert(segmentRequests.length>0&&segmentRequests.every(segment=>segment.finished>=segment.started));
  const lastSegmentFinish=Math.max(...segmentRequests.map(segment=>segment.finished));
  if(startWhileReceiving){
   assert(evidence.startEpochMs<lastSegmentFinish,'Start must precede completion of network prefetch');
   assert.equal(new Set(segmentRequests.map(segment=>segment.path)).size,segmentRequests.length,'Start replayed an HLS segment');
   const groups=[language,...(audioOnly?[]:[height===720?'v720':'v'])];
   const expectedSegments=groups.flatMap(group=>fs.readdirSync(path.join(media,group)).filter(name=>/^index\d+\.ts$/.test(name)).map(name=>'/'+group+'/'+name)).sort();
   assert.deepEqual(segmentRequests.map(segment=>segment.path).sort(),expectedSegments,'Early Start skipped a selected segment');
  }
  if(fileInfoDriver&&process.env.UDM_TRACE_STARTUP_CHECKPOINTS){
   assert(firstSegmentCatalog?.hasMediaFolder);
   if(process.env.UDM_EXPECT_REUSED_MEDIA_CHECKPOINT){
    assert.equal(checkpointsBeforeFirstSegment,0);
    assert.equal(firstSegmentCatalog.status,'Awaiting confirmation');
    assert.equal(firstSegmentCatalog.confirmationPending,false);
   }else{
    assert(checkpointsBeforeFirstSegment>=1);
    assert(firstSegmentCatalog.confirmationPending);
    assert(['Awaiting confirmation','Downloading'].includes(firstSegmentCatalog.status));
   }
  }
  pass(startWhileReceiving?'Start during receiving continues prefetch without duplicate HLS segment requests':'Cold HLS File Info opens before verification and publishes the exact output shown before Start',{
   ...evidence,productionCold:coldDesktop&&!warmProduction,productionExecutable:coldDesktop,productionReadyBeforeSelection:warmProduction,...(fileInfoDriver?{selectionToAdmissionMs:evidence.admissionEpochMs-selectedAt,
    selectionToFirstReceivedObservationMs:evidence.firstReceivedObservationEpochMs-selectedAt}:{}),
   selectionToActionableMs:evidence.actionableEpochMs-selectedAt,
   selectionToExactMs:evidence.exactVisibleEpochMs-selectedAt,
   selectionToFirstSegmentMs:firstSegmentEpochMs-selectedAt,
   lastSegmentToExactMs:evidence.exactVisibleEpochMs-lastSegmentFinish,
   segments:segmentRequests.map(({started,finished,...segment})=>({...segment,startedMs:started-selectedAt,finishedMs:finished-selectedAt})),
   checkpointsBeforeFirstSegment,firstSegmentCatalog
  });
 }
 const probe=JSON.parse(execFileSync(fp,['-v','error','-count_frames','-show_streams','-show_format','-of','json',file],{windowsHide:true})),audio=probe.streams.filter(x=>x.codec_type==='audio');assert.equal(audio.length,1);assert.equal(probe.streams.some(x=>x.codec_type==='video'&&x.height===height),!audioOnly);assert.equal(audio[0].tags.language,language==='es'?'spa':'eng');assert.equal(path.extname(file),audioOnly?'.m4a':container==='TS'?'.ts':'.mp4');assert.match(job.FormatDescription,new RegExp(language==='es'?'Español.*es':'English.*en'));
 assert(Math.abs(Number(probe.format.duration)-8)<0.25,'Incomplete HLS output timeline');
 if(!audioOnly)assert.equal(Number(probe.streams.find(x=>x.codec_type==='video').nb_read_frames),192,'HLS output lost video frames');
 const outputHash=crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
 assert.equal(outputHash,job.Sha256,'Published output differs from its native checksum');assert.equal(fs.statSync(file).size,job.Size,'Published size differs from its catalog');
 pass(kind+' '+container+' complete output matches its timeline, frame count, size and checksum',{duration:Number(probe.format.duration),videoFrames:audioOnly?0:Number(probe.streams.find(x=>x.codec_type==='video').nb_read_frames),bytes:job.Size,sha256:outputHash});
 if(startWhileReceiving&&!audioOnly){
  const frames=JSON.parse(execFileSync(fp,['-v','error','-count_frames','-select_streams','v:0','-show_entries','stream=nb_read_frames','-of','json',file],{windowsHide:true}));
  assert.equal(Number(frames.streams[0]?.nb_read_frames),192,'Early-confirmed video lost frames');
 }
 const tone=frequency(file);assert(Math.abs(tone-(language==='es'?880:440))<3,'Wrong audio tone: '+tone);pass(kind+' '+container+' output contains the selected audio and language tag',{language,frequencyHz:tone,bytes:fs.statSync(file).size,sha256:job.Sha256});
 if(subtitles){const track=probe.streams.find(s=>s.codec_type==='subtitle');assert(track);assert.equal(track.tags.language,'spa');const caption=run(['-i',file,'-map','0:s:0','-f','webvtt','pipe:1']).toString('utf8');assert(caption.includes('Subtítulos españoles'));assert.match(caption,/00:00\.500 --> 00:02\.000/);assert.equal(caption.split('Subtítulos españoles').length,2);pass(kind+' selected subtitles preserve Unicode, timing, language and unique cues');}
 const sent=requests.slice(requestStart),wrong=language==='es'?'en':'es';assert(!sent.some(x=>x.includes('/'+wrong+'/')||x==='/'+wrong+'.m4a'));assert(!sent.some(x=>x.includes('/wrong/')));pass(kind+' '+container+' never fetches unselected audio or the overridden parent group');
 if(audioOnly){assert(!sent.some(s=>/^\/(?:v|v720)\/index\d+\.ts$/.test(s)));assert.equal(sent.filter(s=>s==='/video.mp4').length,1);pass(kind+' audio-only creates M4A without downloading the video track');}
 await session.detach();
}
(async()=>{
 fs.mkdirSync(data,{recursive:true});
 if(reuseMedia){for(const name of ['video.mp4','en.m4a','es.m4a','master.m3u8','master.mpd','v/index.m3u8','v720/index.m3u8','en/index.m3u8','es/index.m3u8','sub/index.m3u8','captions.vtt'])assert(fs.statSync(path.join(media,name)).isFile(),'Missing reusable media '+name);snapshotMedia(media);}
 else{fs.mkdirSync(media,{recursive:true});
 run(['-f','lavfi','-i','testsrc2=size=640x360:rate=24','-t','8','-c:v','libx264','-preset','ultrafast','-pix_fmt','yuv420p','-g','48','-movflags','+faststart',path.join(media,'video.mp4')]);
 for(const [lang,tone] of [['en',440],['es',880]]){fs.mkdirSync(path.join(media,lang),{recursive:true});run(['-f','lavfi','-i',`sine=frequency=${tone}:sample_rate=48000`,'-t','8','-c:a','aac','-b:a','96k',path.join(media,lang+'.m4a')]);run(['-i',path.join(media,lang+'.m4a'),'-c','copy','-hls_time','2','-hls_playlist_type','vod',path.join(media,lang,'index.m3u8')]);}
 fs.mkdirSync(path.join(media,'v'),{recursive:true});run(['-i',path.join(media,'video.mp4'),'-c','copy','-hls_time','2','-hls_playlist_type','vod',path.join(media,'v/index.m3u8')]);
 fs.mkdirSync(path.join(media,'v720'),{recursive:true});
 run(['-f','lavfi','-i','testsrc2=size=1280x720:rate=24','-t','8','-c:v','libx264','-preset','ultrafast','-pix_fmt','yuv420p','-g','48','-f','hls','-hls_time','2','-hls_playlist_type','vod',path.join(media,'v720/index.m3u8')]);
 }
 const master='#EXTM3U\n#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID="a",NAME="English",LANGUAGE="en",DEFAULT=YES,AUTOSELECT=YES,URI="en/index.m3u8"\n#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID="a",NAME="Español",LANGUAGE="es",AUTOSELECT=YES,URI="es/index.m3u8"\n#EXT-X-MEDIA:TYPE=SUBTITLES,GROUP-ID="s",NAME="Español",LANGUAGE="es",URI="sub/index.m3u8"\n#EXT-X-STREAM-INF:BANDWIDTH=1000000,RESOLUTION=640x360,CODECS="avc1.42c01e,mp4a.40.2",AUDIO="a",SUBTITLES="s"\nv/index.m3u8\n';if(!reuseMedia)fs.writeFileSync(path.join(media,'master.m3u8'),master);
 const inner=master.replace(/URI="(en|es|sub)\//g,'URI="../$1/').replace('\nv/index.m3u8','\n../v/index.m3u8')+'#EXT-X-STREAM-INF:BANDWIDTH=3000000,RESOLUTION=1280x720,CODECS="avc1.42c01e,mp4a.40.2",AUDIO="a"\n../v720/index.m3u8\n';
 const outer='#EXTM3U\n#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID="a",NAME="Wrong parent",LANGUAGE="en",DEFAULT=YES,URI="wrong/index.m3u8"\n#EXT-X-STREAM-INF:BANDWIDTH=3000000,AUDIO="a"\nnested/master.m3u8\n#EXT-X-STREAM-INF:BANDWIDTH=3000000,AUDIO="a"\nnested/master.m3u8\n';
 const cycle=outer+'#EXT-X-STREAM-INF:BANDWIDTH=1000000\ncycle.m3u8\n';

 if(!reuseMedia){fs.writeFileSync(path.join(media,'master.mpd'),'<MPD type="static" mediaPresentationDuration="PT8S"><Period><AdaptationSet mimeType="video/mp4"><Representation id="v" height="360" bandwidth="1000000"><BaseURL>video.mp4</BaseURL></Representation></AdaptationSet><AdaptationSet mimeType="audio/mp4" lang="es"><Label>Español</Label><Representation id="a" bandwidth="128000"><BaseURL>es.m4a</BaseURL></Representation></AdaptationSet><AdaptationSet mimeType="audio/mp4" lang="en"><Label>English</Label><Role value="main"/><Representation id="a" bandwidth="64000"><BaseURL>en.m4a</BaseURL></Representation></AdaptationSet><AdaptationSet mimeType="text/vtt" lang="es"><Label>Español</Label><Representation id="s"><BaseURL>captions.vtt</BaseURL></Representation></AdaptationSet></Period></MPD>');

 fs.mkdirSync(path.join(media,'sub'),{recursive:true});
 const caption='00:00:00.500 --> 00:00:02.000\nSubtítulos españoles\n\n';
 fs.writeFileSync(path.join(media,'captions.vtt'),'WEBVTT\n\n'+caption);
 // The MPEGTS fixture starts at 1.4 seconds; duplicate cues straddle playlist boundaries.
 const clock=JSON.parse(execFileSync(fp,['-v','error','-show_entries','format=start_time','-of','json',path.join(media,'v/index0.ts')],{windowsHide:true})).format.start_time;
 for(let i=0;i<2;i++)fs.writeFileSync(path.join(media,'sub',i+'.vtt'),'WEBVTT\nX-TIMESTAMP-MAP=LOCAL:00:00:00.000,MPEGTS:'+Math.round(Number(clock)*90000)+'\n\n'+caption);
 fs.writeFileSync(path.join(media,'sub/index.m3u8'),'#EXTM3U\n#EXTINF:4,\n0.vtt\n#EXTINF:4,\n1.vtt\n#EXT-X-ENDLIST\n');
 }
 server=http.createServer((req,res)=>{const u=new URL(req.url,'http://localhost');requests.push(u.pathname);res.setHeader('Cache-Control','no-store');if(['/nested','/flat','/cycle'].includes(u.pathname)){res.setHeader('Content-Type','text/html; charset=utf-8');res.end(`<!doctype html><meta charset=utf-8><title>UDM alternate audio fixture</title><style>body{margin:60px;font:16px Arial;background:#e8eef4}video{width:640px}p{max-width:640px}</style><h1>Choose the audio track</h1><p>Synthetic 360p video: English is a 440 Hz tone; Español is an 880 Hz tone.</p><video controls autoplay muted loop></video><script>(async()=>{const v=document.querySelector('video');v.src=URL.createObjectURL(new Blob([await fetch('/video.mp4').then(r=>r.arrayBuffer())],{type:'video/mp4'}));await fetch('${u.pathname==='/flat'?'/master.m3u8':u.pathname==='/cycle'?'/cycle.m3u8':'/outer.m3u8'}').then(r=>r.text());if('${u.pathname}'!=='/flat')await fetch('/real/master.m3u8').then(r=>r.text());document.body.dataset.ready='yes';await v.play();})();</script>`);return;}
 if(u.pathname==='/nested/master.m3u8'){res.writeHead(302,{Location:'/real/master.m3u8'}).end();return;}
 if(['/outer.m3u8','/real/master.m3u8','/cycle.m3u8'].includes(u.pathname)){res.writeHead(200,{'Content-Type':'application/vnd.apple.mpegurl'}).end(u.pathname==='/real/master.m3u8'?inner:u.pathname==='/cycle.m3u8'?cycle:outer);return;}
 if(previewMalformedResponse&&u.pathname==='/video.mp4'){req.socket.end('not an HTTP response\r\n');return;}
 const name=u.pathname.slice(1);if(!/^(?:captions\.vtt|sub\/(?:index\.m3u8|[01]\.vtt)|video\.mp4|(?:en|es)\.m4a|master\.(?:m3u8|mpd)|(?:v|v720|en|es)\/(?:index\.m3u8|index\d+\.ts))$/.test(name)){res.writeHead(404).end();return;}const file=path.join(media,name);if(!fs.existsSync(file)){res.writeHead(404).end();return;}const buffer=fs.readFileSync(file),range=/^bytes=(\d+)-(\d*)$/.exec(req.headers.range||''),a=range?+range[1]:0,b=range&&range[2]?Math.min(+range[2],buffer.length-1):buffer.length-1;if(a>b){res.writeHead(416).end();return;}res.writeHead(range?206:200,{'Content-Length':b-a+1,'Content-Type':name.endsWith('vtt')?'text/vtt':name.endsWith('m3u8')?'application/vnd.apple.mpegurl':name.endsWith('mpd')?'application/dash+xml':name.endsWith('m4a')?'audio/mp4':name.endsWith('ts')?'video/mp2t':'video/mp4',ETag:'"audio-fixture"',...(range?{'Content-Range':`bytes ${a}-${b}/${buffer.length}`}:{})});if(segmentDelayMs&&/\/(?:v|v720|en|es)\/index1\.ts$/.test(u.pathname))setTimeout(()=>{if(!res.destroyed)res.end(buffer.subarray(a,b+1));},segmentDelayMs);else res.end(buffer.subarray(a,b+1));
 });
 server.prependListener('request',(request,response)=>{
  const diagnostic={path:request.url,method:request.method,status:response.statusCode,finished:false};serverEvents.push(diagnostic);
  response.once('finish',()=>{diagnostic.status=response.statusCode;diagnostic.finished=true;if(httpDiagnostics)diagnostic.headers=traceHeaders(response.getHeaders());});
  response.once('close',()=>{diagnostic.status=response.statusCode;diagnostic.closed=true;diagnostic.finished=response.writableFinished;});
 });
 if(fileInfoEnabled)server.prependListener('request',(request,response)=>{
  const pathname=new URL(request.url,'http://localhost').pathname;
  if(!/^\/(?:v|v720|en|es)\/index\d+\.ts$/.test(pathname))return;
  const segment={path:pathname,started:Date.now(),finished:0};
  segmentRequests.push(segment);
  response.once('finish',()=>{segment.finished=Date.now();});
  if(firstSegmentEpochMs)return;
  firstSegmentEpochMs=segment.started;
  if(process.env.UDM_TRACE_STARTUP_CHECKPOINTS){
   const record=jobs()[0];
   firstSegmentCatalog={status:record?.Status,confirmationPending:record?.ConfirmationPending===true,
    hasMediaFolder:typeof record?.MediaFolder==='string'&&path.isAbsolute(record.MediaFolder)};
  }
 });
 await new Promise(r=>server.listen(0,'127.0.0.1',r));base='http://127.0.0.1:'+server.address().port;
 fs.writeFileSync(state,JSON.stringify({Schema:1,Settings:{DownloadFolder:path.join(root,'downloads'),CategoryFolders:false,Connections:4,Parallel:1,ProxyMode:'Connect directly',SkipBrowserFileInfo:!fileInfoEnabled,PrefetchFileInfo:fileInfoEnabled,SuppressProgressDialog:true,SuppressCompletionDialog:true,Sound:false,CloseToTray:false,ClipboardMonitor:false,DropBasket:false,DuplicatePolicy:'Numbered'},Queues:[{Name:'Main queue',Enabled:true,Parallel:1}],Projects:[],Downloads:[]}));
 if(fileInfoEnabled)fs.writeFileSync(path.join(root,'.udm-live-file-info-fixture'),'isolated HLS File Info');
 if(coldDesktop){
  fs.mkdirSync(coldNative);
  coldNativeCreated=true;
  for(const name of ['UDM.exe','Udm.NativeHost.exe'])fs.linkSync(name==='UDM.exe'?privateAppSource:path.join(path.dirname(exe),name),path.join(coldNative,name));
  fs.cpSync(path.join(path.dirname(exe),'assets'),path.join(coldNative,'assets'),{recursive:true});
  // Installed media tools can be on another volume; reuse their directory.
  fs.symlinkSync(path.resolve(path.dirname(ff)),path.join(coldNative,'tools'),'junction');
  fs.writeFileSync(path.join(coldNative,'udm-data.json'),JSON.stringify({dataDirectory:data}));
 }else{
  desktop=spawn(fileInfoDriver||exe,fileInfoDriver?['--browser-cold-file-info',root]:['--background','--data-dir',data,'--instance-tag',tag],
   {windowsHide:true,stdio:'ignore',env:{...process.env,UDM_INSTANCE_TAG:tag}});
 }
 fs.cpSync(browserSource,path.join(root,'extension'),{recursive:true});
 const manifest=JSON.parse(fs.readFileSync(path.join(root,'extension/manifest.json'))),id=crypto.createHash('sha256').update(Buffer.from(manifest.key,'base64')).digest('hex').slice(0,32).replace(/[0-9a-f]/g,c=>String.fromCharCode(97+parseInt(c,16)));
 const hostFile=path.join(root,'native-host.json');fs.writeFileSync(hostFile,JSON.stringify({name:hostName,description:'Isolated HLS catalog fixture',path:path.join(coldDesktop?coldNative:path.dirname(exe),'Udm.NativeHost.exe'),type:'stdio',allowed_origins:['chrome-extension://'+id+'/']}));
 const background=path.join(root,'extension/background.js');fs.writeFileSync(background,fs.readFileSync(background,'utf8').replace("'com.udm.download_manager'",JSON.stringify(hostName)));
 let exists=false;try{execFileSync('reg.exe',['query',registry],{windowsHide:true,stdio:'ignore'});exists=true;}catch{}assert(!exists);execFileSync('reg.exe',['add',registry,'/ve','/t','REG_SZ','/d',hostFile,'/f'],{windowsHide:true,stdio:'ignore'});registered=true;
 browser=await chromium.launchPersistentContext(path.join(root,'profile'),{channel:process.env.UDM_TEST_BROWSER||'msedge',headless:true,ignoreDefaultArgs:['--disable-extensions'],env:{...process.env,UDM_INSTANCE_TAG:tag},viewport:{width:1100,height:850},args:['--enable-unsafe-extension-debugging','--autoplay-policy=no-user-gesture-required',...(httpDiagnostics?['--log-net-log='+privateNetlog,'--net-log-capture-mode=Everything']:[])]});const cdp=await browser.browser().newBrowserCDPSession();await cdp.send('Extensions.loadUnpacked',{path:path.join(root,'extension')});worker=browser.serviceWorkers()[0]||await browser.waitForEvent('serviceworker');
 const info=await until(async()=>{try{const result=await worker.evaluate(action=>nativeRequest({action}),coldDesktop&&!warmProduction?'hello':'diagnostics');return result?.ok&&result;}catch{return null;}});
 const diagnosticsReadyAt=Date.now();
 assert.equal(info.version,expectedVersion);
 assert.equal(await worker.evaluate(()=>chrome.runtime.getManifest().version),manifest.version);
 if(coldDesktop||externalActionable){
  let ready='';
  const observerExe=coldDesktop?path.join(coldNative,'UDM.exe'):fileInfoDriver;
  const observerArgs=externalActionable?['--actionable-only','--pid',String(desktop.pid)]:[...(retryAdmissionOnce?['--retry-admission-once']:[]),...(retryDecisionWriteOnce?['--retry-start-write-once']:[])];
  observer=spawn(process.env.UDM_TEST_PYTHON||'python',[path.join(__dirname,'observe-cold-file-info.py'),root,observerExe,...observerArgs],{windowsHide:true,stdio:['ignore','pipe','pipe']});
  observer.stdout.on('data',chunk=>{ready+=chunk;});observer.stderr.on('data',chunk=>{observerError+=chunk;});
  observer.on('error',error=>{observerError=error.message;});
  await until(()=>{if(observerError||observer.exitCode!==null)throw Error(observerError||'File Info observer exited');return ready.includes('ready');},5000);
 }
 if(!coldDesktop||warmProduction){
  assert.equal(path.resolve(info.dataDirectory).toLowerCase(),data.toLowerCase());
  assert.equal(info.downloads,0);
 }
 pass('Updated extension and native app connect only to the isolated audio fixture',{diagnosticsReadyEpochMs:diagnosticsReadyAt,observerReadyEpochMs:Date.now()});
 page=browser.pages()[0]||await browser.newPage();if(httpDiagnostics){httpTrace=createTrace(base);const network=await browser.newCDPSession(page);network.on('Network.requestWillBeSent',e=>httpTrace.request(e));network.on('Network.responseReceivedExtraInfo',e=>httpTrace.response(e));network.on('Network.loadingFailed',e=>httpTrace.failed(e));network.on('Network.loadingFinished',e=>httpTrace.finished(e));await network.send('Network.enable');}const errors=pageErrors;page.on('pageerror',e=>errors.push(e.message));
 page.on('requestfailed',r=>browserEvents.push({event:'failed',url:r.url(),error:r.failure()?.errorText}));
 page.on('response',r=>{if(r.url().startsWith(base+'/')){const diagnostic={event:'response',url:r.url(),status:r.status(),contentLength:r.headers()['content-length']};browserEvents.push(diagnostic);void r.finished().then(error=>{diagnostic.finished=true;if(error)diagnostic.error=String(error);}).catch(error=>{diagnostic.error=error.message;});}});
 if(fileInfoEnabled){
  assert(['MP4','TS','M4A'].includes(fileInfoFormat),'Expected MP4, TS or M4A');
  await choose('nested',fileInfoFormat==='MP4'?{height:720}:fileInfoFormat==='TS'?{language:'en',container:'TS'}:{audioOnly:true});
 }else{
  await choose('nested',{height:720});await choose('nested',{audioOnly:true});await choose('nested',{language:'en',container:'TS'});await choose('flat',{back:true,subtitles:true});await choose('cycle',{height:720});
 }
 assert.deepEqual(errors,[]);pass('Audio selection pages have no script errors');
})().catch(e=>{console.error(e);results.push({passed:false,error:e.stack});process.exitCode=1;}).finally(async()=>{
 if(page&&!page.isClosed()){let timer;try{const snapshot=await Promise.race([page.evaluate(()=>{const v=document.querySelector('video');return {url:location.href,ready:document.body?.dataset.ready,video:v?{height:v.videoHeight,width:v.videoWidth,readyState:v.readyState,networkState:v.networkState,duration:v.duration,currentSrc:v.currentSrc,error:v.error?{code:v.error.code,message:v.error.message}:null}:null};}),new Promise((_,reject)=>{timer=setTimeout(()=>reject(Error('Snapshot deadline')),3000);})]);fs.writeFileSync(path.join(root,'page-snapshot.json'),JSON.stringify(snapshot,null,2));}catch(error){fs.writeFileSync(path.join(root,'page-snapshot.json'),JSON.stringify({error:error.message}));}finally{clearTimeout(timer);}}
 if(browser)await browser.close();
 if(httpDiagnostics&&browser){try{const summary=summarizeNetlog(JSON.parse(fs.readFileSync(privateNetlog,'utf8')),base);fs.writeFileSync(path.join(root,'netlog-summary.json'),JSON.stringify(summary,null,2));netlogSummaryWritten=true;}catch(error){results.push({passed:false,error:'NetLog summary: '+error.message});process.exitCode=1;}}
 if(desktop)desktop.kill();if(observer&&observer.exitCode===null)observer.kill();if(server){server.closeAllConnections();await new Promise(r=>server.close(r));}if(registered)execFileSync('reg.exe',['delete',registry,'/f'],{windowsHide:true,stdio:'ignore'});let registryRemoved=false;try{execFileSync('reg.exe',['query',registry],{windowsHide:true,stdio:'ignore'});}catch{registryRemoved=true;}
 if(coldNativeCreated&&fs.existsSync(path.join(coldNative,'UDM.exe'))){
  const pids=execFileSync('powershell.exe',['-NoProfile','-NonInteractive','-Command','Get-Process | Where-Object {$_.ProcessName -eq "UDM" -and $_.Path -eq $env:UDM_TEST_OWNED_EXE} | Select-Object -ExpandProperty Id'],{env:{...process.env,UDM_TEST_OWNED_EXE:path.join(coldNative,'UDM.exe')},windowsHide:true,encoding:'utf8'}).trim();
  for(const pid of pids.split(/\s+/).filter(Boolean)){try{process.kill(Number(pid));}catch(error){if(error.code!=='ESRCH')throw error;}}
 }
 const reusedMediaUnchanged=!reuseMedia||[...mediaHashes].every(([file,hash])=>fs.existsSync(file)&&crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex')===hash);
 if(!reusedMediaUnchanged){results.push({passed:false,error:'Reusable media fixture changed during the run'});process.exitCode=1;}
 if(httpTrace)fs.writeFileSync(path.join(root,'http-diagnostics.json'),JSON.stringify(httpTrace.result(),null,2));fs.writeFileSync(path.join(root,'requests.json'),JSON.stringify(requests,null,2));fs.writeFileSync(path.join(root,'browser-events.json'),JSON.stringify(browserEvents,null,2));fs.writeFileSync(path.join(root,'server-events.json'),JSON.stringify(serverEvents,null,2));fs.writeFileSync(path.join(root,'page-errors.json'),JSON.stringify(pageErrors,null,2));fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({registryRemoved,reuseMedia,reusedMediaUnchanged,reusedMediaFiles:mediaHashes.size,previewOnly,httpDiagnostics,previewMalformedResponse,netlogSummaryWritten,scope:previewOnly?'Preview bootstrap only; no panel selection, transfer or completed-file validation':'Real isolated Edge and native app; nested/flat/cyclic HLS, actual panel clicks, full output decoding and selected audio/language verification',browser:process.env.UDM_TEST_BROWSER||'msedge',passed:results.filter(x=>x.passed).length,failed:results.filter(x=>!x.passed).length,results},null,2));});
