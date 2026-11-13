'use strict';
const assert=require('node:assert/strict'),vm=require('node:vm'),fs=require('node:fs'),path=require('node:path');
const base=process.env.UDM_BROWSER_SOURCE||path.resolve(__dirname,'../browser/chromium'),id='Q3TI27IN7X0',page='https://www.youtube.com/watch?v='+id;
const body=Uint8Array.of(42,1,1,154,1,2,8,1);let checks=0;
function pass(name){checks++;console.log('PASS '+name);}
function harness(startTime=Date.now()){
 let now=startTime;
 const stored={},events={},state={snapshot:{videoId:id,page,timeOrigin:now-10000,visit:3},documentId:'doc-a',incognito:false};
 const event=name=>({addListener:fn=>events[name]=fn});
 const api={tabs:{get:async()=>({incognito:state.incognito}),onRemoved:event('removed'),onUpdated:event('updated')},webRequest:{onBeforeRequest:event('before'),onHeadersReceived:event('headers'),onErrorOccurred:event('failed')},scripting:{executeScript:async()=>{if(state.proofGate)await state.proofGate;return [{frameId:0,documentId:state.documentId,result:state.snapshot}];}},storage:{session:{get:async key=>({[key]:stored[key]}),set:async value=>Object.assign(stored,value),remove:async key=>{delete stored[key];}}}};
 const context=vm.createContext({URL,TextDecoder,Uint8Array,ArrayBuffer,btoa,Date:{now:()=>now},console});
 vm.runInContext(fs.readFileSync(path.join(base,'ump.js'),'utf8')+'\n'+fs.readFileSync(path.join(base,'streaming-capture.js'),'utf8')+'\nglobalThis.capture=UdmStreamingCapture;',context);context.capture.install(api);
 const request=(extra={})=>({requestId:'one',tabId:7,frameId:0,documentId:'doc-a',method:'POST',url:'https://rr1.googlevideo.com/videoplayback?sabr=1',requestBody:{raw:[{bytes:body.buffer}]},...extra});
 const response=(extra={})=>({...request(),statusCode:200,responseHeaders:[{name:'Content-Type',value:'application/vnd.yt-ump'}],...extra});
 const query=(extra={})=>context.capture.session({tabId:7,frameId:0,documentId:'doc-a',id,snapshot:{visit:3},...extra});
 return {capture:context.capture,state,events,stored,request,response,query,advance:(delta=100)=>{now+=delta;}};
}
(async()=>{
 let h=harness();h.events.before(h.request());await h.capture.response(h.response());let session=await h.query();assert.equal(session.videoId,id);assert.equal(session.body,Buffer.from(body).toString('base64'));assert.equal(session.captureSource,'browser-request');pass('Browser API captures a matched successful streaming POST without relying on page fetch wrappers');
 for(const marker of ['el=adunit','advideo=1','el=detailpage&el=adunit']){h.stored['sabr:7'][0].url=session.url+'&'+marker;assert.equal(await h.query(),null);}h.stored['sabr:7'][0].url=session.url;pass('Saved streaming sessions revalidate ad markers after an extension reload');
 assert.equal(await h.query({frameId:4}),null);assert.equal(await h.query({documentId:'doc-b'}),null);assert.equal(await h.query({id:'aaaaaaaaaaa'}),null);pass('Streaming offers stay bound to video, frame and document');
 assert.equal(await h.query({snapshot:{visit:4}}),null);pass('Returning to the same video cannot reuse a capture from its previous visit');
 h=harness();
 const older=h.request({requestId:'older'}),newer=h.request({requestId:'newer',url:older.url+'&rn=2'});
 h.events.before(older);h.advance();h.events.before(newer);
 await h.capture.response(h.response(newer));
 await h.capture.response(h.response(older));
 assert.equal((await h.query()).url,newer.url);pass('An older request finishing late cannot replace the latest accepted capture');
 await h.capture.clear(7);assert.equal(await h.query(),null);pass('Navigation clearing removes streaming request context');
 {
  const fixed=Date.now();h=harness(fixed);const first=h.request({requestId:'tie-old'}),second=h.request({requestId:'tie-new',url:first.url+'&rn=tie-new'});
  h.events.before(first);h.events.before(second);await h.capture.response(h.response(second));await h.capture.response(h.response(first));
  assert.equal((await h.query()).url,second.url);pass('Equal wall-clock timestamps retain request ordering through browser-level capture');
  assert.equal(Object.hasOwn(await h.query(),'captureOrder'),false);assert.equal(Object.hasOwn(await h.query(),'captureOrderEpoch'),false);pass('Internal ordering metadata stays out of the streaming offer');
  h=harness(fixed);h.events.before(h.request());await h.capture.response(h.response());h.advance(-1000);const rollback=h.request({requestId:'rollback',url:h.request().url+'&rn=rollback'});h.events.before(rollback);await h.capture.response(h.response(rollback));
  assert.equal((await h.query()).url,rollback.url);pass('A newer same-worker capture survives a backward wall-clock adjustment');
  const previous=harness(fixed);previous.events.before(previous.request({requestId:'one'}));previous.events.before(previous.request({requestId:'two',url:previous.request().url+'&rn=previous'}));await previous.capture.response(previous.response({requestId:'two',url:previous.request().url+'&rn=previous'}));
  const restarted=harness(fixed);Object.assign(restarted.stored,previous.stored);const fresh=restarted.request({requestId:'restart',url:restarted.request().url+'&rn=restart'});restarted.events.before(fresh);await restarted.capture.response(restarted.response(fresh));
  assert.equal((await restarted.query()).url,fresh.url);pass('Restarted worker ordering does not compare its first request with the old worker sequence');
 }
 h=harness();let releaseProof;h.state.proofGate=new Promise(resolve=>{releaseProof=resolve;});h.events.before(h.request());const lateResponse=h.capture.response(h.response());await h.capture.clear(7);releaseProof();await lateResponse;assert.equal(await h.query(),null);assert.equal(h.capture.diagnostics(7).accepted,undefined);pass('A delayed identity lookup cannot restore a capture after navigation');
 h=harness();h.events.before(h.request());await h.capture.clear(7);await h.capture.response(h.response());assert.equal(await h.query(),null);pass('A response from a request removed by navigation cannot create a new offer');

 for(const statusCode of [302,403,500]){h=harness();h.events.before(h.request());await h.capture.response(h.response({statusCode}));assert.equal(await h.query(),null);}pass('Redirected and rejected streaming responses are not offered');
 h=harness();h.events.before(h.request());await h.capture.response(h.response({responseHeaders:[{name:'Content-Type',value:'video/mp4'}]}));assert.equal(await h.query(),null);pass('A raw MP4 response is not mistaken for framed UMP');
 for(const change of [{snapshot:null},{documentId:'other-document'},{incognito:true}]){h=harness();Object.assign(h.state,change);h.events.before(h.request());await h.capture.response(h.response());assert.equal(await h.query(),null);}pass('Ads/unmatched players, changed documents and private tabs do not create capture offers');
 h=harness();h.events.before(h.request({requestBody:{raw:[{bytes:body.slice(0,3).buffer},{bytes:body.slice(3).buffer}]}}));await h.capture.response(h.response());assert.equal((await h.query()).body,Buffer.from(body).toString('base64'));pass('Multi-chunk browser request bodies are reconstructed exactly');
 for(const requestBody of [{error:'unavailable'},{raw:[{file:'private.file'}]},{raw:[{bytes:new ArrayBuffer(131073)}]},{raw:[{bytes:Uint8Array.of(10,127).buffer}]}]){h=harness();h.events.before(h.request({requestBody}));await h.capture.response(h.response());assert.equal(await h.query(),null);}pass('Unavailable, file-backed, oversized and malformed request bodies are rejected');
 h=harness();h.events.before(h.request({frameId:-1}));await h.capture.response(h.response());assert.equal(await h.query(),null);assert.equal(h.capture.diagnostics(7).unassociatedFrame,1);pass('Unassociated worker requests are counted instead of assigned to an arbitrary player');
 h=harness();h.events.before(h.request());await h.capture.response(h.response({documentId:'doc-b'}));assert.equal(await h.query(),null);pass('Response document identity must match the captured request');
 h=harness();h.events.before(h.request());h.events.failed({requestId:'one'});await h.capture.response(h.response());assert.equal(await h.query(),null);pass('Failed requests release their pending capture');
 h=harness();assert.equal(h.capture.endpoint('https://googlevideo.com.evil.test/videoplayback?sabr=1'),false);assert.equal(h.capture.endpoint('http://rr1.googlevideo.com/videoplayback?sabr=1'),false);pass('Streaming observation is restricted to HTTPS playback endpoints');
 const {UdmFormats}=require('../browser/chromium/formats.js');const direct=itag=>'https://rr1.googlevideo.com/videoplayback?itag='+itag+'&expire=4102444800';
 const formats=[{id:'137',height:1080,mime:'video/mp4; codecs="avc1"',url:''},{id:'399',height:1080,mime:'video/mp4; codecs="av01"',url:direct(399)},{id:'140',mime:'audio/mp4; codecs="mp4a"',url:direct(140)}];
 assert.equal(UdmFormats.choices({videoId:id,formats},id)[0].formatId,'399');pass('A downloadable codec at the selected quality takes priority over an unavailable preferred codec');
 const framed={videoId:id,formats:[{id:'18',height:360,muxed:true,mime:'video/mp4; codecs="avc1,mp4a"',url:direct(18)+'&sabr=1'}]};assert.equal(UdmFormats.choices(framed,id)[0].videoUrl,'');pass('Framed SABR URLs are never sent as ordinary MP4 links');
 const player={getVideoData:()=>({video_id:id}),classList:{contains:name=>name==='ad-showing'}};
 const context=vm.createContext({URL,location:{href:page,pathname:'/watch'},document:{getElementById:()=>player}});vm.runInContext(fs.readFileSync(path.join(base,'formats.js'),'utf8'),context);assert.equal(context.readYouTubeFormats(id),null);pass('An explicit ad state blocks the format menu even if the player retains the content video ID');
 const levels=[];const prepareContext=vm.createContext({URL,location:{href:page,pathname:'/watch'},document:{getElementById:()=>({getVideoData:()=>({video_id:id}),classList:{contains:()=>false},getAvailableQualityLevels:()=>['hd1080'],setPlaybackQualityRange:(a,b)=>levels.push([a,b])})}});vm.runInContext(fs.readFileSync(path.join(base,'capture.js'),'utf8'),prepareContext);
 for(const route of ['/shorts/','/embed/']){prepareContext.location.href='https://www.youtube.com'+route+id;prepareContext.location.pathname=route+id;assert.equal(prepareContext.__udmCaptureV1.prepare(id,1080),true);}assert.equal(levels.length,2);pass('Quality preparation supports Shorts and embedded players');
 console.log('ALL '+checks+' VIDEO CAPTURE REGRESSIONS PASSED');
})().catch(e=>{console.error(e);process.exitCode=1;});
