'use strict';
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
const base=process.env.UDM_BROWSER_SOURCE||path.resolve(__dirname,'../browser/chromium');
const {UdmFormats}=require(path.join(base,'formats.js'));
const id='Q3TI27IN7X0',url='https://www.youtube.com/watch?v='+id;
const audio=(lang='en')=>({id:'140',mime:'audio/mp4; codecs="mp4a.40.2"',lastModified:'123456790',audioTrackId:lang+'.1',audioName:lang==='en'?'English':'Español',audioDefault:lang==='en',bitrate:128000,xtags:'lang='+lang,url:''});
const snapshot=()=>({videoId:id,timeOrigin:1000,durationMs:180000,formats:[audio(),audio('es')]});
const session=()=>({videoId:id,url:'https://rr1.googlevideo.com/videoplayback?sabr=1',body:'fixture',capturedAt:Date.now()});
function harness(){
 const calls=[],events={},state={snapshots:[snapshot(),snapshot()],documents:['doc','doc'],session:session(),sessionDocument:'doc',timeOrigin:1000,capability:true};let reads=0;
 const event=n=>({addListener:f=>events[n]=f});
 const api={runtime:{id:'fixture',onInstalled:event('installed'),onMessage:event('message'),sendNativeMessage:async(_,m)=>{calls.push(m);return m.action==='hello'?{ok:true,capabilities:state.capability?['sabr-audio']:[]}:{ok:true,id:'audio-job'};}},storage:{local:{get:async()=>({}),set:async()=>{}},session:{get:async()=>({}),remove:async()=>{},set:async()=>{}}},tabs:{get:async()=>({url,incognito:false,title:'Audio test'}),onRemoved:event('remove'),onUpdated:event('update')},scripting:{executeScript:async args=>{
  if(args.func.name==='readYouTubeFormats'){const n=Math.min(reads++,state.snapshots.length-1);return [{frameId:0,documentId:state.documents[n],result:state.snapshots[n]}];}
  return [{frameId:0,documentId:state.sessionDocument,result:{session:state.session,timeOrigin:state.timeOrigin}}];
 }},action:{setBadgeText:async()=>{}},contextMenus:{onClicked:event('context')},downloads:{onCreated:event('download')},webRequest:{onHeadersReceived:event('headers')}};
 const context=vm.createContext({chrome:api,URL,URLSearchParams,navigator:{userAgent:'audio-browser'},console,setTimeout,clearTimeout});
 for(const name of ['file-recognition.js','media.js','formats.js','background.js'])vm.runInContext(fs.readFileSync(path.join(base,name),'utf8'),context);
 const message={action:'youtube-audio',url,tabId:7,audioKey:UdmFormats.audioChoices(snapshot(),id,true)[1].key,title:'Streaming audio'};
 return {calls,state,context,run:()=>context.audioHandoff(message,{})};
}
let passed=0;function check(name){++passed;console.log('PASS '+name);}
(async()=>{
 let choices=UdmFormats.audioChoices(snapshot(),id,true);assert.equal(choices.length,2);assert.equal(choices[1].audioUrl,'');assert.equal(choices[1].streamFormat.xtags,'lang=es');assert.equal(UdmFormats.audioChoices(snapshot(),id).length,0);check('Explicit streaming choices retain the complete selected language identity');
 for(const [name,alter] of [['unknown revision',s=>s.formats.forEach(f=>delete f.lastModified)],['overflow revision',s=>s.formats.forEach(f=>f.lastModified='18446744073709551616')],['oversize tags',s=>s.formats.forEach(f=>f.xtags='a'.repeat(2049))],['missing duration',s=>delete s.durationMs],['live stream',s=>s.live=true],['wrong video',s=>s.videoId='abcdefghijk'],['video format',s=>s.formats.forEach(f=>f.mime='video/mp4; codecs="avc1"')]]){let s=snapshot();alter(s);assert.equal(UdmFormats.audioChoices(s,id,true).length,0);check('No streaming audio choice for '+name);}
 let h=harness();await h.run();assert.deepEqual(h.calls.map(x=>x.action),['hello','sabr']);let sent=h.calls[1];assert.equal(sent.output,'audio');assert.equal(sent.sabr.audio.xtags,'lang=es');assert.equal(sent.sabr.video,undefined);assert.equal(sent.height,0);assert.equal(sent.pixelHeight,0);assert.match(sent.filename,/Español.*\.m4a$/);assert.equal(sent.videoUrl,undefined);assert.equal(sent.cookies,undefined);check('Streaming handoff selects only Spanish audio with no video, cookies or player retrieval');
 for(const [name,alter] of [
  ['older desktop',h=>h.state.capability=false],['missing playback session',h=>h.state.session=null],['expired session',h=>h.state.session.capturedAt=Date.now()-360000],['future session',h=>h.state.session.capturedAt=Date.now()+60000],
  ['foreign video session',h=>h.state.session.videoId='abcdefghijk'],['foreign session document',h=>h.state.sessionDocument='other'],['untrusted endpoint',h=>h.state.session.url='https://googlevideo.com.evil.test/videoplayback'],['credentialed endpoint',h=>h.state.session.url='https://user:pass@rr1.googlevideo.com/videoplayback'],
  ['navigation during session lookup',h=>h.state.documents[1]='new'],['advertisement before handoff',h=>h.state.snapshots[1]=null],['changed audio revision',h=>h.state.snapshots[1].formats[1].lastModified='123456791'],['changed duration',h=>h.state.snapshots[1].durationMs=190000],['removed language',h=>h.state.snapshots[1].formats.pop()]
 ]){h=harness();alter(h);await assert.rejects(h.run());assert(!h.calls.some(x=>['add','sabr','youtube-player'].includes(x.action)));check('No download after '+name);}
 h=harness();h.state.documents=['',''];h.state.sessionDocument='';await h.run();assert.equal(h.calls[1].action,'sabr');check('Firefox-style timeOrigin binds streaming audio to the same document');
 h=harness();h.state.documents=['',''];h.state.sessionDocument='';h.state.timeOrigin=2000;await assert.rejects(h.run());assert.equal(h.calls.length,1);check('Changed Firefox-style session document cannot hand off audio');
 h=harness();h.state.snapshots[1].formats[1].url='https://rr1.googlevideo.com/videoplayback?id=content&itag=140&expire=4102444800&xtags=lang%3Des';await h.run();assert.equal(h.calls[1].action,'add');check('Fresh direct audio discovered before handoff takes priority over streaming');
 console.log('ALL '+passed+' YOUTUBE STREAM AUDIO CHECKS PASSED');
})().catch(e=>{console.error(e);process.exitCode=1;});
