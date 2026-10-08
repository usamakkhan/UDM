'use strict';
// Synthetic clear recorded DASH, using a separate native catalog and browser profile.
const {chromium}=require('playwright'),fs=require('node:fs'),path=require('node:path'),http=require('node:http'),assert=require('node:assert/strict'),crypto=require('node:crypto'),{spawn,execFileSync}=require('node:child_process');
const project=path.resolve(process.env.UDM_PROJECT||path.join(__dirname,'..')),root=path.resolve(process.argv[2]),media=path.join(root,'media'),data=path.join(root,'state'),state=path.join(data,'state.json'),exe=process.env.UDM_APP_EXE,ff=process.env.UDM_FFMPEG,fp=path.join(path.dirname(ff),'ffprobe.exe'),tag='sidx'+crypto.randomBytes(8).toString('hex'),results=[],requests=[],files={};
let fixtureHostKey=null;const fixtureInputs={};let preparationTiming=[];
const expectedNative=process.env.UDM_EXPECT_NATIVE||'0.42.0',expectedBrowser=process.env.UDM_EXPECT_BROWSER||'0.34.0';
const cycles=Number(process.env.UDM_TEST_CYCLES||1),browserEvents=[];assert(Number.isInteger(cycles)&&cycles>=1&&cycles<=10,'Use 1–10 fixture cycles');
const firefox=process.env.UDM_TEST_BROWSER==='firefox',wait=ms=>new Promise(r=>setTimeout(r,ms));let browser,page,worker,desktop,server,base,active=0,peak=0,driver,driverBase,driverSession,pending=null,answer=null;
async function driverRequest(method,route,value){const response=await fetch(driverBase+route,{method,headers:{'Content-Type':'application/json'},body:value?JSON.stringify(value):undefined,signal:AbortSignal.timeout(45000)}),json=await response.json();if(json.value?.error)throw Error(json.value.message);return json.value;}
const command=(route,value)=>driverRequest('POST','/session/'+driverSession+route,value);
async function background(value){answer=null;pending=value;const result=await until(()=>answer);if(result.error)throw Error(result.error);return result;}
async function firefoxPanelElement(selector,label){return until(async()=>{
 const hosts=await command('/elements',{using:'css selector',value:'[id^="udm-video-panel-"]'});
 for(const host of hosts){const h=host['element-6066-11e4-a52e-4f735466cecf'];const shadow=await driverRequest('GET','/session/'+driverSession+'/element/'+h+'/shadow');const elements=await command('/shadow/'+shadow['shadow-6066-11e4-a52e-4f735466cecf']+'/elements',{using:'css selector',value:selector});
  for(const element of elements)if(await command('/execute/sync',{script:'return !!arguments[0].getClientRects().length && (arguments[0].getAttribute("aria-label")||arguments[0].textContent).includes(arguments[1]);',args:[element,label]}))return element;
 }return null;
});}
async function firefoxClick(element){await command('/element/'+element['element-6066-11e4-a52e-4f735466cecf']+'/click',{});}
async function until(fn,timeout=45000){const end=Date.now()+timeout;while(Date.now()<end){const v=await fn();if(v)return v;await wait(100);}throw Error('Indexed DASH fixture timed out');}
function jobs(){try{return JSON.parse(fs.readFileSync(state)).Downloads;}catch{return [];}}
function pass(name,detail={}){results.push({name,passed:true,...detail});console.log('PASS '+name);}
const run=args=>execFileSync(ff,['-hide_banner','-loglevel','error','-nostdin','-y',...args],{windowsHide:true,maxBuffer:16e6});
function probe(file){return JSON.parse(execFileSync(fp,['-v','error','-show_streams','-show_format','-of','json',file],{windowsHide:true}));}
function frequency(file){const raw=run(['-i',file,'-map','0:a:0','-ss','2','-t','2','-ar','48000','-ac','1','-f','s16le','pipe:1']);let n=0;for(let i=2;i<raw.length;i+=2)if(raw.readInt16LE(i-2)<=0&&raw.readInt16LE(i)>0)n++;return n/(raw.length/2/48000);}
function inspect(name){const bytes=fs.readFileSync(path.join(media,name));let at=0,box;while(at<bytes.length){let size=bytes.readUInt32BE(at);if(size===1)size=Number(bytes.readBigUInt64BE(at+8));assert(size>=8&&at+size<=bytes.length);if(bytes.toString('ascii',at+4,at+8)==='sidx'){assert(!box);box={start:at,length:size};}at+=size;}assert(box);const ranges=require(path.join(project,'browser/chromium/media.js')).sidx(bytes.subarray(box.start,box.start+box.length),box.start,bytes.length);assert(ranges.length>=4);files[name]={bytes,index:box,ranges};}
function representation(name){const f=files[name];return '<BaseURL>'+name+'</BaseURL><SegmentBase indexRange="'+f.index.start+'-'+(f.index.start+f.index.length-1)+'"><Initialization range="0-'+(f.index.start-1)+'"/></SegmentBase>';}
async function clickAX(session,role,name){const node=await until(async()=>{const tree=await session.send('Accessibility.getFullAXTree');return tree.nodes.find(n=>!n.ignored&&n.role?.value===role&&name(n.name?.value||''));});const {model}=await session.send('DOM.getBoxModel',{backendNodeId:node.backendDOMNodeId});const q=model.content;await page.mouse.click((q[0]+q[2])/2,(q[1]+q[5])/2);return node;}
async function choose(audioOnly=false,bad=false){
 const before=new Set(jobs().map(j=>j.Id)),start=requests.length;peak=0;let session;
 if(firefox){
  await command('/url',{url:base+(bad?'/bad':'/dash')});
  await until(()=>command('/execute/sync',{script:'return document.querySelector("video")?.videoHeight===360 && document.body.dataset.ready==="yes" && !!document.querySelector("video")?.getAttribute("data-udm-player")',args:[]}));
  await firefoxClick(await firefoxPanelElement('button','Download this video with UDM'));
  await firefoxClick(await firefoxPanelElement('button',audioOnly?'Audio only (M4A)…':'Audio/subtitle options for '));
  assert(!requests.slice(start).some(r=>r.type==='index'));pass('Firefox panel lists media options without prefetching indexes');
  const select=await firefoxPanelElement('select','Audio track');
  const options=await command('/element/'+select['element-6066-11e4-a52e-4f735466cecf']+'/elements',{using:'css selector',value:'option'});
  let spanish;for(const option of options)if(await command('/execute/sync',{script:'return arguments[0].textContent.includes("Español");',args:[option]}))spanish=option;assert(spanish);await firefoxClick(spanish);
  assert(await command('/execute/sync',{script:'return arguments[0].selectedOptions[0].textContent.includes("Español");',args:[select]}));
  await firefoxClick(await firefoxPanelElement('button',audioOnly?'Download audio (M4A)':'Download video'));
  if(bad){await firefoxPanelElement('p','did not return the requested DASH index range');assert.equal(jobs().filter(j=>!before.has(j.Id)).length,0);assert(!requests.slice(start).some(r=>r.type==='media'));pass('Firefox panel displays ignored Range error without creating a native transfer');return;}
 }else{
 await page.goto(base+(bad?'/bad':'/dash'));
 await page.waitForFunction(()=>document.querySelector('video').videoHeight===360&&document.body.dataset.ready==='yes');await until(()=>page.locator('[id^="udm-video-panel-"]').count());session=await browser.newCDPSession(page);
 await clickAX(session,'button',n=>n==='Download this video with UDM');await clickAX(session,'button',n=>audioOnly?n==='Audio only (M4A)…':n.startsWith('Audio/subtitle options for ')&&n.includes('MP4 · 360p'));
 assert(!requests.slice(start).some(r=>r.type==='index'));pass('Opening '+(audioOnly?'audio':'video')+' choices does not prefetch media indexes');
 await clickAX(session,'combobox',n=>n==='Audio track');await page.keyboard.press('End');await page.keyboard.press('Enter');
 const tree=await session.send('Accessibility.getFullAXTree');assert.match(tree.nodes.find(n=>!n.ignored&&n.role?.value==='combobox'&&n.name?.value==='Audio track').value.value,/Español/);
 await clickAX(session,'button',n=>n===(audioOnly?'Download audio (M4A)':'Download video'));
 if(bad){await until(async()=>{const t=await session.send('Accessibility.getFullAXTree');return t.nodes.some(n=>!n.ignored&&/did not return the requested DASH index range/.test(n.name?.value||''));});assert.equal(jobs().filter(j=>!before.has(j.Id)).length,0);assert(!requests.slice(start).some(r=>r.type==='media'));pass('A server ignoring Range shows an error and creates no native transfer');await session.detach();return;}
 }
 const job=await until(()=>{const j=jobs().find(x=>!before.has(x.Id));if(j?.Status==='Failed')throw Error(j.Error);return j?.Status==='Complete'&&j;},60000),file=path.join(job.Folder,job.FileName),out=probe(file),tone=frequency(file);
 assert.equal(out.streams.some(s=>s.codec_type==='video'&&s.height===360),!audioOnly);const audio=out.streams.filter(s=>s.codec_type==='audio');assert.equal(audio.length,1);assert.equal(audio[0].tags.language,'spa');assert(Math.abs(tone-880)<3);assert.equal(path.extname(file),audioOnly?'.m4a':'.mp4');assert(Math.abs(Number(out.format.duration)-8)<0.1);
 const seen=requests.slice(start),tracks=audioOnly?['es.mp4']:['v.mp4','es.mp4'];assert(!seen.some(r=>r.name==='en.mp4'));assert(!seen.some(r=>r.name==='v.mp4')||!audioOnly);
 for(const name of tracks){const f=files[name],got=seen.filter(r=>r.name===name);assert(got.every(r=>r.range&&r.status===206));assert.equal(got.filter(r=>r.type==='index').length,1);const expected=[{start:0,length:f.index.start},...f.ranges].map(p=>'bytes='+p.start+'-'+(p.start+p.length-1)).sort();assert.deepEqual(got.filter(r=>r.type!=='index').map(r=>r.range).sort(),expected);}
 const segments=tracks.reduce((n,t)=>n+1+files[t].ranges.length,0);assert.equal(job.AdaptiveTotalSegments,segments);assert.equal(job.AdaptiveCompletedSegments,segments);assert(peak>=2);
 pass((audioOnly?'M4A':'MP4')+' contains the selected Spanish track with correct duration and decoded audio',{duration:Number(out.format.duration),frequencyHz:tone,sha256:job.Sha256});
 pass((audioOnly?'Audio-only':'Video')+' downloads exact indexed ranges in parallel without unselected tracks',{segments,peakConcurrentMediaRequests:peak});
 if(session)await session.detach();
}
(async()=>{
 assert(exe&&ff,'Set UDM_APP_EXE and UDM_FFMPEG');assert(!fs.existsSync(state),'Use a new test folder');fs.mkdirSync(media,{recursive:true});fs.mkdirSync(data,{recursive:true});
 const flags='+frag_keyframe+empty_moov+default_base_moof+global_sidx';
 run(['-f','lavfi','-i','testsrc2=size=640x360:rate=24','-t','8','-c:v','libx264','-threads','2','-preset','ultrafast','-pix_fmt','yuv420p','-g','48','-keyint_min','48','-sc_threshold','0','-movflags',flags,'-frag_duration','2000000',path.join(media,'v.mp4')]);
 run(['-i',path.join(media,'v.mp4'),'-c','copy','-movflags','+faststart',path.join(media,'preview.mp4')]);
 for(const [lang,tone] of [['en',440],['es',880]])run(['-f','lavfi','-i','sine=frequency='+tone+':sample_rate=48000','-t','8','-c:a','aac','-b:a','96k','-movflags',flags,'-frag_duration','2000000',path.join(media,lang+'.mp4')]);
 for(const name of ['v.mp4','en.mp4','es.mp4'])inspect(name);
 const mpd='<MPD type="static" mediaPresentationDuration="PT8S"><Period><AdaptationSet mimeType="video/mp4"><Representation id="v" height="360" bandwidth="1000000">'+representation('v.mp4')+'</Representation></AdaptationSet><AdaptationSet mimeType="audio/mp4" lang="en"><Label>English</Label><Role value="main"/><Representation id="en">'+representation('en.mp4')+'</Representation></AdaptationSet><AdaptationSet mimeType="audio/mp4" lang="es"><Label>Español</Label><Representation id="es">'+representation('es.mp4')+'</Representation></AdaptationSet></Period></MPD>';
 server=http.createServer((req,res)=>{
  const u=new URL(req.url,'http://localhost');res.setHeader('Cache-Control','no-store');
  if(u.pathname==='/command'){res.setHeader('Content-Type','application/json');res.end(JSON.stringify(pending));pending=null;return;}
  if(u.pathname==='/answer'){let value='';req.on('data',c=>value+=c);req.on('end',()=>{answer=JSON.parse(value);res.end('{}');});return;}
  if(['/dash','/bad'].includes(u.pathname)){res.setHeader('Content-Type','text/html; charset=utf-8');res.end('<!doctype html><meta charset=utf-8><title>UDM indexed DASH test</title><style>body{margin:60px;font:16px Arial}video{width:640px}</style><h1>Indexed DASH test</h1><video controls autoplay muted loop></video><script>(async()=>{const v=document.querySelector("video");v.src=URL.createObjectURL(new Blob([await fetch("/preview.mp4").then(r=>r.arrayBuffer())],{type:"video/mp4"}));await fetch("/'+(u.pathname==='/bad'?'bad':'master')+'.mpd").then(r=>r.text());document.body.dataset.ready="yes";await v.play();})();</script>');return;}
  if(['/master.mpd','/bad.mpd'].includes(u.pathname)){res.setHeader('Content-Type','application/dash+xml');res.end(u.pathname==='/bad.mpd'?mpd.replaceAll('.mp4</BaseURL>','.mp4?bad=1</BaseURL>'):mpd);return;}
  const name=u.pathname.slice(1),f=files[name],bytes=f?.bytes||(name==='preview.mp4'?fs.readFileSync(path.join(media,name)):null);if(!bytes){res.writeHead(404).end();return;}
  const range=/^bytes=(\d+)-(\d*)$/.exec(req.headers.range||''),a=range?+range[1]:0,b=range&&range[2]?+range[2]:bytes.length-1;if(a>b||b>=bytes.length){res.writeHead(416).end();return;}
  const type=f?(a===f.index.start?'index':a>=f.ranges[0].start?'media':'init'):'preview',bad=u.searchParams.has('bad')&&type==='index',status=bad?200:range?206:200;
  requests.push({name,type,range:req.headers.range,status});res.writeHead(status,{'Content-Type':'video/mp4','Content-Length':bad?bytes.length:b-a+1,ETag:'"sidx-fixture"',...(range&&!bad?{'Content-Range':'bytes '+a+'-'+b+'/'+bytes.length}:{})});
  if(type==='media'){active++;peak=Math.max(peak,active);let done=false;res.on('close',()=>{if(!done){active--;done=true;}});setTimeout(()=>res.end(bytes.subarray(a,b+1)),180);}else res.end(bad?bytes:bytes.subarray(a,b+1));
 });await new Promise(r=>server.listen(0,'127.0.0.1',r));base='http://127.0.0.1:'+server.address().port;
 fs.writeFileSync(state,JSON.stringify({Schema:1,Settings:{DownloadFolder:path.join(root,'downloads'),CategoryFolders:false,Connections:4,Parallel:1,ProxyMode:'Connect directly',SkipBrowserFileInfo:true,PrefetchFileInfo:false,SuppressProgressDialog:true,SuppressCompletionDialog:true,Sound:false},Queues:[{Name:'Main queue',Enabled:true,Parallel:1}],Projects:[],Downloads:[]}));
 desktop=spawn(exe,['--background','--data-dir',data,'--instance-tag',tag],{windowsHide:true,stdio:'ignore'});
 const ext=path.join(root,'extension');fs.cpSync(path.join(project,'browser',firefox?'firefox':'chromium'),ext,{recursive:true});
 if(process.env.UDM_TEST_NATIVE_HOST){
  const host=path.resolve(process.env.UDM_TEST_NATIVE_HOST),manifest=JSON.parse(fs.readFileSync(path.join(ext,'manifest.json'))),hostName='com.udm.sidxfixture'+crypto.randomBytes(8).toString('hex');
  const extensionId=firefox?manifest.browser_specific_settings.gecko.id:crypto.createHash('sha256').update(Buffer.from(manifest.key,'base64')).digest('hex').slice(0,32).replace(/[0-9a-f]/g,c=>String.fromCharCode(97+parseInt(c,16)));
  const hash=file=>crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');Object.assign(fixtureInputs,{project,app:exe,appHash:hash(exe),nativeHost:host,nativeHostHash:hash(host),manifestHash:hash(path.join(ext,'manifest.json'))});
  const hostFile=path.join(root,'native-host.json');fs.writeFileSync(hostFile,JSON.stringify({name:hostName,description:'Isolated UDM package media test',path:host,type:'stdio',...(firefox?{allowed_extensions:[extensionId]}:{allowed_origins:['chrome-extension://'+extensionId+'/']})}));
  const key='HKCU\\Software\\'+(firefox?'Mozilla':'Microsoft\\Edge')+'\\NativeMessagingHosts\\'+hostName;let exists=false;try{execFileSync('reg.exe',['query',key],{stdio:'ignore',windowsHide:true});exists=true;}catch{}assert(!exists,'Test host key must be new');
  execFileSync('reg.exe',['add',key,'/ve','/t','REG_SZ','/d',hostFile,'/f'],{stdio:'ignore',windowsHide:true});fixtureHostKey=key;
  const bg=path.join(ext,'background.js'),source=fs.readFileSync(bg,'utf8');assert(source.includes("'com.udm.download_manager'"));fs.writeFileSync(bg,source.replace("'com.udm.download_manager'",JSON.stringify(hostName)));
 }
 if(firefox){
  // A temporary fixture observer selects the same UdmSites actions using the real page token.
  const observer='let fixtureChoice;setInterval(async()=>{try{const m=await(await fetch('+JSON.stringify(base+'/command')+')).json();if(!m)return;let result;if(m.action==="choices"){const tab=(await api.tabs.query({})).find(t=>t.url?.startsWith('+JSON.stringify(base+'/')+'));const injected=(await api.scripting.executeScript({target:{tabId:tab.id},world:"MAIN",func:()=>({token:document.querySelector("video").getAttribute("data-udm-player"),page:location.href})}))[0];fixtureChoice={message:injected.result,sender:{tab:{id:tab.id},frameId:0,url:tab.url,...(injected.documentId?{documentId:injected.documentId}:{})}};result=await UdmSites.list(fixtureChoice.message,fixtureChoice.sender);}else if(m.action==="select"){result=await UdmSites.download({...fixtureChoice.message,...m},fixtureChoice.sender);}else if(m.action==="version"){result={version:api.runtime.getManifest().version};}else result=await nativeRequest(m);await fetch('+JSON.stringify(base+'/answer')+',{method:"POST",body:JSON.stringify(result)});}catch(e){await fetch('+JSON.stringify(base+'/answer')+',{method:"POST",body:JSON.stringify({error:e.message})}).catch(()=>{});}},100);';
  fs.appendFileSync(path.join(ext,'background.js'),'\n'+observer);
  const portServer=require('node:net').createServer();await new Promise(r=>portServer.listen(0,'127.0.0.1',r));const port=portServer.address().port;await new Promise(r=>portServer.close(r));driverBase='http://127.0.0.1:'+port;
  const profile=path.join(root,'profile');fs.mkdirSync(profile);driver=spawn(process.env.UDM_GECKODRIVER,['--port',String(port),'--profile-root',root],{windowsHide:true,env:{...process.env,UDM_INSTANCE_TAG:tag},stdio:['ignore','pipe','pipe']});const log=fs.createWriteStream(path.join(root,'driver.log'));driver.stdout.pipe(log);driver.stderr.pipe(log);
  await until(async()=>{try{return await driverRequest('GET','/status');}catch{return false;}});
  const created=await driverRequest('POST','/session',{capabilities:{alwaysMatch:{'moz:firefoxOptions':{binary:process.env.UDM_FIREFOX,args:['-headless','-profile',profile],prefs:{'browser.shell.checkDefaultBrowser':false,'media.autoplay.default':0,'extensions.webextensions.warnings-as-errors':false}}}}});driverSession=created.sessionId;
  assert.equal(await command('/moz/addon/install',{path:ext,temporary:true}),'udm@local.example');const info=await background({action:'diagnostics'});assert.equal(info.version,expectedNative);assert.equal(path.resolve(info.dataDirectory).toLowerCase(),data.toLowerCase());assert.equal(info.downloads,0);assert.equal((await background({action:'version'})).version,expectedBrowser);pass('Firefox extension '+expectedBrowser+' connects only to isolated native '+expectedNative);
  await choose();await choose(true);await choose(false,true);
 }else{
 browser=await chromium.launchPersistentContext(path.join(root,'profile'),{channel:process.env.UDM_TEST_BROWSER||'msedge',headless:true,ignoreDefaultArgs:['--disable-extensions'],env:{...process.env,UDM_INSTANCE_TAG:tag},viewport:{width:1100,height:850},args:['--enable-unsafe-extension-debugging','--autoplay-policy=no-user-gesture-required']});
 const cdp=await browser.browser().newBrowserCDPSession();await cdp.send('Extensions.loadUnpacked',{path:ext});worker=browser.serviceWorkers()[0]||await browser.waitForEvent('serviceworker');
  if(process.env.UDM_TEST_PREPARE_DIAGNOSTICS==='1')await worker.evaluate(()=>{
  const events=[];globalThis.__udmFixtureTiming=events;
  const timed=async(name,operation)=>{const event={operation:name,startMs:performance.now(),status:'pending'};events.push(event);if(events.length>1000)events.shift();try{const result=await operation();event.status='completed';return result;}catch(error){event.status='rejected';throw error;}finally{event.durationMs=performance.now()-event.startMs;}};
  for(const [object,key,label] of [[chrome.tabs,'get','tab'],[chrome.storage.local,'get','local settings'],[chrome.storage.session,'get','session storage'],[chrome.scripting,'executeScript','player inspection'],[chrome.permissions,'contains','permission']]){const original=object[key];object[key]=function(...args){return timed(label,()=>original.apply(object,args));};}
  const originalFetch=globalThis.fetch;globalThis.fetch=async function(...args){const response=await timed('fetch headers',()=>originalFetch.apply(this,args));if(response.body){const originalReader=response.body.getReader.bind(response.body);response.body.getReader=function(...options){const reader=originalReader(...options),originalRead=reader.read.bind(reader);reader.read=(...options)=>timed('response body read',()=>originalRead(...options));return reader;};}return response;};
 });
 const info=await until(async()=>{try{return await worker.evaluate(()=>nativeRequest({action:'diagnostics'}));}catch{return null;}});assert.equal(info.version,expectedNative);assert.equal(path.resolve(info.dataDirectory).toLowerCase(),data.toLowerCase());assert.equal(info.downloads,0);assert.equal(await worker.evaluate(()=>chrome.runtime.getManifest().version),expectedBrowser);pass('Extension '+expectedBrowser+' connects to isolated native '+expectedNative);
 page=browser.pages()[0]||await browser.newPage();const errors=[];
 // Keep fixture-only diagnostics even when readiness fails before assertions run.
 page.on('pageerror',e=>{errors.push(e.message);browserEvents.push({event:'pageerror',message:e.message});});
 page.on('requestfailed',r=>browserEvents.push({event:'requestfailed',path:new URL(r.url()).pathname,error:r.failure()?.errorText}));
 page.on('response',r=>{if(r.url().startsWith(base))browserEvents.push({event:'response',path:new URL(r.url()).pathname,status:r.status()});});
 page.on('console',m=>{if(m.type()==='error')browserEvents.push({event:'console-error',message:m.text()});});
 for(let cycle=0;cycle<cycles;cycle++){await choose();await choose(true);await choose(false,true);}
 assert.deepEqual(errors,[]);pass('Indexed media pages have no script errors');
 }
})().catch(async e=>{if(page){try{const session=await browser.newCDPSession(page);const tree=await session.send('Accessibility.getFullAXTree');fs.writeFileSync(path.join(root,'failure-ui.json'),JSON.stringify(tree,null,2));await page.screenshot({path:path.join(root,'failure.png')});console.error('FIXTURE PAGE',await page.evaluate(()=>({url:location.href,ready:document.body.dataset.ready,video:{height:document.querySelector('video')?.videoHeight,error:document.querySelector('video')?.error?.message}})));}catch{}}console.error(e);results.push({passed:false,error:e.stack});process.exitCode=1;}).finally(async()=>{if(worker&&process.env.UDM_TEST_PREPARE_DIAGNOSTICS==='1'){try{preparationTiming=await worker.evaluate(()=>globalThis.__udmFixtureTiming||[]);console.log('PREPARATION TIMING',JSON.stringify(preparationTiming.filter(x=>x.status==='pending'||x.durationMs>200).slice(-20)));}catch(e){preparationTiming=[{operation:'diagnostic collection',status:'unavailable'}];}}if(browser)await browser.close();if(driverSession)try{await driverRequest('DELETE','/session/'+driverSession);}catch{}if(driver)driver.kill();if(desktop)desktop.kill();if(server){server.closeAllConnections();await new Promise(r=>server.close(r));}if(fixtureHostKey){execFileSync('reg.exe',['delete',fixtureHostKey,'/f'],{stdio:'ignore',windowsHide:true});let remaining=false;try{execFileSync('reg.exe',['query',fixtureHostKey],{stdio:'ignore',windowsHide:true});remaining=true;}catch{}assert(!remaining,'Test host registration must be removed');fixtureInputs.registrationRemoved=true;}fs.mkdirSync(root,{recursive:true});fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({scope:'Real isolated browser/native app; synthetic clear recorded indexed DASH. Chrome/Edge and Firefox use real panel UI actions; Firefox observer is used only for initial bridge/version diagnostics.',fixtureInputs,browser:process.env.UDM_TEST_BROWSER||'msedge',passed:results.filter(x=>x.passed).length,failed:results.filter(x=>!x.passed).length,results,requests,browserEvents,cycles,preparationTiming},null,2));});
