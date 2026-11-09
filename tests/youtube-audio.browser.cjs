'use strict';
// Production extension/UI and native downloader, with a deliberate test-only
// URL substitution at the native boundary. No real YouTube CDN claim is made.
const {chromium}=require('playwright'),fs=require('node:fs'),path=require('node:path'),http=require('node:http'),assert=require('node:assert/strict'),crypto=require('node:crypto'),{spawn,execFileSync}=require('node:child_process');
const project=process.env.UDM_TEST_PROJECT||path.resolve(__dirname,'..'),root=path.resolve(process.argv[2]),channel=process.env.UDM_TEST_BROWSER||'msedge',tag='yta'+crypto.randomBytes(8).toString('hex'),id='Q3TI27IN7X0',url='https://www.youtube.com/watch?v='+id;
const ff=process.env.UDM_FFMPEG,fp=path.join(path.dirname(ff),'ffprobe.exe'),state=path.join(root,'state/state.json'),results=[],requests=[];
let browser,page,worker,desktop,server,base,nativeBridge,active=0,peak=0;
const wait=ms=>new Promise(r=>setTimeout(r,ms));
const run=args=>execFileSync(ff,['-hide_banner','-loglevel','error','-nostdin','-y',...args],{windowsHide:true,maxBuffer:16e6});
const hash=file=>crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
function jobs(){try{return JSON.parse(fs.readFileSync(state)).Downloads;}catch{return [];}}
function pass(name,detail={}){results.push({name,passed:true,...detail});console.log('PASS '+name);}
async function until(fn,timeout=30000){const end=Date.now()+timeout;while(Date.now()<end){const r=await fn();if(r)return r;await wait(100);}throw Error('Timed out waiting for fixture state');}
async function click(session,role,name){
 const box=await until(async()=>{
  const {nodes}=await session.send('Accessibility.getFullAXTree');
  const node=nodes.find(n=>!n.ignored&&n.role?.value===role&&n.backendDOMNodeId&&name(n.name?.value||''));
  if(!node)return null;
  try{return (await session.send('DOM.getBoxModel',{backendNodeId:node.backendDOMNodeId})).model.content;}
  catch(error){if(/Could not compute box model|Could not find node/.test(error.message))return null;throw error;}
 });
 await page.mouse.click((box[0]+box[2])/2,(box[1]+box[5])/2);
}
async function tree(session){return (await session.send('Accessibility.getFullAXTree')).nodes.filter(n=>!n.ignored);}
async function activate(session,name){
 const control=await until(async()=>(await tree(session)).find(n=>n.role?.value==='button'&&n.name?.value===name&&n.backendDOMNodeId));
 await session.send('DOM.focus',{backendNodeId:control.backendDOMNodeId});
 await page.keyboard.press('Enter');
}
async function choose(language){
 const before=jobs().length,requestStart=requests.length;peak=0;await page.goto(url,{waitUntil:'domcontentloaded'});await until(()=>page.locator('[id^="udm-video-panel-"]').count());const session=await browser.newCDPSession(page);
 await click(session,'button',n=>n==='Download this video with UDM');await click(session,'button',n=>n==='Audio only (M4A)…');
 const track=n=>n.role?.value==='combobox'&&n.name?.value==='Audio track';
 const nodes=await tree(session),selector=nodes.find(track);
 assert(!nodes.some(n=>n.role?.value==='button'&&n.name?.value==='Download video'));assert.match(selector?.value?.value||'',/English.*256 kbps/);assert.equal(jobs().length,before);pass('Audio-only selector chooses the highest-rate default and has no video-download action');
 if(language==='es'){
  await session.send('DOM.focus',{backendNodeId:selector.backendDOMNodeId});
  await page.keyboard.press('End');
  await until(async()=>/Español/.test((await tree(session)).find(track)?.value?.value||''));
 }
 await activate(session,'Download audio (M4A)');
 const job=await until(()=>{const j=jobs().slice(before)[0];if(j?.Status==='Failed')throw Error(j.Error);return j?.Status==='Complete'&&j;},60000),file=path.join(job.Folder,job.FileName),expected=path.join(root,language+'.m4a');
 assert.equal(jobs().length,before+1);assert.equal(path.extname(file),'.m4a');assert.equal(hash(file),hash(expected));assert.match(job.FileName,new RegExp(language==='es'?'Español':'English'));assert.equal(job.DownloadPage,url);
 const probe=JSON.parse(execFileSync(fp,['-v','error','-show_streams','-show_format','-of','json',file],{windowsHide:true}));assert.equal(probe.streams.length,1);assert.equal(probe.streams[0].codec_type,'audio');assert.equal(probe.streams[0].codec_name,'aac');assert(Math.abs(Number(probe.format.duration)-180)<0.1);run(['-i',file,'-f','null','-']);
 pass(language+' M4A is byte-identical to the selected source, decodes fully, and contains no video',{sha256:hash(file),bytes:fs.statSync(file).size,duration:Number(probe.format.duration)});
 const made=requests.slice(requestStart);assert(made.some(r=>r.bytes>0));assert(made.every(r=>r.path==='/'+language+'.m4a'));
 pass(language+' native download requests only the selected track',{peakConcurrentRequests:peak,requests:made.length});
 const handoffs=await worker.evaluate(()=>globalThis.audioFixtureHandoffs);assert.equal(handoffs.length,before+1);const h=handoffs.at(-1);assert.equal(h.action,'add');assert.equal(new URL(h.url).searchParams.get('xtags'),language);assert.equal(new URL(h.url).searchParams.get('sig'),'renewed');assert.equal(h.reportedDirectSize,fs.statSync(expected).size);assert.equal(h.videoUrl,undefined);assert.equal(h.referrer,url);pass('Production panel submits the renewed URL and source byte count before the test-only loopback substitution');
 await session.detach();
}
(async()=>{
 assert(ff&&process.env.UDM_APP_EXE&&process.env.UDM_HOST_EXE);assert(!fs.existsSync(state),'Use a fresh test directory');fs.mkdirSync(path.dirname(state),{recursive:true});
 for(const [lang,tone] of [['en',440],['es',880]])run(['-f','lavfi','-i','sine=frequency='+tone+':sample_rate=48000','-t','180','-c:a','aac','-b:a','256k',path.join(root,lang+'.m4a')]);
 server=http.createServer((req,res)=>{if(!['/en.m4a','/es.m4a'].includes(req.url)){res.writeHead(404).end();return;}const buf=fs.readFileSync(path.join(root,req.url.slice(1))),m=/^bytes=(\d+)-(\d*)$/.exec(req.headers.range||''),a=m?+m[1]:0,b=m&&m[2]?Math.min(+m[2],buf.length-1):buf.length-1;if(a>b){res.writeHead(416).end();return;}const bytes=req.method==='HEAD'?0:b-a+1;requests.push({path:req.url,range:req.headers.range||'',method:req.method,bytes});res.writeHead(m?206:200,{'Content-Type':'audio/mp4','Accept-Ranges':'bytes','Content-Length':b-a+1,ETag:'"'+req.url+'"',...(m?{'Content-Range':'bytes '+a+'-'+b+'/'+buf.length}:{})});if(bytes>1){active++;peak=Math.max(peak,active);res.on('close',()=>active--);setTimeout(()=>res.end(buf.subarray(a,b+1)),180);}else res.end(req.method==='HEAD'?undefined:buf.subarray(a,b+1));});await new Promise(r=>server.listen(0,'127.0.0.1',r));base='http://127.0.0.1:'+server.address().port;
 fs.writeFileSync(state,JSON.stringify({Schema:1,Settings:{DownloadFolder:path.join(root,'downloads'),CategoryFolders:false,Connections:4,Parallel:1,ProxyMode:'Connect directly',SkipBrowserFileInfo:true,PrefetchFileInfo:false,SuppressProgressDialog:true,SuppressCompletionDialog:true,Sound:false},Queues:[{Name:'Main queue',Enabled:true,Parallel:1}],Projects:[],Downloads:[]}));
 desktop=spawn(process.env.UDM_APP_EXE,['--background','--data-dir',path.dirname(state),'--instance-tag',tag],{windowsHide:true,stdio:'ignore'});
 browser=await chromium.launchPersistentContext(path.join(root,'profile'),{channel,headless:true,ignoreDefaultArgs:['--disable-extensions'],env:{...process.env,UDM_INSTANCE_TAG:tag},viewport:{width:1100,height:850},args:['--enable-unsafe-extension-debugging']});
 const extension=path.join(root,'extension');
 fs.cpSync(path.join(project,'browser/chromium'),extension,{recursive:true});
 const background=path.join(extension,'background.js');
 const source=fs.readFileSync(background,'utf8'),receiptTransport='UdmMediaReceipts.create(api,nativeRequest)';
 assert(source.includes("'com.udm.download_manager'")&&source.includes(receiptTransport));
 // Keep receipt dispatch on the same isolated transport as direct dispatch.
 fs.writeFileSync(background,source.replace("'com.udm.download_manager'",JSON.stringify('com.udm.'+tag))
  .replace(receiptTransport,'UdmMediaReceipts.create(api,(...args)=>nativeRequest(...args))'));
 const cdp=await browser.browser().newBrowserCDPSession();await cdp.send('Extensions.loadUnpacked',{path:extension});worker=browser.serviceWorkers()[0]||await browser.waitForEvent('serviceworker');
 nativeBridge=await require('./isolated-native-bridge.cjs')(process.env.UDM_HOST_EXE,tag);await worker.evaluate(url=>{nativeRequest=async (message,beforeSend,validate)=>{await validate?.(message);beforeSend?.();return (await fetch(url,{method:'POST',body:JSON.stringify(message)})).json();};},nativeBridge.url);
 const info=await until(async()=>{try{return await worker.evaluate(()=>nativeRequest({action:'diagnostics'}));}catch{return null;}});assert.equal(info.version,process.env.UDM_EXPECT_NATIVE_VERSION||'0.52.0');assert.equal(path.resolve(info.dataDirectory).toLowerCase(),path.dirname(state).toLowerCase());assert.equal(info.downloads,0);assert.equal(await worker.evaluate(()=>chrome.runtime.getManifest().version),JSON.parse(fs.readFileSync(path.join(project,'browser/chromium/manifest.json'))).version);pass('Staged extension connects only to the isolated native catalog');
 await worker.evaluate(base=>{
  const original=nativeRequest,read=availableMedia;
  globalThis.audioFixtureHandoffs=[];
  availableMedia=async (message,sender,...options)=>{
   const snapshot=await read(message,sender,...options);
   if(message.action==='youtube-audio'&&options[1]===true){
    await chrome.scripting.executeScript({target:{tabId:snapshot.tabId,frameIds:[snapshot.frameId]},world:'MAIN',func:()=>{
     for(const format of globalThis.ytInitialPlayerResponse.streamingData.adaptiveFormats){
      const url=new URL(format.url);url.searchParams.set('sig','renewed');format.url=url.href;
     }
    }});
   }
   return snapshot;
  };
  nativeRequest=async (message,beforeSend,validate)=>{
   if(['youtube-player','media','sabr'].includes(message.action))throw Error('Audio-only attempted video retrieval');
   if(message.action!=='add')return original(message,beforeSend,validate);
   await validate?.(message);
   const u=new URL(message.url),language=u.searchParams.get('xtags');
   if(u.hostname!=='rr1.googlevideo.com'||u.pathname!=='/videoplayback'||!['en','es'].includes(language))throw Error('Unexpected production handoff');
   audioFixtureHandoffs.push(message);
   const local={...message,url:base+'/'+language+'.m4a'};
   // The loopback origin is an ordinary direct download in the native app.
   delete local.mediaToken;
   delete local.youtubeDirectAudio;
   return original(local,beforeSend);
  };
 },base);
 const audio=(lang,itag,rate)=>({itag,mimeType:'audio/mp4; codecs="mp4a.40.2"',bitrate:rate,xtags:lang,audioTrack:{id:lang+'.1',displayName:lang==='en'?'English':'Español',audioIsDefault:lang==='en'},url:'https://rr1.googlevideo.com/videoplayback?id=fixture&itag='+itag+'&expire=4102444800&xtags='+lang+'&sig=fixture&clen='+fs.statSync(path.join(root,lang+'.m4a')).size});
 const response={videoDetails:{videoId:id,lengthSeconds:'180'},streamingData:{adaptiveFormats:[audio('en',140,128000),audio('es',140,128000),audio('en',141,256000)]}};
 await browser.route('https://www.youtube.com/**',route=>route.fulfill({contentType:'text/html',body:'<!doctype html><meta charset=utf-8><title>UDM audio test - YouTube</title><style>body{margin:60px;font:16px Arial}video{width:640px;height:360px;background:#203345}</style><h1>UDM YouTube audio fixture</h1><div id="movie_player"><video controls></video></div><script>globalThis.ytInitialPlayerResponse='+JSON.stringify(response)+';const p=document.getElementById("movie_player");p.getVideoData=()=>({video_id:"'+id+'"});p.getPlayerResponse=()=>ytInitialPlayerResponse;</script>'}));
 page=await browser.newPage();const errors=[];page.on('pageerror',e=>errors.push(e.message));await choose('es');await choose('en');
 // The native refresh decision is covered by StreamRecoveryChecks. Here the
 // controlled acknowledgment exercises the production panel's review message.
 await worker.evaluate(()=>{const original=nativeRequest;nativeRequest=async(message,beforeSend,validate)=>{if(message.action!=='add')return original(message,beforeSend,validate);await validate?.(message);beforeSend?.();return {ok:true,id:'existing-fixture',refreshPending:true};};});
 await page.goto(url,{waitUntil:'domcontentloaded'});await until(()=>page.locator('[id^="udm-video-panel-"]').count());const refreshSession=await browser.newCDPSession(page);
 await click(refreshSession,'button',n=>n==='Download this video with UDM');await click(refreshSession,'button',n=>n==='Audio only (M4A)…');await activate(refreshSession,'Download audio (M4A)');
 await until(async()=>(await tree(refreshSession)).some(n=>(n.name?.value||'').includes('Fresh streams received. Review Refresh media session in UDM.')));
 assert.equal(jobs().length,2);await page.screenshot({path:path.join(root,'refresh-received.png')});pass('Panel displays the native refresh-review acknowledgment without adding a fixture download');await refreshSession.detach();
 assert.deepEqual(errors,[]);pass('YouTube audio panel has no page script errors');
})().catch(async e=>{console.error(e);results.push({passed:false,error:e.stack});try{if(page){await page.screenshot({path:path.join(root,'failure.png')});const s=await browser.newCDPSession(page);fs.writeFileSync(path.join(root,'failure-ui.json'),JSON.stringify(await tree(s),null,2));}}catch{}process.exitCode=1;}).finally(async()=>{if(browser)await browser.close();if(desktop)desktop.kill();if(nativeBridge)await nativeBridge.close();if(server){server.closeAllConnections();await new Promise(r=>server.close(r));}fs.mkdirSync(root,{recursive:true});fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({scope:'Real Chrome/Edge UI and native downloads. Synthetic YouTube page; production captured Googlevideo URL asserted before a test-only native boundary substitution to loopback. Does not establish compatibility with current YouTube CDN/session behavior.',browser:channel,passed:results.filter(r=>r.passed).length,failed:results.filter(r=>!r.passed).length,results,requests},null,2));});
