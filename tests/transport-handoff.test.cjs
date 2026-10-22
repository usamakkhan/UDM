'use strict';
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
const base=process.env.UDM_BROWSER_SOURCE||path.resolve(__dirname,'../browser/chromium');
const id='Q3TI27IN7X0',url='https://www.youtube.com/watch?v='+id;
const direct=(itag,version='new')=>'https://rr1.googlevideo.com/videoplayback?id=content&itag='+itag+'&expire=4102444800&sig='+version;
function snapshot(version){return {videoId:id,visitorData:"fixture-visitor",durationMs:120000,formats:[{id:'137',height:1080,mime:'video/mp4; codecs="avc1"',lastModified:'123456789',url:version?direct(137,version):''},{id:'140',mime:'audio/mp4; codecs="mp4a"',lastModified:'123456790',url:version?direct(140,version):''}]};}
function harness(snapshots,documents=['doc','doc'],nativeReply=null,sessionAvailable=true){
 const calls=[],lookups=[],events={};let reads=0,sessionReads=0,prepared=false;const event=name=>({addListener:f=>events[name]=f});
 const api={runtime:{id:'fixture',onInstalled:event('installed'),onMessage:event('message'),sendNativeMessage:async(_,message)=>{if(message.action==='youtube-player'){lookups.push(message);return typeof nativeReply==='function'?nativeReply(message):nativeReply||{ok:false};}calls.push(message);return {ok:true,id:'fixture-job'};}},storage:{local:{get:async()=>({}),set:async()=>{}},session:{get:async()=>({}),remove:async()=>{},set:async()=>{}}},tabs:{get:async()=>({url,incognito:false}),onRemoved:event('remove'),onUpdated:event('update')},scripting:{executeScript:async args=>{
   if(args.func.name==='readYouTubeFormats'){const index=Math.min(reads++,snapshots.length-1);return [{frameId:0,documentId:documents[Math.min(index,documents.length-1)],result:snapshots[index]}];}
   if(args.args?.length===2){prepared=true;return [];}
   sessionReads++;if(typeof sessionAvailable==='function')await sessionAvailable();return [{frameId:0,documentId:'doc',result:(sessionAvailable==='after-prepare'?prepared:sessionAvailable)?{videoId:id,url:'https://rr1.googlevideo.com/videoplayback?sabr=1',body:'fixture',capturedAt:Date.now()}:null}];
 }},action:{setBadgeText:async()=>{}},contextMenus:{onClicked:event('context')},downloads:{onCreated:event('download')},webRequest:{onHeadersReceived:event('headers')}};
 const context=vm.createContext({chrome:api,URL,URLSearchParams,navigator:{userAgent:'fixture'},console,setTimeout,clearTimeout});
 for(const name of ['file-recognition.js','media.js','formats.js','background.js'])vm.runInContext(fs.readFileSync(path.join(base,name),'utf8'),context);
 return {calls,lookups,context,api,get sessionReads(){return sessionReads;},run:()=>context.mediaHandoff({url,tabId:7,height:1080,formatKey:'137'},{})};
}
let passed=0;const check=name=>{passed++;console.log('PASS '+name);};
(async()=>{
 // Gates prove overlap without depending on wall-clock performance.
 let parallel=harness([snapshot('new')]);let releaseTab,releaseSettings;
 parallel.api.tabs.get=()=>new Promise(resolve=>releaseTab=()=>resolve({url,incognito:false}));
 parallel.api.storage.local.get=()=>new Promise(resolve=>releaseSettings=()=>resolve({}));
 let selection=parallel.context.availableMedia({url,tabId:7},{});
 await new Promise(resolve=>setImmediate(resolve));
 assert.equal(typeof releaseTab,'function');assert.equal(typeof releaseSettings,'function');
 // Later current-tab validation must remain enabled and finish normally.
 parallel.api.tabs.get=async()=>({url,incognito:false});releaseTab();releaseSettings();await selection;
 check('Tab identity and preferences are read concurrently without removing final tab validation');
 parallel=harness([snapshot('new')]);let releaseFormats,releaseObserved;
 const readFormats=parallel.api.scripting.executeScript;
 parallel.api.scripting.executeScript=args=>new Promise(resolve=>releaseFormats=async()=>resolve(await readFormats(args)));
 parallel.api.storage.session.get=()=>new Promise(resolve=>releaseObserved=()=>resolve({}));
 selection=parallel.context.availableMedia({url,tabId:7},{});await new Promise(resolve=>setImmediate(resolve));
 assert.equal(typeof releaseFormats,'function');assert.equal(typeof releaseObserved,'function');
 releaseObserved();await releaseFormats();await selection;
 check('Page formats and captured URL storage are read concurrently');
 parallel=harness([snapshot('new')]);let injected=false;
 parallel.api.storage.local.get=async()=>({settings:{panelEnabled:false}});
 parallel.api.scripting.executeScript=async()=>{injected=true;return [];};
 await assert.rejects(parallel.context.availableMedia({url,tabId:7},{}),/disabled/);assert.equal(injected,false);
 check('Disabled panel policy still prevents page injection');
 const sized=()=>{const s=snapshot('new');s.formats[0].contentLength=1000;s.formats[1].contentLength=200;return s;};
 let h=harness([sized(),sized()]);await h.run();assert.equal(h.calls[0].reportedSize,1200);check('Selected video and audio sizes reach the desktop before download');

 assert.equal(h.calls[0].estimatedSize,undefined);check('Reported lengths are not presented as bitrate estimates');
 const nested=sized();nested.formats.forEach(f=>{f.transcodeMetadata={contentLength:String(f.contentLength)};delete f.contentLength;});h=harness([nested,nested]);await h.run();assert.equal(h.calls[0].reportedSize,1200);check('Nested transcode metadata supplies reported lengths');
 const query=sized();query.formats.forEach(f=>{f.url+='&clen='+f.contentLength;f.contentLength=-1;});h=harness([query,query]);await h.run();assert.equal(h.calls[0].reportedSize,1200);check('Invalid metadata falls back to the selected URL clen');
 const bitrate=snapshot('new');bitrate.formats.forEach(f=>f.bitrate=1000000);h=harness([bitrate,bitrate]);await h.run();assert.equal(h.calls[0].reportedSize,0);check('Bitrate and duration cannot become exact bytes');
 const streaming=sized();streaming.formats.forEach(f=>f.url='');h=harness([streaming,streaming]);await h.run();assert.equal(h.calls[0].action,'sabr');assert.equal(h.calls[0].reportedSize,1200);check('Streaming sizes do not require direct audio URLs');
 const streamingSized=bytes=>{const s=snapshot(null);if(bytes){s.formats[0].contentLength=bytes;s.formats[1].contentLength=200;}return s;};
 for(const [label,first,last,expected] of [
  ['newly available',0,1000,1200],['updated',1000,2000,2200],['removed',1000,0,0]
 ]){
  h=harness([streamingSized(first),streamingSized(last)]);await h.run();
  assert.equal(h.calls[0].action,'sabr');assert.equal(h.calls[0].reportedSize,expected);assert.equal(h.sessionReads,1);
  check('Final streaming metadata uses '+label+' reported lengths without another session read');
 }
 const partial=sized();delete partial.formats[1].contentLength;h=harness([partial,partial]);await h.run();assert.equal(h.calls[0].reportedSize,0);check('Missing audio size does not underreport the combined file');
 h=harness([sized(),snapshot('new')]);await h.run();assert.equal(h.calls[0].reportedSize,0);check('Fresh metadata without size does not reuse a stale size hint');
 for(const [name,change] of [
  ['video revision',s=>s.formats[0].lastModified='987654321'],
  ['audio revision',s=>s.formats[1].lastModified='987654322'],
  ['audio language',s=>s.formats[1].xtags='lang=fr'],
  ['duration',s=>s.durationMs+=1000]
 ]){
  const changed=snapshot(null);change(changed);h=harness([snapshot(null),changed]);
  await assert.rejects(h.run(),/stream.*changed/i);assert.equal(h.calls.length,0);
  check('Changed '+name+' cannot submit a stale streaming selection');
 }
 h=harness([snapshot(null),snapshot('new')]);await h.run();assert.equal(h.calls[0].action,'media');assert.equal(h.calls[0].videoUrl,direct(137));assert.equal(h.calls[0].sabr,null);check('A direct pair captured before final confirmation replaces the earlier SABR candidate');
 h=harness([snapshot('old'),snapshot('new')]);await h.run();assert.equal(h.calls[0].videoUrl,direct(137));assert.equal(h.calls[0].audioUrl,direct(140));check('Final handoff uses the freshly confirmed video and audio URLs together');
 h=harness([snapshot('old'),snapshot(null)]);await assert.rejects(h.run(),/direct.*changed|no longer/i);assert.equal(h.calls.length,0);check('A disappeared direct pair does not send stale URLs or silently change transport');
 h=harness([snapshot('new'),snapshot('new')]);await h.run();assert.equal(h.calls[0].action,'media');assert.equal(h.sessionReads,0);check('Available direct links bypass all SABR-session lookups');
 h=harness([snapshot(null),snapshot(null)]);await h.run();assert.equal(h.calls[0].action,'sabr');check('When no direct pair exists the existing explicit SABR handoff remains available');
 h=harness([snapshot('new'),snapshot('new')],['old-document','new-document']);await assert.rejects(h.run(),/changed before handoff/);assert.equal(h.calls.length,0);check('Navigation between capture and handoff creates no download');
 const timedSnapshot=origin=>({...snapshot('new'),timeOrigin:origin});
 h=harness([timedSnapshot(1000),timedSnapshot(2000)],['','']);await assert.rejects(h.run(),/changed before handoff/);assert.equal(h.calls.length,0);check('A reload without document IDs cannot hand off stale video URLs');
 h=harness([timedSnapshot(1000),timedSnapshot(1000)],['','']);await h.run();assert.equal(h.calls[0].action,'media');check('Stable timeOrigin permits video handoff without document IDs');
 h=harness([snapshot('new'),snapshot('new')],['','']);await assert.rejects(h.run(),/changed before handoff/);assert.equal(h.calls.length,0);check('Missing document IDs and timeOrigin cannot prove video ownership');
 h=harness([snapshot('new'),{...snapshot('new'),videoId:'aaaaaaaaaaa'}]);await assert.rejects(h.run());assert.equal(h.calls.length,0);check('An advertisement or changed video cannot supply the final transport');

 const nativePair=()=>({ok:true,videoId:id,formatId:'137',height:1080,pixelHeight:1080,userAgent:'Mozilla/5.0 (Macintosh; Intel Mac OS X 15_7_3) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/26.0 Safari/605.1.15',validatedAt:Date.now(),transport:'player-direct',video:{url:direct(137)+'&clen=1000',itag:137,size:1000,kind:'video'},audio:{url:direct(140)+'&clen=200',itag:140,size:200,kind:'audio'}});
 h=harness([snapshot(null),snapshot(null)],['doc','doc'],nativePair());await h.run();assert.equal(h.calls[0].action,'media');assert.equal(h.calls[0].videoUrl,nativePair().video.url);assert.equal(h.calls[0].reportedSize,1200);assert.equal(h.calls[0].userAgent,nativePair().userAgent);assert.equal(h.sessionReads,1);check('Verified native pair uses parallel-file handoff with its matching client user agent');
 assert.equal(h.lookups[0].videoId,id);assert.equal(h.lookups[0].formatId,'137');assert.equal(h.lookups[0].height,1080);assert.equal(h.lookups[0].cookies,undefined);assert.equal(h.lookups[0].visitorData,'fixture-visitor');check('Native request contains only current video selection and bounded metadata');
 await h.run();assert.equal(h.lookups.length,1);check('Repeated selection reuses a short-lived native pair for the same document');
 for(const [name,alter] of [
  ['wrong video',p=>p.videoId='aaaaaaaaaaa'],['wrong format',p=>p.formatId='136'],['wrong height',p=>p.height=720],['stale probe',p=>p.validatedAt=Date.now()-120000],['untrusted host',p=>p.video.url='https://googlevideo.com.evil.test/videoplayback'],['SABR endpoint',p=>p.video.url+='&sabr=1'],['n transform',p=>p.video.url+='&n=untransformed'],['mismatched itag',p=>p.video.itag=136],['header injection',p=>p.userAgent='fake\r\nInjected: true']
 ]){const pair=nativePair();alter(pair);h=harness([snapshot(null),snapshot(null)],['doc','doc'],pair);await h.run();assert.equal(h.calls[0].action,'sabr');check('Invalid native '+name+' preserves browser fallback');}
 h=harness([snapshot(null),snapshot(null)],['old','new'],nativePair());await assert.rejects(h.run(),/changed before handoff/);assert.equal(h.calls.length,0);check('Native retrieval cannot hand a previous document to the downloader');
 h=harness([snapshot(null),snapshot('fresh-browser')],['doc','doc'],nativePair());await h.run();assert.equal(h.calls[0].videoUrl,direct(137,'fresh-browser'));assert.equal(h.calls[0].userAgent,'fixture');check('Fresh browser pair replaces retrieval candidate with matching browser headers');
 h=harness([snapshot(null),snapshot(null)],['doc','doc'],async()=>{throw Error('host unavailable');});await h.run();assert.equal(h.calls[0].action,'sabr');check('Older or unavailable native host keeps existing playback capture usable');
 h=harness([snapshot(null),snapshot(null)],['doc','doc'],nativePair());await h.run();h.context.clearPlayerPairs(7);await h.run();assert.equal(h.lookups.length,2);check('Tab navigation invalidates cached native links');

 h=harness([snapshot(null),snapshot(null)],['doc','doc'],async()=>{await new Promise(resolve=>setTimeout(resolve,150));return nativePair();});await h.run();assert.equal(h.calls[0].action,'sabr');assert.equal(h.sessionReads,1);assert.equal(h.calls.length,1);check('A ready streaming session does not wait for a delayed direct pair');
 let finishPlayer;h=harness([snapshot(null),snapshot(null)],['doc','doc'],()=>new Promise(resolve=>finishPlayer=resolve));const began=Date.now();await h.run();assert.equal(h.calls[0].action,'sabr');assert.ok(Date.now()-began<250);finishPlayer(nativePair());await new Promise(resolve=>setTimeout(resolve,0));assert.equal(h.calls.length,1);check('A ready browser stream avoids the ten-second native lookup without a duplicate late handoff');
 h=harness([snapshot(null),snapshot(null)],['old','new'],async()=>{await new Promise(resolve=>setTimeout(resolve,20));return nativePair();});await assert.rejects(h.run(),/changed before handoff/);assert.equal(h.calls.length,0);check('Navigation during a pending retrieval prevents stale downloads');
 h=harness([snapshot(null),snapshot(null)],['doc','doc'],{ok:false,diagnostics:{phase:'audio-range',timeout:true,visitorData:'SECRET',url:'SECRET'}});await h.run();let diagnostic=h.context.playerDiagnostics(7);assert.equal(diagnostic.phase,'audio-range');assert.equal(diagnostic.timeout,true);assert.equal(diagnostic.visitorPresent,true);assert.ok(!JSON.stringify(diagnostic).includes('SECRET'));check('Player diagnostics expose failure phase without visitor tokens or signed links');
 h.context.clearPlayerPairs(7);assert.equal(h.context.playerDiagnostics(7),null);check('Navigating away clears in-memory player diagnostics');
 h=harness([snapshot(null),snapshot(null)],['doc','doc'],async()=>{await new Promise(resolve=>setTimeout(resolve,2300));return nativePair();},false);await h.run();assert.equal(h.calls[0].action,'media');assert.equal(h.calls.length,1);check('Without a captured stream the pending direct lookup is still awaited');
 h=harness([snapshot(null),snapshot(null)],['doc','doc'],{ok:false},'after-prepare');const preparationWaits=[];
 h.context.setTimeout=(callback,delay)=>{preparationWaits.push(delay);return setTimeout(callback,delay);};
 await h.run();assert.equal(h.calls[0].action,'sabr');assert.equal(h.calls.length,1);assert.ok(!preparationWaits.includes(400));
 check('A session ready after preparation is handed off without an artificial polling delay');
 let releaseCold;h=harness([snapshot(null),snapshot(null)],['doc','doc'],()=>new Promise(resolve=>releaseCold=resolve),'after-prepare');const coldBegan=Date.now();await h.run();assert.equal(h.calls[0].action,'sabr');assert.ok(Date.now()-coldBegan<1800);releaseCold(nativePair());await new Promise(resolve=>setTimeout(resolve,0));assert.equal(h.calls.length,1);check('Cold playback preparation runs before the pending native lookup completes, without duplicate submission');
 h=harness([snapshot(null),snapshot(null)],['old','new'],{ok:false},'after-prepare');await assert.rejects(h.run(),/document changed/);assert.equal(h.calls.length,0);check('Navigation during cold preparation rejects the new document');
 h=harness([{...snapshot(null),timeOrigin:1000},{...snapshot(null),timeOrigin:2000}],['',''],{ok:false},'after-prepare');await assert.rejects(h.run(),/document changed/);assert.equal(h.calls.length,0);check('Cold preparation rejects a reload even when document IDs are absent');

 let releaseCapture,releaseLookup;
 h=harness([snapshot(null),snapshot(null)],['doc','doc'],()=>new Promise(resolve=>releaseLookup=resolve),()=>new Promise(resolve=>releaseCapture=resolve));
 const overlapping=h.run();await new Promise(resolve=>setTimeout(resolve,30));
 assert.equal(h.sessionReads,1);assert.equal(h.calls.length,0);assert.equal(h.lookups.length,1);
 releaseCapture();await overlapping;assert.equal(h.calls[0].action,'sabr');releaseLookup(nativePair());await new Promise(resolve=>setTimeout(resolve,0));assert.equal(h.calls.length,1);
 check('Capture read starts while native lookup is pending and only one download is submitted');
 h=harness([snapshot(null),snapshot(null)],['doc','doc'],nativePair(),async()=>{throw Error('capture unavailable');});await h.run();assert.equal(h.calls[0].action,'media');assert.equal(h.calls.length,1);check('Speculative capture failure cannot reject a verified direct pair');
 h=harness([snapshot(null),snapshot(null)],['doc','doc'],{ok:false},async()=>{throw Error('capture unavailable');});await assert.rejects(h.run(),/capture unavailable/);assert.equal(h.calls.length,0);check('Capture failure remains visible when no direct pair exists');
 h=harness([snapshot(null),snapshot(null)],['doc','doc'],nativePair(),()=>new Promise(resolve=>releaseCapture=resolve));await h.run();assert.equal(h.calls[0].action,'media');releaseCapture();await new Promise(resolve=>setTimeout(resolve,0));assert.equal(h.calls.length,1);check('A stalled capture read does not delay direct handoff or submit a late duplicate');
 h=harness([snapshot(null),snapshot(null)],['doc','doc'],async()=>{await new Promise(resolve=>setTimeout(resolve,500));return nativePair();},()=>new Promise(resolve=>releaseCapture=resolve));
 let rescueTimer;try{await Promise.race([h.run(),new Promise((_,reject)=>{rescueTimer=setTimeout(()=>reject(Error('Late direct pair remained blocked by capture')),2000);})]);}finally{clearTimeout(rescueTimer);releaseCapture?.();}
 assert.equal(h.calls[0].action,'media');assert.equal(h.calls[0].reportedSize,1200);await new Promise(resolve=>setTimeout(resolve,0));assert.equal(h.calls.length,1);check('Native pair arriving after grace rescues a stalled capture without duplicate submission');
 h=harness([snapshot(null),snapshot(null)],['doc','doc'],{ok:false},async()=>{await new Promise(resolve=>setTimeout(resolve,50));});await h.run();assert.equal(h.calls[0].action,'sabr');check('Unavailable native pair does not mask a later valid browser capture');
 // A ready direct pair must wake a held retry timer, not wait for its tick.
 let retryStarted,releaseRetryPair,retryCallback,retryCleared=false;
 const retryGate=new Promise(resolve=>retryStarted=resolve);
 h=harness([snapshot(null),snapshot(null)],['doc','doc'],()=>new Promise(resolve=>releaseRetryPair=resolve),false);
 const retryHandle={};
 h.context.setTimeout=(callback,delay)=>{if(delay===400){retryCallback=callback;retryStarted();return retryHandle;}return setTimeout(callback,delay);};
 h.context.clearTimeout=handle=>{if(handle===retryHandle)retryCleared=true;else clearTimeout(handle);};
 const retryRun=h.run();await retryGate;releaseRetryPair(nativePair());await new Promise(resolve=>setImmediate(resolve));
 const wokeBeforeTick=h.calls.length===1;
 if(!wokeBeforeTick)retryCallback();await retryRun;
 assert.ok(wokeBeforeTick,'A ready direct pair still waits for the 400 ms polling tick');assert.ok(retryCleared);assert.equal(h.calls[0].action,'media');assert.equal(h.calls.length,1);
 check('A direct pair arriving during playback retry cancels the wait and submits once after validation');
 // Hold the final metadata response, then fire its watchdog explicitly.
 h=harness([snapshot('new'),snapshot('new')]);
 let finalReadStarted,releaseFinal;
 const finalReadGate=new Promise(resolve=>finalReadStarted=resolve);
 const originalRead=h.api.scripting.executeScript;let metadataReads=0;
 h.api.scripting.executeScript=args=>{
  if(args.func.name==='readYouTubeFormats'&&++metadataReads===2){
   finalReadStarted();return new Promise(resolve=>releaseFinal=async()=>resolve(await originalRead(args)));
  }
  return originalRead(args);
 };
 const activeWatchdogs=new Set();
 h.context.setTimeout=(callback,delay)=>{
  if(delay===12000){const handle={callback};activeWatchdogs.add(handle);return handle;}
  return setTimeout(callback,delay);
 };
 h.context.clearTimeout=handle=>{if(activeWatchdogs.has(handle))activeWatchdogs.delete(handle);else clearTimeout(handle);};
 const timedHandoff=h.run();const expectedTimeout=assert.rejects(timedHandoff,/timed out|respond|reading|refresh/i);
 await finalReadGate;assert.equal(activeWatchdogs.size,1);
 [...activeWatchdogs][0].callback();await expectedTimeout;
 assert.equal(h.calls.length,0);await releaseFinal();await new Promise(resolve=>setImmediate(resolve));
 assert.equal(h.calls.length,0);assert.equal(activeWatchdogs.size,0);
 check('A timed-out final metadata read cannot submit a late download after its browser reply arrives');
 console.log('ALL '+passed+' TRANSPORT HANDOFF CHECKS PASSED');
})().catch(error=>{console.error(error.message);process.exitCode=1;});
