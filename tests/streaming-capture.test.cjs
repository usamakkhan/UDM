'use strict';
const assert=require('node:assert/strict'),vm=require('node:vm'),fs=require('node:fs'),path=require('node:path');
const base=path.resolve(__dirname,'../browser/chromium'),id='Q3TI27IN7X0',page='https://www.youtube.com/watch?v='+id;
const body=Uint8Array.of(42,1,1,154,1,2,8,1);let checks=0;
function pass(name){checks++;console.log('PASS '+name);}
function harness(){
 const stored={},events={},state={snapshot:{videoId:id,page,timeOrigin:Date.now()-10000},documentId:'doc-a',incognito:false};
 const event=name=>({addListener:fn=>events[name]=fn});
 const api={tabs:{get:async()=>({incognito:state.incognito}),onRemoved:event('removed'),onUpdated:event('updated')},webRequest:{onBeforeRequest:event('before'),onHeadersReceived:event('headers'),onErrorOccurred:event('failed')},scripting:{executeScript:async()=>{if(state.proofGate)await state.proofGate;return [{frameId:0,documentId:state.documentId,result:state.snapshot}];}},storage:{session:{get:async key=>({[key]:stored[key]}),set:async value=>Object.assign(stored,value),remove:async key=>{delete stored[key];}}}};
 const context=vm.createContext({URL,TextDecoder,Uint8Array,ArrayBuffer,btoa,Date,console});
 vm.runInContext(fs.readFileSync(path.join(base,'ump.js'),'utf8')+'\n'+fs.readFileSync(path.join(base,'streaming-capture.js'),'utf8')+'\nglobalThis.capture=UdmStreamingCapture;',context);context.capture.install(api);
 const request=(extra={})=>({requestId:'one',tabId:7,frameId:0,documentId:'doc-a',method:'POST',url:'https://rr1.googlevideo.com/videoplayback?sabr=1',requestBody:{raw:[{bytes:body.buffer}]},...extra});
 const response=(extra={})=>({...request(),statusCode:200,responseHeaders:[{name:'Content-Type',value:'application/vnd.yt-ump'}],...extra});
 const query=(extra={})=>context.capture.session({tabId:7,frameId:0,documentId:'doc-a',id,...extra});
 return {capture:context.capture,state,events,stored,request,response,query};
}
(async()=>{
 let h=harness();h.events.before(h.request());await h.capture.response(h.response());let session=await h.query();assert.equal(session.videoId,id);assert.equal(session.body,Buffer.from(body).toString('base64'));assert.equal(session.captureSource,'browser-request');pass('Browser API captures a matched successful streaming POST without relying on page fetch wrappers');
 assert.equal(await h.query({frameId:4}),null);assert.equal(await h.query({documentId:'doc-b'}),null);assert.equal(await h.query({id:'aaaaaaaaaaa'}),null);pass('Streaming offers stay bound to video, frame and document');
 await h.capture.clear(7);assert.equal(await h.query(),null);pass('Navigation clearing removes streaming request context');
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
