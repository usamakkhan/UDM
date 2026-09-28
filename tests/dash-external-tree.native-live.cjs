'use strict';
// Synthetic clear recorded DASH, using a separate native catalog and browser profile.
const {chromium}=require('playwright'),fs=require('node:fs'),path=require('node:path'),http=require('node:http'),assert=require('node:assert/strict'),crypto=require('node:crypto'),{spawn,execFileSync}=require('node:child_process');
const {externalTree}=require('./support/external-tree.cjs');
const project=path.resolve(__dirname,'..'),root=path.resolve(process.argv[2]),media=path.join(root,'media'),data=path.join(root,'state'),state=path.join(data,'state.json'),exe=process.env.UDM_APP_EXE,ff=process.env.UDM_FFMPEG,fp=path.join(path.dirname(ff),'ffprobe.exe'),tag='sidx'+crypto.randomBytes(8).toString('hex'),results=[],requests=[],files={},mixedFiles={};
const firefox=process.env.UDM_TEST_BROWSER==='firefox',wait=ms=>new Promise(r=>setTimeout(r,ms));let browser,page,worker,desktop,server,indexServer,base,indexBase,active=0,peak=0,driver,driverBase,driverSession,pending=null,answer=null;
async function driverRequest(method,route,value){const response=await fetch(driverBase+route,{method,headers:{'Content-Type':'application/json'},body:value?JSON.stringify(value):undefined,signal:AbortSignal.timeout(45000)}),json=await response.json();if(json.value?.error)throw Error(json.value.message);return json.value;}
const command=(route,value)=>driverRequest('POST','/session/'+driverSession+route,value);
async function background(value){answer=null;pending=value;const result=await until(()=>answer);if(result.error)throw Error(result.error);return result;}
async function until(fn,timeout=45000){const end=Date.now()+timeout;while(Date.now()<end){const v=await fn();if(v)return v;await wait(100);}throw Error('External DASH tree fixture timed out');}
function jobs(){try{return JSON.parse(fs.readFileSync(state)).Downloads;}catch{return [];}}
function pass(name,detail={}){results.push({name,passed:true,...detail});console.log('PASS '+name);}
const run=args=>execFileSync(ff,['-hide_banner','-loglevel','error','-nostdin','-y',...args],{windowsHide:true,maxBuffer:16e6});
function probe(file){return JSON.parse(execFileSync(fp,['-v','error','-show_streams','-show_format','-of','json',file],{windowsHide:true}));}
function frequency(file){const raw=run(['-i',file,'-map','0:a:0','-ss','2','-t','2','-ar','48000','-ac','1','-f','s16le','pipe:1']);let n=0;for(let i=2;i<raw.length;i+=2)if(raw.readInt16LE(i-2)<=0&&raw.readInt16LE(i)>0)n++;return n/(raw.length/2/48000);}
function inspect(name){const source=fs.readFileSync(path.join(media,name));files[name]=externalTree(source);mixedFiles[name]=externalTree(source,{mixed:true});assert(files[name].ranges.length>=4);assert(files[name].bytes.equals(mixedFiles[name].bytes));fs.writeFileSync(path.join(media,name),files[name].bytes);}
const sourceFor=(name,mode)=>(mode.includes('mixed')?mixedFiles:files)[name];
function representation(name,mode){const f=sourceFor(name,mode),indexUrl=indexBase+'/'+name+'.idx?mode='+mode;return '<BaseURL>'+name+'?mode='+mode+'</BaseURL><SegmentBase><Initialization range="0-'+(f.initLength-1)+'"/><RepresentationIndex sourceURL="'+indexUrl+'"'+(mode.includes('ranged')?' range="'+f.index.start+'-'+(f.index.start+f.index.length-1)+'"':'')+'/></SegmentBase>';}
async function clickAX(session,role,name){const node=await until(async()=>{const tree=await session.send('Accessibility.getFullAXTree');return tree.nodes.find(n=>!n.ignored&&n.role?.value===role&&name(n.name?.value||''));});const {model}=await session.send('DOM.getBoxModel',{backendNodeId:node.backendDOMNodeId});const q=model.content;await page.mouse.click((q[0]+q[2])/2,(q[1]+q[5])/2);return node;}
async function choose(audioOnly=false,mode='whole'){
 const nativeFailure=mode==='native-change',failureMode=mode.startsWith('ranged-child'),failure=/requested DASH index range|DASH media changed|identity or timing|Failed to fetch|NetworkError|Network request failed/;
 const before=new Set(jobs().map(j=>j.Id)),start=requests.length;peak=0;let session;
 if(firefox){
  await command('/url',{url:base+'/dash?mode='+mode});
  await until(()=>command('/execute/sync',{script:'return document.querySelector("video")?.videoHeight===360 && document.body.dataset.ready==="yes" && !!document.querySelector("video")?.getAttribute("data-udm-player")',args:[]}));
  const list=await background({action:'choices'}),choice=list.choices.find(c=>c.label.includes('MP4 · 360p'));assert(choice);assert(!requests.slice(start).some(r=>r.type==='index'||r.type==='probe'));pass('Firefox external choices do not prefetch indexes or media probes');
  const audio=choice.audioOptions.find(a=>a.label.includes('Español'));assert(audio);const selection={action:'select',key:choice.key,audioKey:audio.key,...(audioOnly?{output:'audio'}:{})};
  if(failureMode){await assert.rejects(background(selection),failure);assert.equal(jobs().filter(j=>!before.has(j.Id)).length,0);assert(!requests.slice(start).some(r=>r.type==='media'));pass('Firefox '+mode+' refuses the invalid external handoff');return;}
  await background(selection);
 }else{
  await page.goto(base+'/dash?mode='+mode);await page.waitForFunction(()=>document.querySelector('video').videoHeight===360&&document.body.dataset.ready==='yes');await until(()=>page.locator('[id^="udm-video-panel-"]').count());session=await browser.newCDPSession(page);
  await clickAX(session,'button',n=>n==='Download this video with UDM');await clickAX(session,'button',n=>audioOnly?n==='Audio only (M4A)…':n.includes('MP4 · 360p'));
  assert(!requests.slice(start).some(r=>r.type==='index'||r.type==='probe'));pass('Opening '+mode+' '+(audioOnly?'audio':'video')+' choices performs no index/probe reads');
  await clickAX(session,'combobox',n=>n==='Audio track');await page.keyboard.press('End');await page.keyboard.press('Enter');
  const tree=await session.send('Accessibility.getFullAXTree');assert.match(tree.nodes.find(n=>!n.ignored&&n.role?.value==='combobox'&&n.name?.value==='Audio track').value.value,/Español/);
  await clickAX(session,'button',n=>n===(audioOnly?'Download audio (M4A)':'Download video'));
  if(failureMode){await until(async()=>{const t=await session.send('Accessibility.getFullAXTree');return t.nodes.some(n=>!n.ignored&&failure.test(n.name?.value||''));});assert.equal(jobs().filter(j=>!before.has(j.Id)).length,0);assert(!requests.slice(start).some(r=>r.type==='media'));pass(mode+' shows an actionable error and creates no native transfer');await session.detach();return;}
 }
 if(nativeFailure){const job=await until(()=>{const j=jobs().find(x=>!before.has(x.Id));assert(j?.Status!=='Complete');return j?.Status==='Failed'&&j;});assert.match(job.Error,/streaming file changed/);assert.equal(job.Received,0);assert(!fs.existsSync(path.join(job.Folder,job.FileName)));pass('A media file changing after its probe is rejected by native 0.42.0');if(session)await session.detach();return;}
 const job=await until(()=>{const j=jobs().find(x=>!before.has(x.Id));if(j?.Status==='Failed')throw Error(j.Error);return j?.Status==='Complete'&&j;},60000),file=path.join(job.Folder,job.FileName),out=probe(file),tone=frequency(file);
 assert.equal(out.streams.some(s=>s.codec_type==='video'&&s.height===360),!audioOnly);const audio=out.streams.filter(s=>s.codec_type==='audio');assert.equal(audio.length,1);assert.equal(audio[0].tags.language,'spa');assert(Math.abs(tone-880)<3);assert.equal(path.extname(file),audioOnly?'.m4a':'.mp4');assert(Math.abs(Number(out.format.duration)-8)<0.1);
 const seen=requests.slice(start),tracks=audioOnly?['es.mp4']:['v.mp4','es.mp4'];assert(!seen.some(r=>r.name==='en.mp4'));assert(!seen.some(r=>r.name==='v.mp4')||!audioOnly);
 for(const name of tracks){
  const f=sourceFor(name,mode),got=seen.filter(r=>r.name===name),indexes=got.filter(r=>r.type==='index'),probes=got.filter(r=>r.type==='probe'),segments=got.filter(r=>r.type==='init'||r.type==='media');
  if(mode.includes('ranged')){
   assert.deepEqual(indexes.map(r=>r.range),f.indexReads.map(x=>'bytes='+x.start+'-'+(x.start+x.length-1)));assert(indexes.every(r=>r.status===206));assert(indexes.slice(1).every(r=>r.ifMatch==='"index-'+name+'"'));
  }else{assert.equal(indexes.length,1);assert.equal(indexes[0].range,undefined);assert.equal(indexes[0].status,200);}
  assert(indexes.every(r=>r.url.startsWith(indexBase)));
  assert.equal(probes.length,1);assert.equal(probes[0].range,'bytes=0-0');assert.equal(probes[0].bytes,1);
  const expected=[{start:0,length:f.initLength},...f.ranges].map(p=>'bytes='+p.start+'-'+(p.start+p.length-1)).sort();
  assert.deepEqual(segments.map(r=>r.range).sort(),expected);assert(segments.every(r=>r.status===206&&r.ifMatch==='"media-'+name+'"'&&!r.url.startsWith(indexBase)));
 }
 const segments=tracks.reduce((n,t)=>n+1+files[t].ranges.length,0);assert.equal(job.AdaptiveTotalSegments,segments);assert.equal(job.AdaptiveCompletedSegments,segments);assert(peak>=2);
 pass(mode+' '+(audioOnly?'M4A':'MP4')+' fully decodes with the selected Spanish audio',{duration:Number(out.format.duration),frequencyHz:tone,sha256:job.Sha256});
 run(['-i',file,'-f','null','-']);
 pass(mode+' '+(audioOnly?'audio':'video')+' uses exact media ranges, independent validators and parallel requests',{segments,peakConcurrentMediaRequests:peak});
 if(session)await session.detach();
}

