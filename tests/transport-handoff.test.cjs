'use strict';
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
const base=process.env.UDM_BROWSER_SOURCE||path.resolve(__dirname,'../browser/chromium');
const id='Q3TI27IN7X0',url='https://www.youtube.com/watch?v='+id;
const direct=(itag,version='new')=>'https://rr1.googlevideo.com/videoplayback?id=content&itag='+itag+'&expire=4102444800&sig='+version;
function snapshot(version){return {videoId:id,visitorData:"fixture-visitor",durationMs:120000,formats:[{id:'137',height:1080,mime:'video/mp4; codecs="avc1"',lastModified:'123456789',url:version?direct(137,version):''},{id:'140',mime:'audio/mp4; codecs="mp4a"',lastModified:'123456790',url:version?direct(140,version):''}]};}
function harness(snapshots,documents=['doc','doc'],nativeReply=null){
 const calls=[],lookups=[],events={};let reads=0,sessionReads=0;const event=name=>({addListener:f=>events[name]=f});
 const api={runtime:{id:'fixture',onInstalled:event('installed'),onMessage:event('message'),sendNativeMessage:async(_,message)=>{if(message.action==='youtube-player'){lookups.push(message);return typeof nativeReply==='function'?nativeReply(message):nativeReply||{ok:false};}calls.push(message);return {ok:true,id:'fixture-job'};}},storage:{local:{get:async()=>({}),set:async()=>{}},session:{get:async()=>({}),remove:async()=>{},set:async()=>{}}},tabs:{get:async()=>({url,incognito:false}),onRemoved:event('remove'),onUpdated:event('update')},scripting:{executeScript:async args=>{
   if(args.func.name==='readYouTubeFormats'){const index=Math.min(reads++,snapshots.length-1);return [{frameId:0,documentId:documents[Math.min(index,documents.length-1)],result:snapshots[index]}];}
   sessionReads++;return [{frameId:0,documentId:'doc',result:{videoId:id,url:'https://rr1.googlevideo.com/videoplayback?sabr=1',body:'fixture',capturedAt:Date.now()}}];
 }},action:{setBadgeText:async()=>{}},contextMenus:{onClicked:event('context')},downloads:{onCreated:event('download')},webRequest:{onHeadersReceived:event('headers')}};
 const context=vm.createContext({chrome:api,URL,URLSearchParams,navigator:{userAgent:'fixture'},console,setTimeout,clearTimeout});
 for(const name of ['media.js','formats.js','background.js'])vm.runInContext(fs.readFileSync(path.join(base,name),'utf8'),context);
 return {calls,lookups,context,get sessionReads(){return sessionReads;},run:()=>context.mediaHandoff({url,tabId:7,height:1080,formatKey:'137'},{})};
}
let passed=0;const check=name=>{passed++;console.log('PASS '+name);};
(async()=>{
 let h=harness([snapshot(null),snapshot('new')]);await h.run();assert.equal(h.calls[0].action,'media');assert.equal(h.calls[0].videoUrl,direct(137));assert.equal(h.calls[0].sabr,null);check('A direct pair captured before final confirmation replaces the earlier SABR candidate');
 h=harness([snapshot('old'),snapshot('new')]);await h.run();assert.equal(h.calls[0].videoUrl,direct(137));assert.equal(h.calls[0].audioUrl,direct(140));check('Final handoff uses the freshly confirmed video and audio URLs together');
 h=harness([snapshot('old'),snapshot(null)]);await assert.rejects(h.run(),/direct.*changed|no longer/i);assert.equal(h.calls.length,0);check('A disappeared direct pair does not send stale URLs or silently change transport');
 h=harness([snapshot('new'),snapshot('new')]);await h.run();assert.equal(h.calls[0].action,'media');assert.equal(h.sessionReads,0);check('Available direct links bypass all SABR-session lookups');
 h=harness([snapshot(null),snapshot(null)]);await h.run();assert.equal(h.calls[0].action,'sabr');check('When no direct pair exists the existing explicit SABR handoff remains available');
 h=harness([snapshot('new'),snapshot('new')],['old-document','new-document']);await assert.rejects(h.run(),/changed before handoff/);assert.equal(h.calls.length,0);check('Navigation between capture and handoff creates no download');
 h=harness([snapshot('new'),{...snapshot('new'),videoId:'aaaaaaaaaaa'}]);await assert.rejects(h.run());assert.equal(h.calls.length,0);check('An advertisement or changed video cannot supply the final transport');

 const nativePair=()=>({ok:true,videoId:id,formatId:'137',height:1080,pixelHeight:1080,userAgent:'Mozilla/5.0 (Macintosh; Intel Mac OS X 15_7_3) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/26.0 Safari/605.1.15',validatedAt:Date.now(),transport:'player-direct',video:{url:direct(137)+'&clen=1000',itag:137,size:1000,kind:'video'},audio:{url:direct(140)+'&clen=200',itag:140,size:200,kind:'audio'}});
 h=harness([snapshot(null),snapshot(null)],['doc','doc'],nativePair());await h.run();assert.equal(h.calls[0].action,'media');assert.equal(h.calls[0].videoUrl,nativePair().video.url);assert.equal(h.calls[0].userAgent,nativePair().userAgent);assert.equal(h.sessionReads,0);check('Verified native pair uses parallel-file handoff with its matching client user agent');
 assert.equal(h.lookups[0].videoId,id);assert.equal(h.lookups[0].formatId,'137');assert.equal(h.lookups[0].height,1080);assert.equal(h.lookups[0].cookies,undefined);assert.equal(h.lookups[0].visitorData,'fixture-visitor');check('Native request contains only current video selection and bounded metadata');
 await h.run();assert.equal(h.lookups.length,1);check('Repeated selection reuses a short-lived native pair for the same document');
 for(const [name,alter] of [
  ['wrong video',p=>p.videoId='aaaaaaaaaaa'],['wrong format',p=>p.formatId='136'],['wrong height',p=>p.height=720],['stale probe',p=>p.validatedAt=Date.now()-120000],['untrusted host',p=>p.video.url='https://googlevideo.com.evil.test/videoplayback'],['SABR endpoint',p=>p.video.url+='&sabr=1'],['n transform',p=>p.video.url+='&n=untransformed'],['mismatched itag',p=>p.video.itag=136],['header injection',p=>p.userAgent='fake\r\nInjected: true']
 ]){const pair=nativePair();alter(pair);h=harness([snapshot(null),snapshot(null)],['doc','doc'],pair);await h.run();assert.equal(h.calls[0].action,'sabr');check('Invalid native '+name+' preserves browser fallback');}
 h=harness([snapshot(null),snapshot(null)],['old','new'],nativePair());await assert.rejects(h.run(),/changed before handoff/);assert.equal(h.calls.length,0);check('Native retrieval cannot hand a previous document to the downloader');
 h=harness([snapshot(null),snapshot('fresh-browser')],['doc','doc'],nativePair());await h.run();assert.equal(h.calls[0].videoUrl,direct(137,'fresh-browser'));assert.equal(h.calls[0].userAgent,'fixture');check('Fresh browser pair replaces retrieval candidate with matching browser headers');
 h=harness([snapshot(null),snapshot(null)],['doc','doc'],async()=>{throw Error('host unavailable');});await h.run();assert.equal(h.calls[0].action,'sabr');check('Older or unavailable native host keeps existing playback capture usable');
 h=harness([snapshot(null),snapshot(null)],['doc','doc'],nativePair());await h.run();h.context.clearPlayerPairs(7);await h.run();assert.equal(h.lookups.length,2);check('Tab navigation invalidates cached native links');

 h=harness([snapshot(null),snapshot(null)],['doc','doc'],async()=>{await new Promise(resolve=>setTimeout(resolve,1500));return nativePair();});await h.run();assert.equal(h.calls[0].action,'media');assert.equal(h.sessionReads,0);assert.equal(h.calls.length,1);check('A direct pair arriving after 1.2 seconds wins before any fallback handoff');
 let finishPlayer;h=harness([snapshot(null),snapshot(null)],['doc','doc'],()=>new Promise(resolve=>finishPlayer=resolve));const began=Date.now();await h.run();assert.equal(h.calls[0].action,'sabr');assert.ok(Date.now()-began>=9500&&Date.now()-began<12000);finishPlayer(nativePair());await new Promise(resolve=>setTimeout(resolve,0));assert.equal(h.calls.length,1);check('An unresponsive host falls back at the bounded deadline without a duplicate late handoff');
 h=harness([snapshot(null),snapshot(null)],['old','new'],async()=>{await new Promise(resolve=>setTimeout(resolve,20));return nativePair();});await assert.rejects(h.run(),/changed before handoff/);assert.equal(h.calls.length,0);check('Navigation during a pending retrieval prevents stale downloads');
 h=harness([snapshot(null),snapshot(null)],['doc','doc'],{ok:false,diagnostics:{phase:'audio-range',timeout:true,visitorData:'SECRET',url:'SECRET'}});await h.run();let diagnostic=h.context.playerDiagnostics(7);assert.equal(diagnostic.phase,'audio-range');assert.equal(diagnostic.timeout,true);assert.equal(diagnostic.visitorPresent,true);assert.ok(!JSON.stringify(diagnostic).includes('SECRET'));check('Player diagnostics expose failure phase without visitor tokens or signed links');
 h.context.clearPlayerPairs(7);assert.equal(h.context.playerDiagnostics(7),null);check('Navigating away clears in-memory player diagnostics');
 console.log('ALL '+passed+' TRANSPORT HANDOFF CHECKS PASSED');
})().catch(error=>{console.error(error.message);process.exitCode=1;});
