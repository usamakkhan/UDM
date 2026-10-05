'use strict';
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
const base=process.env.UDM_BROWSER_SOURCE||path.resolve(__dirname,'../browser/chromium');
const {UdmFormats,readYouTubeFormats}=require(path.join(base,'formats.js'));
const id='Q3TI27IN7X0',url='https://www.youtube.com/watch?v='+id;
const stream=(language='en',signature='fresh')=>'https://rr1.googlevideo.com/videoplayback?id=content&itag=140&expire=4102444800&xtags='+language+'&sig='+signature;
const audio=(language='en',signature='fresh')=>({id:'140',mime:'audio/mp4; codecs="mp4a.40.2"',audioTrackId:language+'.1',audioName:language==='en'?'English':'Español',audioDefault:language==='en',bitrate:128000,xtags:language,url:stream(language,signature)});
const snapshot=(formats=[audio(),audio('es')])=>({videoId:id,timeOrigin:1000,formats});
let passed=0;function check(name){passed++;console.log('PASS '+name);}
function harness(snapshots=[snapshot(),snapshot()],documents=['doc','doc']){
 const calls=[],events={},store={},session={};let reads=0;
 const event=name=>({addListener:f=>events[name]=f});
 const api={runtime:{id:'fixture',onInstalled:event('installed'),onMessage:event('message'),sendNativeMessage:async(_,message)=>{calls.push(message);return {ok:true,id:'audio-job'};}},storage:{local:{get:async()=>store,set:async()=>{}},session:{get:async()=>session,remove:async()=>{},set:async()=>{}}},tabs:{get:async()=>({url,incognito:false,title:'Audio test'}),onRemoved:event('remove'),onUpdated:event('update')},scripting:{executeScript:async args=>{assert.equal(args.func.name,'readYouTubeFormats','Audio must not request a video/SABR session');const n=Math.min(reads++,snapshots.length-1);return [{frameId:0,documentId:documents[n],result:snapshots[n]}];}},action:{setBadgeText:async()=>{}},contextMenus:{onClicked:event('context')},downloads:{onCreated:event('download')},webRequest:{onHeadersReceived:event('headers')}};
 const context=vm.createContext({chrome:api,URL,URLSearchParams,navigator:{userAgent:'audio-browser'},console,setTimeout,clearTimeout});
 for(const name of ['file-recognition.js','media.js','formats.js','background.js'])vm.runInContext(fs.readFileSync(path.join(base,name),'utf8'),context);
 const message={action:'youtube-audio',url,tabId:7,audioKey:UdmFormats.audioChoices(snapshot(),id)[1].key,title:'A: title / test'};
 return {calls,api,store,session,context,message,run:(m=message,sender={})=>context.audioHandoff(m,sender),send:(m=message,sender={id:'fixture'})=>new Promise(resolve=>events.message(m,sender,resolve))};
}
(async()=>{
 const choices=UdmFormats.audioChoices(snapshot(),id);assert.equal(choices.length,2);assert.notEqual(choices[0].key,choices[1].key);assert.match(choices[1].label,/Español.*128 kbps.*M4A/);assert.equal(choices[0].default,true);check('Same-itag audio languages remain distinct and show codec and bitrate');
 assert.equal(UdmFormats.audioChoices({...snapshot(),live:true},id).length,0);assert.equal(UdmFormats.audioChoices(snapshot(),'aaaaaaaaaaa').length,0);check('Live and unrelated-video audio are absent');
 for(const [name,alter] of [
  ['muxed video',f=>f.muxed=true],['Opus/WebM',f=>f.mime='audio/webm; codecs="opus"'],['unknown codec',f=>f.mime='audio/mp4'],['wrong itag',f=>f.id='141'],
  ['missing resource',f=>f.url=f.url.replace('id=content&','')],['missing expiry',f=>f.url=f.url.replace('expire=4102444800&','')],['expired',f=>f.url=f.url.replace('4102444800','1')],['non-numeric expiry',f=>f.url=f.url.replace('4102444800','bad')],
  ['lookalike host',f=>f.url=f.url.replace('.googlevideo.com','.googlevideo.com.evil.test')],['embedded credentials',f=>f.url=f.url.replace('https://','https://user:pass@')],['custom port',f=>f.url=f.url.replace('.com/','.com:444/')],
  ['SABR',f=>f.url+='&sabr=1'],['UMP',f=>f.url+='&ump=1'],['sequence stream',f=>f.url+='&sq=2'],['untransformed n',f=>f.url+='&n=encrypted'],['signed byte range',f=>f.url+='&range=0-10&sparams=range']
 ]){const f=audio();alter(f);assert.equal(UdmFormats.audioChoices(snapshot([f]),id).length,0);check('Reject '+name+' as an audio-only file');}
 let f={...audio(),url:stream()+'&range=0-100&rn=4&rbuf=0'};let c=UdmFormats.audioChoices(snapshot([f]),id)[0];assert(!/[?&](range|rn|rbuf)=/.test(c.audioUrl));check('Unsigned playback byte limits are removed for a whole-file download');
 const encrypted={...audio('es'),url:'',matchUrl:stream('es')+'&n=encrypted'};
 let captured=UdmFormats.attachObserved(snapshot([encrypted]),[{itag:140,url:stream('en')+'&n=ready',observedAt:Date.now()}]);assert.equal(UdmFormats.audioChoices(captured,id).length,0);check('Captured default audio cannot replace another language with the same itag');
 captured=UdmFormats.attachObserved(snapshot([encrypted]),[{itag:140,url:stream('es')+'&n=ready',observedAt:Date.now()}]);assert.equal(UdmFormats.audioChoices(captured,id).length,1);check('Matching observed playback supplies a transformed direct audio URL');
 const old={...audio(),lastModified:'2',url:'',matchUrl:stream()+'&lmt=2'};captured=UdmFormats.attachObserved(snapshot([old]),[{itag:140,url:stream()+'&lmt=1',observedAt:Date.now()}]);assert.equal(UdmFormats.audioChoices(captured,id).length,0);check('An older audio revision cannot supply the current track');
 assert.equal(UdmFormats.audioChoices(snapshot([audio(),{...audio(),url:stream('en','other')}]),id).length,0);check('Ambiguous duplicate track metadata is rejected');
 let h=harness();await h.run();assert.equal(h.calls.length,1);const sent=h.calls[0];assert.equal(sent.action,'add');assert.equal(sent.url,stream('es'));assert.equal(sent.referrer,url);assert.equal(sent.userAgent,'audio-browser');assert.match(sent.filename,/Español.*\.m4a$/);assert(!/[<>:"/\\|?*]/.test(sent.filename));assert.equal(sent.videoUrl,undefined);assert.equal(sent.visitorData,undefined);check('Selected audio uses one ordinary native add without player retrieval, SABR or video');
 h=harness();let response=await h.send({action:'formats',url,tabId:7});assert.equal(response.ok,true);assert.equal(response.choices.length,0);assert.equal(response.audioChoices.length,2);assert(!JSON.stringify(response).includes('googlevideo'));assert.equal(h.calls.length,0);check('Audio-only metadata produces a useful menu without signed URLs or native prefetch');
 h=harness();response=await h.send();assert.equal(response.ok,true);assert.equal(h.calls.length,1);check('Runtime message dispatch acknowledges the audio job');
 h=harness([snapshot([audio(),audio('es','old')]),snapshot([audio(),audio('es','renewed')])]);await h.run();assert.equal(h.calls[0].url,stream('es','renewed'));check('Final handoff uses renewed audio URL from the confirmed player');
 for(const [name,last,docs] of [
  ['document navigation',snapshot(),['old','new']],['changed video',{...snapshot(),videoId:'aaaaaaaaaaa'},['doc','doc']],['advertisement',null,['doc','doc']],
  ['removed selected language',snapshot([audio()]),['doc','doc']],['changed resource',snapshot([audio(),{...audio('es'),url:stream('es').replace('id=content','id=other')}]),['doc','doc']],
  ['fallback document navigation',{...snapshot(),timeOrigin:2000},['','']],['unknown document identity',{...snapshot(),timeOrigin:undefined},['','']]
 ]){h=harness([snapshot(),last],docs);await assert.rejects(h.run());assert.equal(h.calls.length,0);check('No native job after '+name);}
 h=harness([snapshot(),snapshot()],['','']);await h.run();assert.equal(h.calls.length,1);check('Firefox-style document binding uses unchanged performance timeOrigin');
 h=harness();h.store.desktopPolicy={panelEnabled:false};await assert.rejects(h.run());assert.equal(h.calls.length,0);check('Desktop panel policy applies to audio-only handoff');
 h=harness();h.api.tabs.get=async()=>({url,incognito:true});await assert.rejects(h.run());assert.equal(h.calls.length,0);check('Private tabs cannot hand off audio');
 h=harness();await assert.rejects(h.run(h.message,{tab:{id:7},url,documentId:'stale',frameId:0}));assert.equal(h.calls.length,0);check('Sender document must match the injected player');
 h=harness();await assert.rejects(h.run({...h.message,audioKey:'invented',audioUrl:stream('en')}));assert.equal(h.calls.length,0);check('A forged audio selection cannot supply its own URL');
 h=harness();h.api.runtime.sendNativeMessage=async()=>({ok:false,error:'Native rejected'});await assert.rejects(h.run(),/Native rejected/);check('Native rejection is reported without success or retry');
 const player={getVideoData:()=>({video_id:id}),getPlayerResponse:()=>({videoDetails:{videoId:id},streamingData:{adaptiveFormats:[{itag:140,mimeType:'audio/mp4; codecs="mp4a"',url:stream(),bitrate:128000,audioTrack:{id:'en.1',displayName:'English',audioIsDefault:true}},{itag:141,drmFamilies:['protected']}]}})};
 const page=vm.createContext({document:{getElementById:()=>player},location:new URL(url),URL,URLSearchParams,performance:{timeOrigin:1000}});vm.runInContext(readYouTubeFormats.toString(),page);const raw=page.readYouTubeFormats(id);assert.equal(raw.formats.length,1);assert.equal(raw.formats[0].audioTrackId,'en.1');assert.equal(raw.formats[0].audioName,'English');assert.equal(raw.formats[0].bitrate,128000);check('Real page reader preserves bounded audio metadata and excludes DRM tracks');
 player.classList={contains:x=>x==='ad-showing'};assert.equal(page.readYouTubeFormats(id),null);check('Real page reader rejects an active advertisement');
 console.log('ALL '+passed+' YOUTUBE AUDIO CHECKS PASSED');
})().catch(e=>{console.error(e);process.exitCode=1;});