(async()=>{
 assert(exe&&ff,'Set UDM_APP_EXE and UDM_FFMPEG');assert(!fs.existsSync(state),'Use a new test folder');fs.mkdirSync(media,{recursive:true});fs.mkdirSync(data,{recursive:true});
 const flags='+frag_keyframe+empty_moov+default_base_moof+global_sidx';
 run(['-f','lavfi','-i','testsrc2=size=640x360:rate=24','-t','8','-c:v','libx264','-threads','2','-preset','ultrafast','-pix_fmt','yuv420p','-g','48','-keyint_min','48','-sc_threshold','0','-movflags',flags,'-frag_duration','2000000',path.join(media,'v.mp4')]);
 run(['-i',path.join(media,'v.mp4'),'-c','copy','-movflags','+faststart',path.join(media,'preview.mp4')]);
 for(const [lang,tone] of [['en',440],['es',880]])run(['-f','lavfi','-i','sine=frequency='+tone+':sample_rate=48000','-t','8','-c:a','aac','-b:a','96k','-movflags',flags,'-frag_duration','2000000',path.join(media,lang+'.mp4')]);
 for(const name of ['v.mp4','en.mp4','es.mp4'])inspect(name);
 const manifest=mode=>'<MPD type="static" mediaPresentationDuration="PT8S"><Period><AdaptationSet mimeType="video/mp4"><Representation id="v" height="360" bandwidth="1000000">'+representation('v.mp4',mode)+'</Representation></AdaptationSet><AdaptationSet mimeType="audio/mp4" lang="en"><Label>English</Label><Role value="main"/><Representation id="en">'+representation('en.mp4',mode)+'</Representation></AdaptationSet><AdaptationSet mimeType="audio/mp4" lang="es"><Label>Español</Label><Representation id="es">'+representation('es.mp4',mode)+'</Representation></AdaptationSet></Period></MPD>';
 indexServer=http.createServer((req,res)=>{
  const u=new URL(req.url,'http://localhost'),mode=u.searchParams.get('mode'),name=u.pathname.slice(1).replace(/\.idx$/,''),f=sourceFor(name,mode);res.setHeader('Cache-Control','no-store');
  if(!f){res.writeHead(404).end();return;}const m=/^bytes=(\d+)-(\d+)$/.exec(req.headers.range||''),a=m?+m[1]:0,b=m?+m[2]:f.indexBytes.length-1;
  const child=!!m&&a!==f.index.start,status=child&&mode==='ranged-child-ignored'?200:child&&mode==='ranged-child-412'?412:child&&mode==='ranged-child-redirect'?302:m?206:200;
  requests.push({url:indexBase+req.url,name,type:'index',range:req.headers.range,status,ifMatch:req.headers['if-match']});
  const headers={'Content-Type':'application/octet-stream','Content-Length':b-a+1,ETag:child&&mode==='ranged-child-etag'?'"changed"':'"index-'+name+'"',...(m&&status===206?{'Content-Range':'bytes '+a+'-'+b+'/'+f.indexBytes.length}:{})};if(status===302)headers.Location=base+'/redirect-target';
  const body=Buffer.from(f.indexBytes.subarray(a,b+1));if(child&&mode==='ranged-child-identity')body.writeUInt32BE(2,12);res.writeHead(status,headers);res.end(body);
 });await new Promise(r=>indexServer.listen(0,'127.0.0.1',r));indexBase='http://127.0.0.1:'+indexServer.address().port;
 server=http.createServer((req,res)=>{
  const u=new URL(req.url,'http://localhost'),mode=u.searchParams.get('mode')||'whole';res.setHeader('Cache-Control','no-store');
  if(u.pathname==='/command'){res.setHeader('Content-Type','application/json');res.end(JSON.stringify(pending));pending=null;return;}
  if(u.pathname==='/answer'){let value='';req.on('data',c=>value+=c);req.on('end',()=>{answer=JSON.parse(value);res.end('{}');});return;}
  if(u.pathname==='/dash'){res.setHeader('Content-Type','text/html; charset=utf-8');res.end('<!doctype html><meta charset=utf-8><title>UDM external DASH tree test</title><style>body{margin:60px;font:16px Arial}video{width:640px}</style><h1>External DASH test</h1><video controls autoplay muted loop></video><script>(async()=>{const v=document.querySelector("video");v.src=URL.createObjectURL(new Blob([await fetch("/preview.mp4").then(r=>r.arrayBuffer())],{type:"video/mp4"}));await fetch("/master.mpd?mode='+mode+'").then(r=>r.text());document.body.dataset.ready="yes";await v.play();})();</script>');return;}
  if(u.pathname==='/master.mpd'){res.setHeader('Content-Type','application/dash+xml');res.end(manifest(mode));return;}
  const name=u.pathname.slice(1),f=files[name],bytes=f?.bytes||(name==='preview.mp4'?fs.readFileSync(path.join(media,name)):null);if(!bytes){res.writeHead(404).end();return;}
  const range=/^bytes=(\d+)-(\d*)$/.exec(req.headers.range||''),a=range?+range[1]:0,b=range&&range[2]?+range[2]:bytes.length-1;if(a>b||b>=bytes.length){res.writeHead(416).end();return;}
  const type=f?(a===0&&b===0?'probe':a>=f.ranges[0].start?'media':'init'):'preview',status=mode==='probe-bad'&&type==='probe'?200:range?206:200,etag=mode==='native-change'&&(type==='media'||type==='init')?'"changed"':'"media-'+name+'"';
  requests.push({url:base+req.url,name,type,range:req.headers.range,status,ifMatch:req.headers['if-match'],bytes:b-a+1});res.writeHead(status,{'Content-Type':'video/mp4','Content-Length':b-a+1,ETag:etag,...(range?{'Content-Range':'bytes '+a+'-'+b+'/'+bytes.length}:{})});
  if(type==='media'){active++;peak=Math.max(peak,active);let done=false;res.on('close',()=>{if(!done){active--;done=true;}});setTimeout(()=>res.end(bytes.subarray(a,b+1)),180);}else res.end(bytes.subarray(a,b+1));
 });await new Promise(r=>server.listen(0,'127.0.0.1',r));base='http://127.0.0.1:'+server.address().port;
 fs.writeFileSync(state,JSON.stringify({Schema:1,Settings:{DownloadFolder:path.join(root,'downloads'),CategoryFolders:false,Connections:4,Parallel:1,ProxyMode:'Connect directly',SkipBrowserFileInfo:true,PrefetchFileInfo:false,SuppressProgressDialog:true,SuppressCompletionDialog:true,Sound:false},Queues:[{Name:'Main queue',Enabled:true,Parallel:1}],Projects:[],Downloads:[]}));
 desktop=spawn(exe,['--background','--data-dir',data,'--instance-tag',tag],{windowsHide:true,stdio:'ignore'});
 const ext=path.join(root,'extension');fs.cpSync(path.join(project,'browser',firefox?'firefox':'chromium'),ext,{recursive:true});
 if(firefox){
  // A temporary fixture observer selects the same UdmSites actions using the real page token.
  const observer='let fixtureChoice;setInterval(async()=>{try{const m=await(await fetch('+JSON.stringify(base+'/command')+')).json();if(!m)return;let result;if(m.action==="choices"){const tab=(await api.tabs.query({})).find(t=>t.url?.startsWith('+JSON.stringify(base+'/')+'));const injected=(await api.scripting.executeScript({target:{tabId:tab.id},world:"MAIN",func:()=>({token:document.querySelector("video").getAttribute("data-udm-player"),page:location.href})}))[0];fixtureChoice={message:injected.result,sender:{tab:{id:tab.id},frameId:0,url:tab.url,...(injected.documentId?{documentId:injected.documentId}:{})}};result=await UdmSites.list(fixtureChoice.message,fixtureChoice.sender);}else if(m.action==="select"){result=await UdmSites.download({...fixtureChoice.message,...m},fixtureChoice.sender);}else if(m.action==="version"){result={version:api.runtime.getManifest().version};}else result=await nativeRequest(m);await fetch('+JSON.stringify(base+'/answer')+',{method:"POST",body:JSON.stringify(result)});}catch(e){await fetch('+JSON.stringify(base+'/answer')+',{method:"POST",body:JSON.stringify({error:e.message})}).catch(()=>{});}},100);';
  fs.appendFileSync(path.join(ext,'background.js'),'\n'+observer);
  const portServer=require('node:net').createServer();await new Promise(r=>portServer.listen(0,'127.0.0.1',r));const port=portServer.address().port;await new Promise(r=>portServer.close(r));driverBase='http://127.0.0.1:'+port;
  const profile=path.join(root,'profile');fs.mkdirSync(profile);driver=spawn(process.env.UDM_GECKODRIVER,['--port',String(port),'--profile-root',root],{windowsHide:true,env:{...process.env,UDM_INSTANCE_TAG:tag},stdio:['ignore','pipe','pipe']});const log=fs.createWriteStream(path.join(root,'driver.log'));driver.stdout.pipe(log);driver.stderr.pipe(log);
  await until(async()=>{try{return await driverRequest('GET','/status');}catch{return false;}});
  const created=await driverRequest('POST','/session',{capabilities:{alwaysMatch:{'moz:firefoxOptions':{binary:process.env.UDM_FIREFOX,args:['-headless','-profile',profile],prefs:{'browser.shell.checkDefaultBrowser':false,'media.autoplay.default':0,'extensions.webextensions.warnings-as-errors':false}}}}});driverSession=created.sessionId;
  assert.equal(await command('/moz/addon/install',{path:ext,temporary:true}),'udm@local.example');const info=await background({action:'diagnostics'});assert.equal(info.version,'0.42.0');assert.equal(path.resolve(info.dataDirectory).toLowerCase(),data.toLowerCase());assert.equal(info.downloads,0);assert.equal((await background({action:'version'})).version,'0.34.0');pass('Firefox extension 0.34.0 connects only to isolated native 0.42.0');
  if(process.env.UDM_EXTERNAL_TREE_CASE)await choose(false,process.env.UDM_EXTERNAL_TREE_CASE);else{for(const mode of ['whole','ranged','mixed-whole','mixed-ranged']){await choose(false,mode);await choose(true,mode);}for(const mode of ['ranged-child-ignored','ranged-child-etag','ranged-child-412','ranged-child-redirect','ranged-child-identity','native-change'])await choose(false,mode);}
 }else{
 browser=await chromium.launchPersistentContext(path.join(root,'profile'),{channel:process.env.UDM_TEST_BROWSER||'msedge',headless:true,ignoreDefaultArgs:['--disable-extensions'],env:{...process.env,UDM_INSTANCE_TAG:tag},viewport:{width:1100,height:850},args:['--enable-unsafe-extension-debugging','--autoplay-policy=no-user-gesture-required']});
 const cdp=await browser.browser().newBrowserCDPSession();await cdp.send('Extensions.loadUnpacked',{path:ext});worker=browser.serviceWorkers()[0]||await browser.waitForEvent('serviceworker');
 const info=await until(async()=>{try{return await worker.evaluate(()=>nativeRequest({action:'diagnostics'}));}catch{return null;}});assert.equal(info.version,'0.42.0');assert.equal(path.resolve(info.dataDirectory).toLowerCase(),data.toLowerCase());assert.equal(info.downloads,0);assert.equal(await worker.evaluate(()=>chrome.runtime.getManifest().version),'0.34.0');pass('Extension 0.34.0 connects to isolated native 0.42.0');
 page=browser.pages()[0]||await browser.newPage();const errors=[];page.on('pageerror',e=>errors.push(e.message));if(process.env.UDM_EXTERNAL_TREE_CASE)await choose(false,process.env.UDM_EXTERNAL_TREE_CASE);else{for(const mode of ['whole','ranged','mixed-whole','mixed-ranged']){await choose(false,mode);await choose(true,mode);}for(const mode of ['ranged-child-ignored','ranged-child-etag','ranged-child-412','ranged-child-redirect','ranged-child-identity','native-change'])await choose(false,mode);}assert.deepEqual(errors,[]);pass('Indexed media pages have no script errors');
 }
})().catch(async e=>{if(page){try{const session=await browser.newCDPSession(page);const tree=await session.send('Accessibility.getFullAXTree');fs.writeFileSync(path.join(root,'failure-ui.json'),JSON.stringify(tree,null,2));await page.screenshot({path:path.join(root,'failure.png')});console.error('FIXTURE PAGE',await page.evaluate(()=>({url:location.href,ready:document.body.dataset.ready,video:{height:document.querySelector('video')?.videoHeight,error:document.querySelector('video')?.error?.message}})));}catch{}}console.error(e);results.push({passed:false,error:e.stack});process.exitCode=1;}).finally(async()=>{if(browser)await browser.close();if(driverSession)try{await driverRequest('DELETE','/session/'+driverSession);}catch{}if(driver)driver.kill();if(desktop)desktop.kill();for(const host of [server,indexServer])if(host){host.closeAllConnections();await new Promise(r=>host.close(r));}fs.mkdirSync(root,{recursive:true});fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({scope:'Real isolated browser/native app; synthetic clear recorded external hierarchical/mixed-index DASH with separate index/media origins. Chrome/Edge use the panel UI; Firefox uses a temporary observer invoking its real page-scoped handoff.',browser:process.env.UDM_TEST_BROWSER||'msedge',passed:results.filter(x=>x.passed).length,failed:results.filter(x=>!x.passed).length,results,requests},null,2));});
